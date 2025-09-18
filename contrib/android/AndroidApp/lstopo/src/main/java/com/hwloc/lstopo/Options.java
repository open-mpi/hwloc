// SPDX-License-Identifier: BSD-3-Clause
// Copyright © 2020-2023 Inria.  All rights reserved.
// See COPYING in top-level directory.

package com.hwloc.lstopo;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.RadioButton;
import android.widget.RadioGroup;

import java.util.ArrayList;

// Create a window to choose the filter to download topology
public class Options extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.options);
        final Button apply = findViewById(R.id.apply);
        final RadioButton logicalIndexes = findViewById(R.id.indexes_logical);
        final RadioButton physicalIndexes = findViewById(R.id.indexes_physical);
        final RadioButton defaultIndexes = findViewById(R.id.indexes_default);
        final RadioButton ioObjectsWhole = findViewById(R.id.IO_object_whole);
        final RadioButton ioObjectsNone = findViewById(R.id.IO_object_none);
        final RadioButton ioObjectsDefault = findViewById(R.id.IO_object_default);
        final CheckBox noFactorize = findViewById(R.id.no_factorize);
        final CheckBox includeDisallowed = findViewById(R.id.include_disallowed);
        final CheckBox noLegend = findViewById(R.id.no_legend);
        final ArrayList<String> options = new ArrayList<>();

        final RadioGroup indexes1 = findViewById(R.id.indexes_group1);
        final RadioGroup indexes2 = findViewById(R.id.indexes_group2);
        final RadioGroup ioObjects1 = findViewById(R.id.IO_objects_group1);
        final RadioGroup ioObjects2 = findViewById(R.id.IO_objects_group2);
        syncRadioGroup(indexes1, indexes2);
        syncRadioGroup(ioObjects1, ioObjects2);

        Intent intent = getIntent();
        int i = 0;
        while(intent.getStringExtra(Integer.toString(i)) != null) {
            switch(intent.getStringExtra(Integer.toString(i))) {
                case "-l":
                    logicalIndexes.setChecked(true);
                    break;
                case "-p":
                    physicalIndexes.setChecked(true);
                    break;

                case "--no-io":
                    ioObjectsNone.setChecked(true);
                    break;
                case "--whole-io":
                    ioObjectsWhole.setChecked(true);
                    break;

                case "--no-factorize":
                    noFactorize.setChecked(true);
                    break;

                case "--disallowed":
                    includeDisallowed.setChecked(true);
                    break;

                case "--no-legend":
                    noLegend.setChecked(true);
                    break;
            }
            i++;
        }

        // Set windows size
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        getWindow().setLayout((int) (width * 0.8), (int) (height * 0.8));

        apply.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if(logicalIndexes.isChecked())
                    options.add("-l");
                else if(physicalIndexes.isChecked())
                    options.add("-p");

                if(ioObjectsNone.isChecked())
                    options.add("--no-io");
                else if(ioObjectsWhole.isChecked())
                    options.add("--whole-io");

                if(noFactorize.isChecked()){
                    options.add("--no-factorize");
                    options.add("--no-collapse");
		}
                if(includeDisallowed.isChecked())
                    options.add("--disallowed");
                if(noLegend.isChecked())
                    options.add("--no-legend");

                // Send the options back to mainActivity
                Intent returnIntent = new Intent();
                returnIntent.putStringArrayListExtra("options", options);
                setResult(RESULT_OK, returnIntent);
                finish();
            }
        });

        if(!logicalIndexes.isChecked() && !physicalIndexes.isChecked())
            defaultIndexes.setChecked(true);
        if(!ioObjectsNone.isChecked() && !ioObjectsWhole.isChecked())
            ioObjectsDefault.setChecked(true);
    }

    private void syncRadioGroup(final RadioGroup radioGroup1, final RadioGroup radioGroup2) {
        radioGroup1.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int i) {
                RadioButton rButton = ((RadioButton) radioGroup.findViewById(i));
                if (rButton != null && rButton.isChecked())
                    radioGroup2.clearCheck();
            }
        });

        radioGroup2.setOnCheckedChangeListener(new RadioGroup.OnCheckedChangeListener() {
            @Override
            public void onCheckedChanged(RadioGroup radioGroup, int i) {
                RadioButton rButton = ((RadioButton) radioGroup.findViewById(i));
                if (rButton != null && rButton.isChecked())
                    radioGroup1.clearCheck();
            }
        });
    }

}
