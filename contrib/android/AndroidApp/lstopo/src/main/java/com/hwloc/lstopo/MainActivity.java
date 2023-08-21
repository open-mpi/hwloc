package com.hwloc.lstopo;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.drawable.GradientDrawable;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;

import com.android.volley.toolbox.StringRequest;
import com.google.android.material.navigation.NavigationView;

import androidx.appcompat.app.ActionBarDrawerToggle;
import androidx.core.app.ActivityCompat;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;
import androidx.core.view.GravityCompat;

import androidx.appcompat.widget.Toolbar;
import androidx.drawerlayout.widget.DrawerLayout;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.Window;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import com.android.volley.Request;
import com.android.volley.RequestQueue;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.JsonObjectRequest;
import com.android.volley.toolbox.Volley;
import com.hwloc.lstopo.ZoomView.ZoomView;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

import static android.view.View.GONE;
import static android.view.View.TEXT_ALIGNMENT_CENTER;
import static android.view.View.VISIBLE;

public class MainActivity extends AppCompatActivity implements NavigationView.OnNavigationItemSelectedListener {

    // Used to load the C library on application startup (defined in CMakelist).
    static {
        System.loadLibrary("lib");
    }

    /**
     * Allows java to call lstopo : C library previously defined.
     * */
    public native int start(Lstopo lstopo, int drawing_method, String outputFile, ArrayList<String> options);
    public native int startWithInput(Lstopo lstopo, int drawing_method, String outputFile, String inputFile, ArrayList<String> options);

    // Frame the application draw on
    private RelativeLayout layout;
    private ZoomView zoomView;
    // Graphic tools that draw the topology
    private Lstopo lstopo;
    private Toolbar toolbar;
    // Menu components
    private DrawerLayout drawerLayout;
    private NavigationView navigationView;
    // current instance of this class' activity
    private Activity activity;
    // Filters to download topology
    private String requestTag;
    // Files use to store or retrieve topology information
    private File txtFile;
    private File xmlFile;
    private File jpgFile;
    private File debugFile;
    // Queue to send API get requests
    private RequestQueue queue;
    // Current menu mode
    private String mode = null;
    // Progress dialog on URL request
    private ProgressDialog progressDialog;
    // Buttons
    Button filtersButton;
    Button optionsButton;
    // Current navigation
    MenuItems menuItems;
    // Communication with thread
    Handler handler;
    // Current topology
    String topology = "phone";
    // Options to draw the topology
    ArrayList<String> options = new ArrayList<>();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(R.layout.activity_main);
        zoomView = findViewById(R.id.zoomview);

        this.configureToolBar();
        this.configureDrawerLayout();
        this.configureNavigationView();

        layout = findViewById(R.id.relative_layout);
        lstopo = new Lstopo(this);
        activity = this;
        queue = Volley.newRequestQueue(this);

        txtFile = getAbsoluteFile("/topology.txt");
        xmlFile = getAbsoluteFile("/topology.xml");
        jpgFile = getAbsoluteFile("/topology.jpg");
        debugFile = getAbsoluteFile("/application_log.txt");

        filtersButton = findViewById(R.id.filters);
        filtersButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                startActivityForResult(new Intent(MainActivity.this, Filters.class), 3);
            }
        });

        optionsButton = findViewById(R.id.options);
        optionsButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(MainActivity.this, Options.class);
                for(int i = 0; i < options.size(); i++) {
                    intent.putExtra(Integer.toString(i),options.get(i));
                }
                startActivityForResult(intent, 4);
            }
        });

        handler = new Handler(Looper.getMainLooper()) {
             @Override
             public void handleMessage(Message inputMessage) {
                 Toast.makeText(MainActivity.this, "Timeout. Retrying in case the online database was asleep.", Toast.LENGTH_LONG).show();
             }
        };

        start(lstopo, 1, "", options);
        setMode("draw");
        menuItems = new MenuItems();
    }

    /**
     * Close menu on touch
     */
    @Override
    public void onBackPressed() {
        if (this.drawerLayout.isDrawerOpen(GravityCompat.START)) {
            this.drawerLayout.closeDrawer(GravityCompat.START);
        } else {
            super.onBackPressed();
        }
    }

    /**
     * Add share button
     */
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.share_menu, menu);
        return true;
    }

    /**
     * Handle toolbar navigation
     */
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.share:
                share();
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    /**
     * Handle menu navigation
     */
    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
        int id = item.getItemId();
        menuItems.setCheckedItems(item);
        zoomView.resetZoom();

        switch (id){
            case R.id.activity_main_drawer_draw :
                if(topology.equals("phone") && !menuItems.isInputPhoneSelected())
                    menuItems.setPhoneInput();

                lstopo.clearDebugFile();
                layout.removeAllViews();

                if(topology.equals("phone"))
                    start(lstopo, 1, "", options);
                else
                    startWithInput(lstopo, 1, "", topology, options);
                setMode("draw");
                break;
            case R.id.activity_main_drawer_text:
                if(topology.equals("phone") && !menuItems.isInputPhoneSelected())
                    menuItems.setPhoneInput();

                try {
                    lstopo.clearDebugFile();
                    layout.removeAllViews();
                    //JNI can't overwrite file
                    txtFile.delete();
                    if(topology.equals("phone"))
                        start(lstopo, 2, txtFile.getAbsolutePath(), options);
                    else
                        startWithInput(lstopo, 2, txtFile.getAbsolutePath(), topology, options);
                    setMode("txt");
                    String lstopoText = readFile(txtFile);
                    lstopo.text(lstopoText, 0, 0, 0, 0, -1);
                    break;
                } catch (IOException e) {
                    e.printStackTrace();
                    break;
                }
            case R.id.activity_main_drawer_xml:
                if(topology.equals("phone") && !menuItems.isInputPhoneSelected())
                    menuItems.setPhoneInput();

                try {
                    lstopo.clearDebugFile();
                    layout.removeAllViews();
                    //JNI can't overwrite file
                    xmlFile.delete();
                    if(topology.equals("phone"))
                        start(lstopo, 3, xmlFile.getAbsolutePath(), options);
                    else
                        startWithInput(lstopo, 3, xmlFile.getAbsolutePath(), topology, options);
                    setMode("xml");
                    String lstopoText = readFile(xmlFile);
                    lstopo.text(lstopoText, 0, 0, 0, 0, -1);
                    break;
                } catch (IOException e) {
                    e.printStackTrace();
                    break;
                }
            case R.id.activity_main_drawer_MyPhone:
                topology = "phone";
                layout.removeAllViews();

                start(lstopo, 1, "", options);
                menuItems.setDrawFormat();
                mode = "draw";
                break;
            case R.id.activity_main_drawer_database:
                /* menuItems.setDrawFormat();
                downloadTopology(); */
                Uri uriDatabase = Uri.parse("https://hwloc.gitlabpages.inria.fr/xmls/");
                Intent intentDatabase = new Intent(Intent.ACTION_VIEW, uriDatabase);
                startActivity(intentDatabase);
                break;
            case R.id.activity_main_drawer_LocalXML:
                file_picker();
                break;
            case R.id.activity_main_drawer_Synthetic_Topology:
                createSyntheticLayout();
                break;
            case R.id.activity_main_drawer_debug:
                final Uri data = FileProvider.getUriForFile(this, "com.hwloc.lstopo.fileprovider", debugFile);
                this.grantUriPermission(this.getPackageName(), data, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                final Intent intent = new Intent(Intent.ACTION_VIEW)
                        .setDataAndType(data, "text/plain")
                        .addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
                this.startActivity(intent);
                break;
            case R.id.activity_main_drawer_about:
                Intent intentAbout = new Intent(MainActivity.this, About.class);
                startActivityForResult(intentAbout, 5);
                break;
            default:
                break;
        }

        this.drawerLayout.closeDrawer(GravityCompat.START);

        return true;
    }

    /**
     * Set current mode and handle button visibility according to current menu
     * */
    private void setMode(String mode){
        this.mode = mode;
        switch(mode){
            case "draw":
                filtersButton.setVisibility(GONE);
                optionsButton.setVisibility(VISIBLE);

                break;
            case "xml":
            case "txt":
                filtersButton.setVisibility(GONE);
                optionsButton.setVisibility(VISIBLE);
                break;
            case "download":
                filtersButton.setVisibility(VISIBLE);
                optionsButton.setVisibility(GONE);
                break;
            case "synthetic":
                filtersButton.setVisibility(GONE);
                optionsButton.setVisibility(GONE);
                break;
        }
    }

    public void file_picker(){
        if (checkPermission()) {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            String [] mimeTypes = {"application/xml", "text/xml"};
            intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
            startActivityForResult(intent, 2);
        } else {
            requestPermission();
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode,
                                 Intent resultData) {
        //XML display
        super.onActivityResult(requestCode, resultCode, resultData);
        if ((requestCode == 2 || requestCode == 21) && resultCode == Activity.RESULT_OK) {
            if (resultData != null) {
                try {
                    String data;
                    layout.removeAllViews();

                    //File from file picker
                    if (requestCode == 2)
                        data = readFile(resultData.getData());
                    //File from API
                    else
                        data = resultData.getDataString();

                    topology = getAbsoluteFile("/inputTopology.xml").getAbsolutePath();
                    setMode("draw");
                    writeFile(topology, data);

                    if(startWithInput(lstopo, 1, "", topology, options) != 0)
                        Toast.makeText(MainActivity.this, "XML not valid.", Toast.LENGTH_LONG).show();

                } catch (IOException e) {
                    e.printStackTrace();
                }
            }

        } else if (requestCode == 1 && resultCode == Activity.RESULT_OK) {
            //Open file picker
            if (resultData.getData().toString().equals("xml"))
                file_picker();
            //Handle filters for API
        } else if (requestCode == 3 && resultCode == Activity.RESULT_OK) {
            requestTag = Uri.encode(resultData.getData().toString());
            downloadTopology();
        } else if (requestCode == 4 && resultCode == Activity.RESULT_OK) {
            layout.removeAllViews();
            options = resultData.getStringArrayListExtra("options");
            if(topology.equals("phone"))
                start(lstopo, 1, "", options);
            else
                startWithInput(lstopo, 1, "", topology, options);
        }
    }

    /**
     * Send current display by mail
     * */
    public void share(){
        String content = lstopo.getCurrentContent();
        String state = Environment.getExternalStorageState();

        File file;
        switch(mode) {
            case "jpg":
                file = jpgFile;
                break;
            case "xml":
                file = xmlFile;
                break;
            case "txt":
                file = txtFile;
                break;
            default:
                file = jpgFile;
        }

        if (Environment.MEDIA_MOUNTED.equals(state)) {
            try {
                if(mode == "draw"){
                    FileOutputStream os = new FileOutputStream(file);
                    View v1 = layout;
                    v1.setDrawingCacheEnabled(true);
                    Bitmap bitmap = getBitmapFromView(v1, ((RelativeLayout) v1).getChildAt(0).getHeight(), ((RelativeLayout) v1).getChildAt(0).getWidth());
                    v1.setDrawingCacheEnabled(false);
                    int quality = 100;
                    bitmap.compress(Bitmap.CompressFormat.JPEG, quality, os);
                    os.flush();
                } else {
                    writeFile(file, content);
                }
            } catch (IOException e) {
                e.printStackTrace();
                }
        }

        Uri path = FileProvider.getUriForFile(activity, "com.hwloc.lstopo.fileprovider", file);

        Intent emailIntent = new Intent(Intent.ACTION_SEND);
        emailIntent.setType("vnd.android.cursor.dir/email");
        emailIntent.putExtra(Intent.EXTRA_STREAM, path);
        emailIntent.putExtra(Intent.EXTRA_SUBJECT, "topology." + (mode.equals("draw") ? "jpg" : mode ));
        
        Intent intent = Intent.createChooser(emailIntent, "Send email...");
        List<ResolveInfo> resInfoList = getPackageManager().queryIntentActivities(intent, PackageManager.MATCH_DEFAULT_ONLY);
        for (ResolveInfo resolveInfo : resInfoList) {
            String packageName = resolveInfo.activityInfo.packageName;
            grantUriPermission(packageName, path, Intent.FLAG_GRANT_WRITE_URI_PERMISSION | Intent.FLAG_GRANT_READ_URI_PERMISSION);
        }
        startActivity(intent);
    }

    private boolean checkPermission() {
        int result = ContextCompat.checkSelfPermission(MainActivity.this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE);
        if (result == PackageManager.PERMISSION_GRANTED) {
            return true;
        } else {
            Toast.makeText(MainActivity.this, "External Storage permission allows us to use files. Please allow this permission in App Settings.", Toast.LENGTH_LONG).show();
            return false;
        }
    }

    /**
     * Check network connection
     * */
    private boolean isNetworkAvailable() {
        ConnectivityManager connectivityManager
                = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetworkInfo = connectivityManager.getActiveNetworkInfo();
        return activeNetworkInfo != null && activeNetworkInfo.isConnectedOrConnecting();
    }

    /**
     * Get the list of downloadable topology from API
     * */
    public void downloadTopology(){
        setMode("download");
        layout.removeAllViews();
        String tags = requestTag != null ? "tags/" + requestTag : "";
        final String url ="https://hwloc-xmls.herokuapp.com/" + tags;

        if(!isNetworkAvailable()) {
            Toast.makeText(MainActivity.this, "No internet connection.", Toast.LENGTH_LONG).show();
            return;
        }

        Display display = getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);

        final LinearLayout linearLayout = new LinearLayout(activity);
        linearLayout.setMinimumWidth(LinearLayout.LayoutParams.WRAP_CONTENT);
        linearLayout.setOrientation(LinearLayout.VERTICAL);
        ScrollView scrollView = new ScrollView(activity);
        scrollView.setFillViewport(true);
        layout.addView(scrollView);
        scrollView.addView(linearLayout);

        final Thread checkTimeOut = new Thread(){
            public void run()
            {
                try {
                    Thread.sleep(2000);
                    Message completeMessage = handler.obtainMessage();
                    completeMessage.sendToTarget();
                } catch (InterruptedException e) {
                    Log.d("Utility info :", "Thread 'check time out' interrupted with success GET.");
                }
            }
        };

        JsonObjectRequest jsonObjectRequest = new JsonObjectRequest(Request.Method.GET, url, new Response.Listener<JSONObject>() {
            @Override
            public void onResponse(JSONObject response) {
                checkTimeOut.interrupt();
                setJsonList(response, linearLayout);
            }
        }, new Response.ErrorListener() {
            @Override
            public void onErrorResponse(VolleyError error) {
                if(error.networkResponse != null && error.networkResponse.statusCode == 503) {
                    checkTimeOut.interrupt();
                    Toast.makeText(MainActivity.this, "Platforms database is under maintenance.", Toast.LENGTH_LONG).show();
                    return;
                }

                JsonObjectRequest jsonObjectRequestRetry = new JsonObjectRequest(Request.Method.GET, url, new Response.Listener<JSONObject>() {
                    @Override
                    public void onResponse(JSONObject response) {
                        setJsonList(response, linearLayout);
                    }
                }, new Response.ErrorListener() {
                    @Override
                    public void onErrorResponse(VolleyError error) {
                        Toast.makeText(MainActivity.this, "Something went wrong reaching the database.", Toast.LENGTH_LONG).show();
                    }
                });

                try {
                    Thread.sleep(3000);
                    queue.add(jsonObjectRequestRetry);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }
            }
        });

        checkTimeOut.start();
        queue.add(jsonObjectRequest);
        linearLayout.setMinimumHeight(linearLayout.getHeight() + 100);
        linearLayout.setMinimumWidth(lstopo.getScreen_width());
    }

    /**
     * Create synthetic input menu
     * */
    public void createSyntheticLayout() {
        setMode("synthetic");
        layout.removeAllViews();
        layout.setMinimumWidth(lstopo.getScreen_width());
        layout.setMinimumHeight(lstopo.getScreen_height());

        final EditText edit = new EditText(this);
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                lstopo.getScreen_width() / 3,
                RelativeLayout.LayoutParams.WRAP_CONTENT
        );

        edit.setX((float)(lstopo.getScreen_width() - params.width) / 2);
        edit.setY((float) lstopo.getScreen_height() / 3);
        //edit.setLayoutParams(params);

        final Button startSynthetic = new Button(this);
        startSynthetic.setText("Start synthetic");
        RelativeLayout.LayoutParams params2 = new RelativeLayout.LayoutParams(
                lstopo.getScreen_width() / 4,
                RelativeLayout.LayoutParams.WRAP_CONTENT
        );

        startSynthetic.setX((float)(lstopo.getScreen_width() - params2.width) / 2);
        startSynthetic.setY((float) lstopo.getScreen_height() / 3 + (float) lstopo.getScreen_height() / 10);

        layout.addView(edit, params);
        layout.addView(startSynthetic, params2);

        GradientDrawable shape =  new GradientDrawable();
        shape.setStroke(4, Color.BLACK);
        shape.setCornerRadius(12);
        shape.setColor(Color.WHITE);

        edit.setBackground(shape);
        edit.setTextSize(20);
        edit.setHintTextColor(Color.LTGRAY);
        edit.setHint("node:2 pu:3");
        edit.setPadding(30, 0, 30, 0);
        edit.setVisibility(VISIBLE);

        startSynthetic.setVisibility(VISIBLE);
        startSynthetic.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                menuItems.setDrawFormat();
                layout.removeAllViews();
                topology = edit.getText().toString();
                setMode("draw");

                if(topology.equals("") || startWithInput(lstopo, 1, "", topology, options) != 0 )
                    Toast.makeText(MainActivity.this, "Synthetic topology not valid...", Toast.LENGTH_LONG).show();
            }
        });
    }

    /**
     * Display the list of downloadable topology from API
     * */
    public void setJsonList(JSONObject response, LinearLayout linearLayout){
        try {
            String title;
            JSONArray xmls = response.getJSONArray("xml");
            for(int i = 0 ; i < xmls.length() ; i++){
                title = xmls.getJSONObject(i).getString("title");
                TextView tv = new TextView(activity);
                tv.setTextAlignment(TEXT_ALIGNMENT_CENTER);
                tv.setTextColor(Color.parseColor("#4295f7"));
                tv.setY(i * 50);
                tv.setText(title);
                tv.setTextSize(20);
                tv.setMaxWidth(lstopo.getScreen_width());

                linearLayout.setMinimumHeight(linearLayout.getMinimumHeight() + 150);
                linearLayout.addView(tv);

                tv.setOnClickListener(new OnClickListenerWithParam(title, this));
            }
        } catch (JSONException e) {
            e.printStackTrace();
        }
    }

    public String readFile(Uri uri) throws IOException {
        InputStream inputStream = getContentResolver().openInputStream(uri);
        BufferedReader r = new BufferedReader(new InputStreamReader(inputStream));
        StringBuilder total = new StringBuilder();
        for (String line; (line = r.readLine()) != null; ) {
            total.append(line).append('\n');
        }
        return total.toString();
    }

    public String readFile(File file) throws IOException {
        Uri uri = Uri.fromFile(file);
        InputStream inputStream = getContentResolver().openInputStream(uri);
        BufferedReader r = new BufferedReader(new InputStreamReader(inputStream));
        StringBuilder total = new StringBuilder();
        for (String line; (line = r.readLine()) != null; ) {
            total.append(line).append('\n');
        }
        return total.toString();
    }

    public void writeFile(File file, String content) {
        try {
            FileWriter writer = new FileWriter(file);
            writer.append(content);
            writer.flush();
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void writeFile(String path, String content) {
        try {
            File file = new File(path);
            FileWriter writer = new FileWriter(file);
            writer.append(content);
            writer.flush();
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private void requestPermission() {
        if (ActivityCompat.shouldShowRequestPermissionRationale(MainActivity.this, android.Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            Toast.makeText(MainActivity.this, "External Storage permission allows us to use files. Please allow this permission in App Settings.", Toast.LENGTH_LONG).show();
        } else {
            ActivityCompat.requestPermissions(MainActivity.this, new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE}, 100);
        }
    }

    private Bitmap getBitmapFromView(View view, int height, int width) {
        Bitmap bitmap = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
        Canvas canvas = new Canvas(bitmap);
        Drawable bgDrawable = view.getBackground();
        if (bgDrawable != null)
            bgDrawable.draw(canvas);
        else
            canvas.drawColor(Color.WHITE);
        view.draw(canvas);
        return bitmap;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        switch (requestCode) {
            case 100:
                if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    Log.e("value", "Permission Granted, Now you can use local drive .");
                } else {
                    Log.e("value", "Permission Denied, You cannot use local drive .");
                }
                break;
        }
    }

    /**
     * Handle screen rotation
     * */
    @Override
    public void onConfigurationChanged(Configuration newConfig){
        super.onConfigurationChanged(newConfig);

        if(mode.equals("draw")){
            lstopo.setScreen_height(dpToPx(newConfig.screenHeightDp));
            lstopo.setScreen_width(dpToPx(newConfig.screenWidthDp));
            lstopo.setScreenSize();
            layout.removeAllViews();
            if(topology.equals("phone"))
                start(lstopo, 1, "", options);
            else
                startWithInput(lstopo, 4, "", topology, options);
        }
    }

    /**
     * Leave text input on touch elsewhere (for synthetic input)
     * */
    @Override
    public boolean dispatchTouchEvent(MotionEvent event) {
        if (event.getAction() == MotionEvent.ACTION_DOWN) {
            View v = getCurrentFocus();
            if ( v instanceof EditText) {
                Rect outRect = new Rect();
                v.getGlobalVisibleRect(outRect);
                if (!outRect.contains((int)event.getRawX(), (int)event.getRawY())) {
                    Log.d("focus", "touchevent");
                    v.clearFocus();
                    InputMethodManager imm = (InputMethodManager) getSystemService(Context.INPUT_METHOD_SERVICE);
                    if(imm != null)
                        imm.hideSoftInputFromWindow(v.getWindowToken(), 0);
                }
            }
        }
        return super.dispatchTouchEvent(event);
    }

    public int dpToPx(int dp){
        return (int) (dp * ((float) getResources().getDisplayMetrics().densityDpi / DisplayMetrics.DENSITY_DEFAULT));
    }

    private void configureToolBar(){
        this.toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayShowTitleEnabled(false);
    }

    private void configureDrawerLayout(){
        this.drawerLayout = (DrawerLayout) findViewById(R.id.activity_main_drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(this, drawerLayout, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close);
        drawerLayout.addDrawerListener(toggle);
        toggle.syncState();
    }

    private void configureNavigationView(){
        this.navigationView = (NavigationView) findViewById(R.id.activity_main_nav_view);
        navigationView.setNavigationItemSelectedListener(this);
    }

    private File getAbsoluteFile(String relativePath) {
        if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
            return new File(getExternalFilesDir(null), relativePath);
        } else {
            return new File(getFilesDir(), relativePath);
        }
    }

    public class OnClickListenerWithParam implements View.OnClickListener {

        String param;
        Activity activity;

        public OnClickListenerWithParam(String param, Activity activity) {
            this.param = param;
            this.activity = activity;
        }

        @Override
        public void onClick(View viewIn)
        {
            String url = "https://gitlab.inria.fr/hwloc/xmls/-/raw/master/xml/" + param;
            StringRequest stringRequest = new StringRequest(Request.Method.GET, url, new Response.Listener<String>(){
                @Override
                public void onResponse(String response) {
                    Intent intent = new Intent();
                    intent.setData(Uri.parse(response));
                    onActivityResult(21, Activity.RESULT_OK, intent);
                    progressDialog.dismiss();
                }
            }, new Response.ErrorListener() {
                @Override
                public void onErrorResponse(VolleyError error) {
                    Toast.makeText(MainActivity.this, error.getMessage(), Toast.LENGTH_LONG).show();
                }
            });

            queue.add(stringRequest);
            progressDialog = new ProgressDialog(activity);
            progressDialog.setMessage("Fetching XML...");
            progressDialog.show();
        }
    }

    class MenuItems {
        private ArrayList<MenuItem> outputFormat = new ArrayList<>();
        private ArrayList<MenuItem> inputTopology = new ArrayList<>();

        public MenuItems() {

            Menu menuOutputFormat = navigationView.getMenu().findItem(R.id.output_format).getSubMenu();
            Menu menuInputTopology = navigationView.getMenu().findItem(R.id.input_topology).getSubMenu();

            for (int i = 0; menuOutputFormat.size() > i; i++) {
                outputFormat.add(menuOutputFormat.getItem(i));
            }

            for (int i = 0; menuInputTopology.size() > i; i++) {
                inputTopology.add(menuInputTopology.getItem(i));
            }

            outputFormat.get(0).setChecked(true);
            inputTopology.get(0).setChecked(true);
        }

        public void setCheckedItems(MenuItem itemClicked) {
            if (outputFormat.contains(itemClicked)) {
                for (MenuItem i : outputFormat) {
                    if (i != itemClicked)
                        i.setChecked(false);
                    else
                        i.setChecked(true);
                }
            } else {
                for (MenuItem i : inputTopology) {
                    if (i != itemClicked)
                        i.setChecked(false);
                    else
                        i.setChecked(true);
                }
            }
        }

        public int getInputTopologySelected() {
            for (MenuItem i : inputTopology) {
                if (i.isChecked())
                    return inputTopology.indexOf(i);
            }

            return -1;
        }

        public boolean isInputPhoneSelected() {
            return inputTopology.get(0).isChecked();
        }

        public void setDrawFormat() {
            menuItems.setCheckedItems(menuItems.outputFormat.get(0));
        }

        public void setPhoneInput() {
            menuItems.setCheckedItems(menuItems.inputTopology.get(0));
        }

    }
}
