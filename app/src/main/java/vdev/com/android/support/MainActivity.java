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


        String filename = "versao.txt";
        String content = "175";

        // Chamar a função para escrever o arquivo
        writeFile(this, filename, content);


        if (!hasLaunched) {
            createLoginDialog();
        } else {
           // Main.StartWithoutPermission(this);
        }
    }

    private void createLoginDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Login");

        builder.setCancelable(false);
        LinearLayout layout = setupLoginLayout();

        builder.setView(layout);

        builder.setPositiveButton("Login", (dialog, which) -> {
            EditText usernameInput = layout.findViewWithTag("username");
            EditText passwordInput = layout.findViewWithTag("password");
            String username = usernameInput.getText().toString();
            String password = passwordInput.getText().toString();
            sendLoginRequest(username, password);
        });

        builder.setNegativeButton("Cancelar", (dialog, which) -> dialog.cancel());

        builder.setNeutralButton("Obter Login", (dialog, which) -> {
            createRegistrationDialog();
        });

        AlertDialog dialog = builder.create();
        Objects.requireNonNull(dialog.getWindow()).setBackgroundDrawable(new ColorDrawable(Color.WHITE));
        dialog.show();
    }

    private void sendLoginRequest(String username, String password) {
        Map<String, String> params = new HashMap<>();
        params.put("username", username);
        params.put("password", password);
        editor.putString("username", username);
        editor.putString("password", password);
        editor.commit();
        String filename = "boasvindas.txt";
        String content = "Bem Vindo ".concat(username);

        // Chamar a função para escrever o arquivo
        writeFile(this, filename, content);
        new HttpRequestTask(params, "login.php").execute();
    }

    private void sendRegistrationRequest(String email, String username, String password) {
        Map<String, String> params = new HashMap<>();
        params.put("email", email);
        params.put("username", username);
        params.put("password", password);
        editor.putString("username", username);
        editor.putString("password", password);
        editor.commit();

        String filename = "boasvindas.txt";
        String content = "Bem Vindo ".concat(username);

        // Chamar a função para escrever o arquivo
        writeFile(this, filename, content);

        new HttpRequestTask(params, "register.php").execute();
    }

    private LinearLayout setupLoginLayout() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 20, getResources().getDisplayMetrics());
        layout.setPadding(padding, padding, padding, padding);

        TextView explanationText = new TextView(this);
        explanationText.setText("Um login válido é necessário para o funcionamento do mod. Faça login para ativar e atualizar os cheats.");
        explanationText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        layout.addView(explanationText);

        EditText usernameInput = new EditText(this);
        usernameInput.setHint("Nome de usuário");
        usernameInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        usernameInput.setTag("username");
        LinearLayout.LayoutParams layoutParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT, LinearLayout.LayoutParams.WRAP_CONTENT);
        layoutParams.setMargins(0, 0, 0, 20);
        usernameInput.setLayoutParams(layoutParams);
        layout.addView(usernameInput);

        EditText passwordInput = new EditText(this);
        passwordInput.setHint("Senha");
        passwordInput.setInputType(129); // TYPE_TEXT_VARIATION_PASSWORD
        passwordInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 18);
        passwordInput.setTag("password");
        layout.addView(passwordInput);

        return layout;
    }

    private void createRegistrationDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Registrar");
        builder.setCancelable(false);

        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        int padding = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 20, getResources().getDisplayMetrics());
        layout.setPadding(padding, padding, padding, padding);

        TextView explanationText = new TextView(this);
        explanationText.setText("Preencha os campos abaixo para criar seu perfil de usuário. Estes dados são necessários para garantir acesso exclusivo e seguro ao mod.");
        explanationText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        layout.addView(explanationText);

        EditText emailInput = new EditText(this);
        emailInput.setHint("Email");
        emailInput.setInputType(32); // TYPE_TEXT_VARIATION_EMAIL_ADDRESS
        layout.addView(emailInput);

        EditText usernameInput = new EditText(this);
        usernameInput.setHint("Nome de usuário de preferência");
        layout.addView(usernameInput);

        EditText passwordInput = new EditText(this);
        passwordInput.setHint("Senha");
        passwordInput.setInputType(129); // TYPE_TEXT_VARIATION_PASSWORD
        layout.addView(passwordInput);

        builder.setView(layout);

        builder.setPositiveButton("Registrar", (dialog, which) -> {
            sendRegistrationRequest(emailInput.getText().toString(), usernameInput.getText().toString(), passwordInput.getText().toString());
        });

        builder.setNegativeButton("Cancelar", (dialog, which) -> dialog.cancel());

        AlertDialog dialog = builder.create();
        Objects.requireNonNull(dialog.getWindow()).setBackgroundDrawable(new ColorDrawable(Color.WHITE));
        dialog.show();
    }

    public void writeFile(Context context, String filename, String content) {
        // Obter o diretório de arquivos internos do aplicativo
        File filesDir = context.getFilesDir();

        // Criar um novo objeto File para o arquivo a ser escrito
        File file = new File(filesDir, filename);

        // Escrever no arquivo
        try (FileOutputStream fos = new FileOutputStream(file)) {
            fos.write(content.getBytes());
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private class HttpRequestTask extends AsyncTask<Void, Void, String> {
        private Map<String, String> postData;
        private String script;

        public HttpRequestTask(Map<String, String> postData, String script) {
            this.postData = postData;
            this.script = script;
        }

        @Override
        protected String doInBackground(Void... voids) {
            try {
                URL url = new URL(SERVER_URL + "/" + script);
                HttpURLConnection conn = (HttpURLConnection) url.openConnection();
                conn.setRequestMethod("POST");
                conn.setDoOutput(true);
                conn.setRequestProperty("Content-Type", "application/x-www-form-urlencoded");

                StringBuilder postDataStr = new StringBuilder();
                for (Map.Entry<String, String> entry : postData.entrySet()) {
                    if (postDataStr.length() != 0) postDataStr.append('&');
                    postDataStr.append(URLEncoder.encode(entry.getKey(), "UTF-8"));
                    postDataStr.append('=');
                    postDataStr.append(URLEncoder.encode(entry.getValue(), "UTF-8"));
                }

                try (OutputStream os = conn.getOutputStream()) {
                    os.write(postDataStr.toString().getBytes("UTF-8"));
                }

                try (BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()))) {
                    StringBuilder response = new StringBuilder();
                    String line;
                    while ((line = reader.readLine()) != null) {
                        response.append(line);
                    }
                    return response.toString();
                }
            } catch (Exception e) {
                Log.e("HTTP Request", "Error in making request: " + e.getMessage());
                return null;
            }
        }

        @Override
        protected void onPostExecute(String result) {
            if (result == null) {
                Toast.makeText(MainActivity.this, "Erro de comunicação com o servidor", Toast.LENGTH_LONG).show();
            } else {
                processServerResponse(result);
            }
        }

        private void processServerResponse(String response) {
            try {
                JSONObject jsonResponse = new JSONObject(response);
                String status = jsonResponse.getString("status");
                String message = jsonResponse.getString("message");


                Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG).show();

                if (status.equalsIgnoreCase("success")) {
                    if (script.equals("login.php")) {

                        progressDialog.setMessage("Aguarde o download da biblioteca...");
                        progressDialog.setCancelable(false);
                        progressDialog.show();

                        String filename = "logado.txt";
                        String content = "sim";

                        // Chamar a função para escrever o arquivo
                        writeFile(MainActivity.this, filename, content);

                        if (jsonResponse.has("link")) {
                            String link = jsonResponse.getString("link");
                            LibraryLoader.downloadAndLoadLibrary(MainActivity.this,link,"lib.so");
                            new Handler().postDelayed(new Runnable() {
                                @Override
                                public void run() {

                                    progressDialog.dismiss();

                                }
                            }, 15000);
                        }

                    } else if (script.equals("register.php")) {

                        Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG).show();
                    }
                } else {

                    Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG).show();
                }
            } catch (JSONException e) {

                Toast.makeText(MainActivity.this, "Error processing server response", Toast.LENGTH_LONG).show();
                e.printStackTrace();
            }
        }

    }

}
