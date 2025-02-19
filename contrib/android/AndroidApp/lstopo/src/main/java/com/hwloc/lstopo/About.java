package com.hwloc.lstopo;

import android.app.Activity;
import android.content.Intent;
import android.graphics.Color;
import android.net.Uri;
import android.os.Bundle;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

import static android.view.View.TEXT_ALIGNMENT_CENTER;

public class About extends Activity {

    private final String privacy = "https://people.bordeaux.inria.fr/goglin/hwloc-android-privacy-policy.html";
    private final String github = "https://github.com/open-mpi/hwloc";
    private final String website = "https://www.open-mpi.org/projects/hwloc";
    private final String ci = "https://ci.inria.fr/hwloc/job/extended/job/master";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.about);

        // Set windows size
        DisplayMetrics dm = new DisplayMetrics();
        getWindowManager().getDefaultDisplay().getMetrics(dm);
        int width = dm.widthPixels;
        int height = dm.heightPixels;
        getWindow().setLayout((int) (width * 0.8), (int) (height * 0.5));

        TextView tvAppName = findViewById(R.id.app_name);
        tvAppName.setTextSize(40);

        String versionName = BuildConfig.VERSION_NAME;
        TextView tvVersion = findViewById(R.id.version);
        tvVersion.setText("Version: " + versionName);
        tvVersion.setTextSize(20);

        TextView ppWebsite = findViewById(R.id.privacy);
        ppWebsite.setText("Privacy Policy: ");
        ppWebsite.setTextSize(20);

        TextView ppWebsiteLink = findViewById(R.id.privacy_link);
        ppWebsiteLink.setText(privacy);
        ppWebsiteLink.setTextColor(Color.parseColor("#4295f7"));
        ppWebsiteLink.setTextAlignment(TEXT_ALIGNMENT_CENTER);
        ppWebsiteLink.setTextSize(20);
        ppWebsiteLink.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                goToLink(privacy);
            }
        });

        TextView tvWebsite = findViewById(R.id.open_mpi);
        tvWebsite.setText("Project Website: ");
        tvWebsite.setTextSize(20);

        TextView tvWebsiteLink = findViewById(R.id.open_mpi_link);
        tvWebsiteLink.setText(website);
        tvWebsiteLink.setTextColor(Color.parseColor("#4295f7"));
        tvWebsiteLink.setTextAlignment(TEXT_ALIGNMENT_CENTER);
        tvWebsiteLink.setTextSize(20);
        tvWebsiteLink.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                goToLink(website);
            }
        });

        TextView tvGithub = findViewById(R.id.github);
        tvGithub.setText("GitHub Repository: ");
        tvGithub.setTextSize(20);

        TextView tvGithubLink = findViewById(R.id.github_link);
        tvGithubLink.setText(github);
        tvGithubLink.setTextColor(Color.parseColor("#4295f7"));
        tvGithubLink.setTextAlignment(TEXT_ALIGNMENT_CENTER);
        tvGithubLink.setTextSize(20);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        params.setMargins(0,0,0,dpToPx(30));
        tvGithubLink.setLayoutParams(params);
        tvGithubLink.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                goToLink(github);
            }
        });

        LinearLayout.LayoutParams paramsDebug = new LinearLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        paramsDebug.setMargins(0,0,0,dpToPx(10));
        tvGithubLink.setLayoutParams(paramsDebug);

        TextView tvCi = findViewById(R.id.ci);
        tvCi.setText("CI with latest APKs: ");
        tvCi.setTextSize(20);

        TextView tvCiLink = findViewById(R.id.ci_link);
        tvCiLink.setText(ci);
        tvCiLink.setTextColor(Color.parseColor("#4295f7"));
        tvCiLink.setTextAlignment(TEXT_ALIGNMENT_CENTER);
        tvCiLink.setTextSize(20);
        tvCiLink.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                goToLink(ci);
            }
        });

        Button bClose = findViewById(R.id.close);
        bClose.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                finish();
            }
        });
    }

    void goToLink(String url){
        Uri uri = Uri.parse(url);
        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        startActivity(intent);
    }

    public int dpToPx(int dp){
        return (int) (dp * ((float) getResources().getDisplayMetrics().densityDpi / DisplayMetrics.DENSITY_DEFAULT));
    }
}
