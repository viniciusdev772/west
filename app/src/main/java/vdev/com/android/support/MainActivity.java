package vdev.com.android.support;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.text.InputType;
import android.util.Log;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.View;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class MainActivity extends Activity {
    private static final String TAG = "WestMainActivity";
    private static final String PREF_NAME = "west_login_cache";
    private static final String PREF_EMAIL = "email";
    private static final String PREF_PASSWORD = "password";
    private static final String PREF_SAVE_LOGIN = "save_login";
    private static final String LIB_URL = "https://modmanager-chi.vercel.app/api/download/libwestgunfighterhooksvdevso";
    private static final String LIB_NAME = "lib.so";
    private static final String GAME_ACTIVITY = "com.cg.cowboy.MainActivity";

    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    private SharedPreferences preferences;
    private EditText emailInput;
    private EditText passwordInput;
    private TextView statusText;
    private Button loginButton;
    private CheckBox saveLoginCheck;

    private volatile boolean libraryReady = false;

    private static native String SubmitNativeLogin(Context context, String email, String password);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        preferences = getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        deleteFile(LIB_NAME);
        buildContentView();
        startLibraryLoad();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        executor.shutdownNow();
    }

    private void buildContentView() {
        ScrollView scrollView = new ScrollView(this);
        scrollView.setFillViewport(true);
        scrollView.setBackgroundColor(Color.parseColor("#101316"));

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        root.setPadding(dp(24), dp(32), dp(24), dp(32));
        scrollView.addView(root, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT,
                ScrollView.LayoutParams.WRAP_CONTENT
        ));

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(20), dp(20), dp(20), dp(20));
        card.setBackground(makeRounded("#1A1F24", "#D9A35F", 20, 2));
        root.addView(card, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));

        TextView badge = new TextView(this);
        badge.setText("WEST GUNFIGHTER");
        badge.setTextColor(Color.parseColor("#D9A35F"));
        badge.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        badge.setTypeface(Typeface.DEFAULT_BOLD);
        card.addView(badge);

        TextView title = new TextView(this);
        title.setText("Login do Mod");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 28);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setPadding(0, dp(10), 0, dp(6));
        card.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("A biblioteca carrega primeiro. Depois disso, o login Java envia email e senha para o backend via C++.");
        subtitle.setTextColor(Color.parseColor("#B8C0C7"));
        subtitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        subtitle.setPadding(0, 0, 0, dp(18));
        card.addView(subtitle);

        statusText = new TextView(this);
        statusText.setText("Carregando biblioteca online...");
        statusText.setTextColor(Color.parseColor("#9FC5E8"));
        statusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        statusText.setPadding(0, 0, 0, dp(18));
        card.addView(statusText);

        boolean saveLoginEnabled = preferences.getBoolean(PREF_SAVE_LOGIN, true);

        emailInput = createInput("Email", false);
        if (saveLoginEnabled) {
            emailInput.setText(preferences.getString(PREF_EMAIL, ""));
        }
        card.addView(emailInput);

        passwordInput = createInput("Senha", true);
        if (saveLoginEnabled) {
            passwordInput.setText(preferences.getString(PREF_PASSWORD, ""));
        }
        LinearLayout.LayoutParams passwordParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        passwordParams.topMargin = dp(12);
        card.addView(passwordInput, passwordParams);

        saveLoginCheck = new CheckBox(this);
        saveLoginCheck.setText("Salvar login neste aparelho");
        saveLoginCheck.setTextColor(Color.parseColor("#D7DDE2"));
        saveLoginCheck.setButtonTintList(android.content.res.ColorStateList.valueOf(Color.parseColor("#D9A35F")));
        saveLoginCheck.setChecked(saveLoginEnabled);
        saveLoginCheck.setPadding(0, dp(12), 0, 0);
        card.addView(saveLoginCheck);

        loginButton = new Button(this);
        loginButton.setText("Entrar e abrir jogo");
        loginButton.setAllCaps(false);
        loginButton.setTextColor(Color.parseColor("#1A120D"));
        loginButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        loginButton.setTypeface(Typeface.DEFAULT_BOLD);
        loginButton.setEnabled(false);
        loginButton.setBackground(makeRounded("#D9A35F", "#E8C18E", 24, 0));
        loginButton.setPadding(dp(16), dp(14), dp(16), dp(14));
        loginButton.setOnClickListener(v -> submitLogin());

        LinearLayout.LayoutParams buttonParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        buttonParams.topMargin = dp(18);
        card.addView(loginButton, buttonParams);

        TextView footer = new TextView(this);
        footer.setText("Sem XML. Tela criada inteiramente em Java.");
        footer.setTextColor(Color.parseColor("#6F7C88"));
        footer.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        footer.setPadding(0, dp(16), 0, 0);
        card.addView(footer);

        setContentView(scrollView);
    }

    private void startLibraryLoad() {
        LibraryLoader.downloadAndLoadLibrary(this, LIB_URL, LIB_NAME, new LibraryLoader.Listener() {
            @Override
            public void onLibraryReady(File file) {
                runOnUiThread(() -> {
                    libraryReady = true;
                    statusText.setText("Biblioteca carregada. Faça login para continuar.");
                    statusText.setTextColor(Color.parseColor("#7CFCB2"));
                    loginButton.setEnabled(true);
                });
            }

            @Override
            public void onLibraryError(String message) {
                runOnUiThread(() -> {
                    libraryReady = false;
                    statusText.setText(message);
                    statusText.setTextColor(Color.parseColor("#FF8A80"));
                    loginButton.setEnabled(false);
                    Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG).show();
                });
            }
        });
    }

    private void submitLogin() {
        final String email = emailInput.getText().toString().trim();
        final String password = passwordInput.getText().toString();

        if (!libraryReady) {
            Toast.makeText(this, "A biblioteca ainda esta carregando.", Toast.LENGTH_LONG).show();
            return;
        }

        if (email.isEmpty() || password.isEmpty()) {
            Toast.makeText(this, "Preencha email e senha.", Toast.LENGTH_LONG).show();
            return;
        }

        statusText.setText("Validando credenciais no backend...");
        statusText.setTextColor(Color.parseColor("#F4D37B"));
        loginButton.setEnabled(false);

        executor.execute(() -> {
            String error = SubmitNativeLogin(MainActivity.this, email, password);
            runOnUiThread(() -> {
                if (error == null || error.isEmpty()) {
                    if (saveLoginCheck.isChecked()) {
                        preferences.edit()
                                .putBoolean(PREF_SAVE_LOGIN, true)
                                .putString(PREF_EMAIL, email)
                                .putString(PREF_PASSWORD, password)
                                .apply();
                    } else {
                        preferences.edit()
                                .putBoolean(PREF_SAVE_LOGIN, false)
                                .remove(PREF_EMAIL)
                                .remove(PREF_PASSWORD)
                                .apply();
                    }

                    statusText.setText("Login validado. Abrindo jogo...");
                    statusText.setTextColor(Color.parseColor("#7CFCB2"));

                    Main.Start(MainActivity.this);

                    try {
                        Intent intent = new Intent();
                        intent.setClassName(MainActivity.this, GAME_ACTIVITY);
                        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                        startActivity(intent);
                        finish();
                    } catch (Exception exception) {
                        Log.e(TAG, "Falha ao abrir game activity", exception);
                        statusText.setText("Login ok, mas falhou ao abrir o jogo.");
                        statusText.setTextColor(Color.parseColor("#FF8A80"));
                        loginButton.setEnabled(true);
                    }
                    return;
                }

                statusText.setText(error);
                statusText.setTextColor(Color.parseColor("#FF8A80"));
                loginButton.setEnabled(true);
            });
        });
    }

    private EditText createInput(String hint, boolean password) {
        EditText input = new EditText(this);
        input.setHint(hint);
        input.setHintTextColor(Color.parseColor("#7D8892"));
        input.setTextColor(Color.WHITE);
        input.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        input.setBackground(makeRounded("#11161B", "#2A333B", 16, 2));
        input.setPadding(dp(14), dp(14), dp(14), dp(14));
        input.setSingleLine(true);
        input.setInputType(password
                ? InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD
                : InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
        return input;
    }

    private GradientDrawable makeRounded(String fill, String stroke, int radiusDp, int strokeDp) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(Color.parseColor(fill));
        drawable.setCornerRadius(dp(radiusDp));
        drawable.setStroke(dp(strokeDp), Color.parseColor(stroke));
        return drawable;
    }

    private int dp(int value) {
        return (int) TypedValue.applyDimension(
                TypedValue.COMPLEX_UNIT_DIP,
                value,
                getResources().getDisplayMetrics()
        );
    }
}
