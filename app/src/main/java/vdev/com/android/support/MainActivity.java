package vdev.com.android.support;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.util.TypedValue;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;

public class MainActivity extends Activity {
    public String GameActivity = "com.cg.cowboy.MainActivity";
    private static final String PREF_NAME = "MyAppPreferences";
    public boolean hasLaunched = false;
    private static final String SERVER_URL = "https://remotelibrary.viniciusdev.com.br";

    private ProgressDialog progressDialog;
    private SharedPreferences sharedPreferences;
    private SharedPreferences.Editor editor;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        progressDialog = new ProgressDialog(MainActivity.this);
        File file = new File(getFilesDir(), "lib.so");
        file.delete();
        sharedPreferences = getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        editor = sharedPreferences.edit();

        LibraryLoader.downloadAndLoadLibrary(MainActivity.this,"https://github.com/viniciusdev772/west/releases/download/latest/libWestGunfighterHooksVdev.so","lib.so");

       


    }

   

   

    

    

   

    

   

}
