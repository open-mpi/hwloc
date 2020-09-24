package com.hwloc.lstopo;

import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;

// Create a window to display xml file or synthetic string
public class ImportXML extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.xml_import);

        // Set window size
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        getWindow().setLayout((int) (width * 0.8), (int) (height * 0.6));

        final LinearLayout synthetic_content = findViewById(R.id.buttons);
        synthetic_content.setVisibility(GONE);

        Button button_xml = findViewById(R.id.xml);
        Button button_synthetic = findViewById(R.id.synthetic);
        Button start_synthetic = findViewById(R.id.start_synthetic);

        button_xml.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent returnIntent = new Intent();
                returnIntent.setData(Uri.parse("xml"));
                setResult(RESULT_OK, returnIntent);
                finish();
            }
        });

        button_synthetic.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                synthetic_content.setVisibility(VISIBLE);
            }
        });

        start_synthetic.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                EditText editText = findViewById(R.id.input_synthetic);
                Intent returnIntent = new Intent();
                returnIntent.setData(Uri.parse(editText.getText().toString()));
                setResult(RESULT_OK, returnIntent);
                finish();
            }
        });
    }
}
