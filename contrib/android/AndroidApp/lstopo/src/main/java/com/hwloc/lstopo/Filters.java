// SPDX-License-Identifier: BSD-3-Clause
// Copyright © 2020 Inria.  All rights reserved.
// See COPYING in top-level directory.

package com.hwloc.lstopo;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.Toast;

import com.android.volley.Request;
import com.android.volley.RequestQueue;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.JsonObjectRequest;
import com.android.volley.toolbox.Volley;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

// Create a window to choose the filter to download topology
public class Filters extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.filters);
        final Context activity = this;
        final Spinner architecture = findViewById(R.id.architecture);
        final Spinner sub_architecture = findViewById(R.id.sub_architecture);
        final Spinner uname_architecture = findViewById(R.id.uname_architecture);
        final Spinner core = findViewById(R.id.core);
        final Spinner packages = findViewById(R.id.packages);
        final Spinner cpu_vendor = findViewById(R.id.cpu_vendor);
        final Spinner cpu_family = findViewById(R.id.cpu_family);
        final Spinner NUMA = findViewById(R.id.NUMA);
        final Spinner chassis = findViewById(R.id.chassis);
        final Spinner year = findViewById(R.id.year);
        List<Spinner> tmpList = Arrays.asList(architecture, core, packages, sub_architecture, uname_architecture, NUMA, chassis, cpu_family, year, cpu_vendor);
        final ArrayList<Spinner> selects = new ArrayList<>(tmpList);

        // Set windows size
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        getWindow().setLayout((int) (width * 0.8), (int) (height * 0.6));

        final RequestQueue queue = Volley.newRequestQueue(this);
        final String url = "https://hwloc-xmls.herokuapp.com";
        final String url2 = "https://hwloc-xmls.herokuapp.com/json";

        // Get all topologies
        JsonObjectRequest jsonObjectRequest = new JsonObjectRequest(Request.Method.GET, url, new Response.Listener<JSONObject>() {
            @Override
            public void onResponse(JSONObject response) {
            try {
                final JSONArray xmls = response.getJSONArray("xml");
                // Get a list of all filters available for each Spinner
                JsonObjectRequest jsonObjectRequest = new JsonObjectRequest(Request.Method.GET, url2, new Response.Listener<JSONObject>() {
                    @Override
                    public void onResponse(JSONObject tags) {
                    String option_info;
                    // For each Spinner
                    for( Spinner select : selects ){
                        ArrayList<String> options = new ArrayList<>();
                        // Run through every topology and look for new filter
                        for( int i = 0 ; i < xmls.length() ; i++ ){
                            try {
                                String title = xmls.getJSONObject(i).getString("title").substring(0, xmls.getJSONObject(i).getString("title").length()-4);
                                JSONObject xml_tags = tags.getJSONObject(title);
                                option_info = xml_tags.getString(String.valueOf(select.getTag()));

                                if(String.valueOf(select.getTag()).equals("nbcores"))
                                    option_info = "≥ " + Integer.parseInt(option_info)/10 + "0";

                                if(!options.contains(option_info))
                                    options.add(option_info);

                            } catch (JSONException e) {
                                e.printStackTrace();
                            }
                        }
                        Collections.sort(options);
                        options.add(0, String.valueOf(select.getTag()));
                        // add filters found to the Spinner
                        ArrayAdapter<String> arrayAdapter = new ArrayAdapter<>(activity, android.R.layout.simple_spinner_item, options);
                        arrayAdapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
                        select.setAdapter(arrayAdapter);
                    }
                    }
                }, new Response.ErrorListener() {

                    @Override
                    public void onErrorResponse(VolleyError error) {
                        Toast.makeText(activity, error.getMessage(), Toast.LENGTH_LONG).show();
                    }
                });
                queue.add(jsonObjectRequest);
            } catch (JSONException e) {
                e.printStackTrace();
            }
            }
        }, new Response.ErrorListener() {

            @Override
            public void onErrorResponse(VolleyError error) {
                Toast.makeText(activity, error.getMessage(), Toast.LENGTH_LONG).show();
            }
        });

        queue.add(jsonObjectRequest);

        final EditText title = findViewById(R.id.input);
        Button apply = findViewById(R.id.apply);

        // Create json object containing json filters
        apply.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
            JSONObject json = new JSONObject();
            try {
                json.put("architecture", architecture.getSelectedItemPosition() != 0 ? architecture.getSelectedItem().toString() : null);
                json.put("sub-architecture", sub_architecture.getSelectedItemPosition() != 0 ? sub_architecture.getSelectedItem().toString() : null);
                json.put("uname-architecture", uname_architecture.getSelectedItemPosition() != 0 ? uname_architecture.getSelectedItem().toString() : null);
                json.put("nbcores", core.getSelectedItemPosition() != 0 ? core.getSelectedItem().toString().substring(1) : null);
                json.put("cpu-vendor", cpu_vendor.getSelectedItemPosition() != 0 ? cpu_vendor.getSelectedItem().toString() : null);
                json.put("cpu-family", cpu_family.getSelectedItemPosition() != 0 ? cpu_family.getSelectedItem().toString() : null);
                json.put("nbpackages", packages.getSelectedItemPosition() != 0 ? packages.getSelectedItem().toString() : null);
                json.put("NUMA", NUMA.getSelectedItemPosition() != 0 ? NUMA.getSelectedItem().toString() : null);
                json.put("Chassis", chassis.getSelectedItemPosition() != 0 ? chassis.getSelectedItem().toString() : null);
                json.put("year", year.getSelectedItemPosition() != 0 ? year.getSelectedItem().toString() : null);
                json.put("title", title.getText());
            } catch (JSONException e) {
                e.printStackTrace();
            }

            // Send the filters back to mainActivity
            Intent returnIntent = new Intent();
            returnIntent.setData(Uri.parse(json.toString()));
            setResult(RESULT_OK, returnIntent);
            finish();
            }
        });

    }
}
