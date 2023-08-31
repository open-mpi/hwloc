package com.hwloc.lstopo;

import android.app.Activity;
import android.content.res.Resources;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.drawable.GradientDrawable;
import android.graphics.PathEffect;
import android.graphics.DashPathEffect;
import android.graphics.Typeface;
import android.os.Environment;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.ScrollView;
import android.widget.TextView;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import androidx.appcompat.app.AppCompatActivity;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;

// Graphic tools to draw a topology
public class Lstopo extends AppCompatActivity {

    private RelativeLayout layout;
    private Activity activity;
    private String currentContent;
    private int screen_height;
    private int screen_width;
    private int hwloc_screen_height;
    private int hwloc_screen_width;
    private float xscale = 1;
    private float yscale = 1;
    private int fontsize;
    private File debugFile;

    public Lstopo(Activity activity) {
        this.activity = activity;
        layout = activity.findViewById(R.id.relative_layout);
        setScreenSize();
        debugFile = getAbsoluteFile("/application_log.txt");
        debugFile.delete();
    }

    /**
     * Draw topology box
     */
    public void box(int r, int g, int b, int x, int y, final int width, final int height, int style, int id, String info){
        LinearLayout view = new LinearLayout(activity);
        view.setOrientation(LinearLayout.VERTICAL);
        view.setId(id);
        layout.addView(view);
        setBoxInfo(view, info);
        setBoxAttributes(view, r, g, b,
                (int) (xscale * x),
                (int) (yscale * y),
                (int) (xscale * width),
                (int) (yscale * height),
                style);

    }

    public void setBoxInfo(LinearLayout view, String info){
        if(info.isEmpty()){
            view.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                }
            });
            return;
        }
        TextView tv = new TextView(activity);
        view.addView(tv);
        tv.setText(info);
        tv.setTextSize((int) (this.fontsize / 1.5));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        params.setMargins((int) (5 * xscale), (int) (1 * yscale), 0, 0);
        tv.setLayoutParams(params);
        tv.setVisibility(GONE);

        view.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                TextView tv = (TextView) ((ViewGroup)v).getChildAt(0);
                int length = ((ViewGroup)v).getChildCount();
                if(tv.getVisibility() == VISIBLE){
                    for(int i = 1 ; i < length ; i ++){
                        ((ViewGroup)v).getChildAt(i).setVisibility(VISIBLE);
                    }
                    tv.setVisibility(GONE);
                } else {
                    for(int i = 1 ; i < length ; i ++){
                        ((ViewGroup)v).getChildAt(i).setVisibility(GONE);
                    }
                    tv.setVisibility(VISIBLE);
                }
            }
        });
    }

    private void setBoxAttributes(View view, int r, int g, int b, int x, int y, int width, int height, int style){
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(width, height);
        view.setY(y);
        view.setX(x);

        GradientDrawable shape = new GradientDrawable();
        shape.setShape(GradientDrawable.RECTANGLE);
        shape.setColor(Color.rgb(r, g, b));
	int linewidth = 2;
	float linedash = 0;
        if(style != 0){
	    linewidth += style;
            linedash = (float)(1<<(2+style));
        }
        shape.setStroke(linewidth, Color.parseColor("#000000"), linedash, linedash);
        view.setBackground(shape);

        view.setLayoutParams(params);
    }

    /**
     * Draw topology text
     */
    public void text(String text, int x, int y, int fontsize, int bold, int outside, int id){
        currentContent = text;

        TextView tv = new TextView(activity);
        tv.setMinWidth(screen_width);
        if( id == -1 ){
            if(text.length() > 100) {
                tv.setTextIsSelectable(true);
                ScrollView scrollView = new ScrollView(activity);
                layout.addView(scrollView);
                scrollView.addView(tv);
                tv.setX(x);
                tv.setY(y);
                tv.setMaxWidth(screen_width);
            } else {
                layout.addView(tv);
                tv.setX(x * xscale);
                tv.setY(y * yscale);
            }
        } else {
            tv.setClickable(false);
            LinearLayout viewGroup = layout.findViewById(id);
	    if (outside != 0) {
		/* Text outside the box (PCI speed or factorization) cannot be in the box view,
		 * or it wouldn't appear. So put it in the entire layout.
		 * We won't be able to hide it on click, but there's nothing to show there on click anyway.
		 */
		layout.addView(tv);
		tv.setX(x * xscale);
		tv.setY(y * yscale);
	    } else {
		viewGroup.addView(tv);
		LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
			LinearLayout.LayoutParams.WRAP_CONTENT,
			LinearLayout.LayoutParams.WRAP_CONTENT
		);
		params.setMargins((int) (10 * xscale), (int) (2 * yscale), 0, 0);
		tv.setLayoutParams(params);
	    }
        }
	if(bold != 0){
	    tv.setTypeface(tv.getTypeface(), Typeface.BOLD);
	}
        tv.setTextSize(this.fontsize);
        tv.setText(text);
    }

    /**
     * Draw topology line
     */
    public void line(int x1, int y1, int x2, int y2){
	int width = x2-x1;
	int height = y2-y1;
	if (width != 0 && height != 0)
	    return; /* not supported yet */
        LinearLayout view = new LinearLayout(activity);
        view.setOrientation(LinearLayout.VERTICAL);
        layout.addView(view);
	if (width == 0)
	    width = 2;
	if (height == 0)
	    height = 2;
        view.setX((int)(xscale * x1)-1);
        view.setY((int)(yscale * y1)-1);
        GradientDrawable shape = new GradientDrawable();
        shape.setShape(GradientDrawable.RECTANGLE);
        shape.setColor(Color.rgb(0, 0, 0));
        view.setBackground(shape);
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams((int)(xscale * width)+2, (int)(yscale * height)+2);
        view.setLayoutParams(params);
    }

    public void setScreenSize(){
        Display display = activity.getWindowManager().getDefaultDisplay();
        Point size = new Point();
        display.getSize(size);
        int toolbars_height = 0;

        // status bar height
        int resourceId = activity.getResources().getIdentifier("status_bar_height", "dimen", "android");
        if (resourceId > 0) {
            toolbars_height += activity.getResources().getDimensionPixelSize(resourceId);
        }

        screen_height = size.y - dpToPx(toolbars_height);
        screen_width = size.x;

        // navigation bar height
        resourceId = activity.getResources().getIdentifier("navigation_bar_height", "dimen", "android");
        if (resourceId > 0) {
            toolbars_height += activity.getResources().getDimensionPixelSize(resourceId);
        }
    }

    public void setScale(int hwloc_screen_height, int hwloc_screen_width, int fontsize){
        this.hwloc_screen_height = hwloc_screen_height;
        this.hwloc_screen_width = hwloc_screen_width;

        yscale = ((float) screen_height / (float) hwloc_screen_height);
        xscale = yscale;

        this.fontsize = (int) pixelsToDp(fontsize * yscale);
        if(this.fontsize > 15)
            this.fontsize = 14;

        layout.setMinimumHeight(screen_height);
    }

    public int getScreen_height() {
        return this.screen_height;
    }

    public int getScreen_width() {
        return this.screen_width;
    }

    public void setScreen_height(int screen_height) {
        this.screen_height = screen_height;
    }

    public void setScreen_width(int screen_width) {
        this.screen_width = screen_width;
    }

    public String getCurrentContent(){
        return this.currentContent;
    }

    public String getDebugFile() {
        return this.debugFile.getAbsolutePath();
    }

    public void clearDebugFile() {
        debugFile.delete();
    }

    public File getAbsoluteFile(String relativePath){
        if (Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState())) {
            return new File(activity.getExternalFilesDir(null), relativePath);
        } else {
            return new File(activity.getFilesDir(), relativePath);
        }
    }

    public void writeDebugFile(String content) {
        try {
            FileWriter writer = new FileWriter(debugFile, true);
            writer.append(content);
            writer.flush();
            writer.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public static float pixelsToDp(float px){
        DisplayMetrics metrics = Resources.getSystem().getDisplayMetrics();
        float dp = px / (metrics.densityDpi / 160f);
        return Math.round(dp);
    }

    public int dpToPx(int dp){
        return (int) (dp * ((float) activity.getResources().getDisplayMetrics().densityDpi / DisplayMetrics.DENSITY_DEFAULT));
    }

}
