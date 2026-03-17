package vdev.com.android.support;

import android.app.AlertDialog;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
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
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.File;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.FileInputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.security.MessageDigest;
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
    private static final String MOD_INFO_URL = "https://vinaomods.vercel.app/api/mods/by-package";
    private static final String MOD_DETAILS_BASE_URL = "https://vinaomods.online/details?slug=";
    private static final String MOD_LOOKUP_PACKAGE = "com.cg.cowboy";

    private final ExecutorService executor = Executors.newSingleThreadExecutor();

    private SharedPreferences preferences;
    private EditText emailInput;
    private EditText passwordInput;
    private TextView statusText;
    private TextView libraryReadyPill;
    private TextView inlineErrorText;
    private LinearLayout statusCard;
    private Button loginButton;
    private Button passwordToggleButton;
    private Button infoButton;
    private Button detailsButton;
    private CheckBox saveLoginCheck;

    private volatile boolean libraryReady = false;
    private boolean passwordVisible = false;
    private boolean loginInFlight = false;
    private boolean modInfoDialogShown = false;
    private boolean apkUpdateDialogShown = false;
    private JSONObject cachedModInfo = null;
    private GradientDrawable emailBackground;
    private GradientDrawable passwordBackground;

    private static native String SubmitNativeLogin(Context context, String email, String password);
    private static native String GetNativeLoginSummary();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        preferences = getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
        deleteFile(LIB_NAME);
        buildContentView();
        startLibraryLoad();
        startModInfoLoad();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        executor.shutdownNow();
    }

    private void buildContentView() {
        FrameLayout frame = new FrameLayout(this);
        frame.setBackground(makeVerticalGradient("#0B0E12", "#131920"));

        View warmGlow = new View(this);
        warmGlow.setBackground(makeVerticalGradient("#59F0B35A", "#00F0B35A"));
        FrameLayout.LayoutParams glowParams = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                dp(240)
        );
        glowParams.gravity = Gravity.TOP;
        frame.addView(warmGlow, glowParams);

        View sideGlow = new View(this);
        sideGlow.setBackground(makeVerticalGradient("#309FC5E8", "#009FC5E8"));
        FrameLayout.LayoutParams sideGlowParams = new FrameLayout.LayoutParams(
                dp(180),
                dp(320)
        );
        sideGlowParams.gravity = Gravity.END | Gravity.TOP;
        sideGlowParams.topMargin = dp(80);
        frame.addView(sideGlow, sideGlowParams);

        ScrollView scrollView = new ScrollView(this);
        scrollView.setFillViewport(true);
        scrollView.setBackgroundColor(Color.TRANSPARENT);
        frame.addView(scrollView, new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.MATCH_PARENT,
                FrameLayout.LayoutParams.MATCH_PARENT
        ));

        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        root.setPadding(dp(22), dp(28), dp(22), dp(28));
        scrollView.addView(root, new ScrollView.LayoutParams(
                ScrollView.LayoutParams.MATCH_PARENT,
                ScrollView.LayoutParams.WRAP_CONTENT
        ));

        TextView eyebrow = new TextView(this);
        eyebrow.setText("VinaoMods Access");
        eyebrow.setTextColor(Color.parseColor("#F0B35A"));
        eyebrow.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        eyebrow.setTypeface(Typeface.DEFAULT_BOLD);
        eyebrow.setLetterSpacing(0.08f);
        eyebrow.setPadding(dp(4), dp(4), dp(4), dp(14));
        root.addView(eyebrow);

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(22), dp(22), dp(22), dp(22));
        card.setBackground(makeRounded("#171C21", "#44F0B35A", 28, 1));
        card.setAlpha(0f);
        card.setTranslationY(dp(24));
        root.addView(card, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));

        TextView badge = new TextView(this);
        badge.setText("WEST GUNFIGHTER");
        badge.setTextColor(Color.parseColor("#F0B35A"));
        badge.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        badge.setTypeface(Typeface.DEFAULT_BOLD);
        badge.setBackground(makeRounded("#221811", "#59F0B35A", 14, 1));
        badge.setPadding(dp(10), dp(6), dp(10), dp(6));
        card.addView(badge);

        TextView title = new TextView(this);
        title.setText("Entrar para liberar o mod");
        title.setTextColor(Color.WHITE);
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 30);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setPadding(0, dp(14), 0, dp(6));
        card.addView(title);

        TextView subtitle = new TextView(this);
        subtitle.setText("Seu acesso e validado no aparelho antes da abertura do jogo. Use sua conta VIP para continuar.");
        subtitle.setTextColor(Color.parseColor("#B8C0C7"));
        subtitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        subtitle.setLineSpacing(dp(2), 1.0f);
        subtitle.setPadding(0, 0, 0, dp(18));
        card.addView(subtitle);

        libraryReadyPill = new TextView(this);
        libraryReadyPill.setText("Biblioteca offline");
        libraryReadyPill.setTextColor(Color.parseColor("#D7DDE2"));
        libraryReadyPill.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        libraryReadyPill.setTypeface(Typeface.DEFAULT_BOLD);
        libraryReadyPill.setBackground(makeRounded("#201715", "#4A2F29", 14, 1));
        libraryReadyPill.setPadding(dp(10), dp(7), dp(10), dp(7));
        LinearLayout.LayoutParams pillParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        pillParams.bottomMargin = dp(18);
        card.addView(libraryReadyPill, pillParams);

        View divider = new View(this);
        divider.setBackgroundColor(Color.parseColor("#2A333B"));
        LinearLayout.LayoutParams dividerParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                dp(1)
        );
        dividerParams.bottomMargin = dp(18);
        card.addView(divider, dividerParams);

        statusCard = new LinearLayout(this);
        statusCard.setOrientation(LinearLayout.VERTICAL);
        statusCard.setPadding(dp(14), dp(12), dp(14), dp(12));
        statusCard.setBackground(makeRounded("#12202B", "#284254", 18, 1));
        card.addView(statusCard, new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        ));

        TextView statusLabel = new TextView(this);
        statusLabel.setText("Status");
        statusLabel.setTextColor(Color.parseColor("#8AA6BC"));
        statusLabel.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        statusLabel.setTypeface(Typeface.DEFAULT_BOLD);
        statusLabel.setLetterSpacing(0.06f);
        statusCard.addView(statusLabel);

        statusText = new TextView(this);
        statusText.setTextColor(Color.parseColor("#9FC5E8"));
        statusText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        statusText.setLineSpacing(dp(2), 1.0f);
        statusText.setPadding(0, dp(6), 0, 0);
        statusCard.addView(statusText);
        setStatus("Carregando biblioteca protegida...", "#9FC5E8", "#12202B", "#284254");

        boolean saveLoginEnabled = preferences.getBoolean(PREF_SAVE_LOGIN, true);

        addFieldLabel(card, "Email da conta");
        emailInput = createInput("Digite seu email", false);
        emailBackground = (GradientDrawable) emailInput.getBackground().mutate();
        if (saveLoginEnabled) {
            emailInput.setText(preferences.getString(PREF_EMAIL, ""));
        }
        LinearLayout.LayoutParams emailParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        emailParams.topMargin = dp(10);
        card.addView(emailInput, emailParams);

        addFieldLabel(card, "Senha");
        passwordInput = createInput("Digite sua senha", true);
        passwordBackground = (GradientDrawable) passwordInput.getBackground().mutate();
        if (saveLoginEnabled) {
            passwordInput.setText(preferences.getString(PREF_PASSWORD, ""));
        }

        LinearLayout passwordRow = new LinearLayout(this);
        passwordRow.setOrientation(LinearLayout.HORIZONTAL);
        passwordRow.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams passwordRowParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        passwordRowParams.topMargin = dp(10);
        card.addView(passwordRow, passwordRowParams);

        LinearLayout.LayoutParams passwordInputParams = new LinearLayout.LayoutParams(
                0,
                LinearLayout.LayoutParams.WRAP_CONTENT,
                1.0f
        );
        passwordRow.addView(passwordInput, passwordInputParams);

        passwordToggleButton = new Button(this);
        passwordToggleButton.setText("Mostrar");
        passwordToggleButton.setAllCaps(false);
        passwordToggleButton.setTextColor(Color.parseColor("#E7BE7A"));
        passwordToggleButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        passwordToggleButton.setTypeface(Typeface.DEFAULT_BOLD);
        passwordToggleButton.setBackground(makeRounded("#18120C", "#6B4A24", 16, 1));
        passwordToggleButton.setPadding(dp(14), dp(13), dp(14), dp(13));
        passwordToggleButton.setOnClickListener(v -> togglePasswordVisibility());
        LinearLayout.LayoutParams toggleParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.WRAP_CONTENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        toggleParams.leftMargin = dp(10);
        passwordRow.addView(passwordToggleButton, toggleParams);

        inlineErrorText = new TextView(this);
        inlineErrorText.setTextColor(Color.parseColor("#FF8A80"));
        inlineErrorText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        inlineErrorText.setLineSpacing(dp(2), 1.0f);
        inlineErrorText.setVisibility(View.GONE);
        LinearLayout.LayoutParams inlineErrorParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        inlineErrorParams.topMargin = dp(10);
        card.addView(inlineErrorText, inlineErrorParams);

        saveLoginCheck = new CheckBox(this);
        saveLoginCheck.setText("Salvar login neste aparelho");
        saveLoginCheck.setTextColor(Color.parseColor("#D7DDE2"));
        saveLoginCheck.setButtonTintList(android.content.res.ColorStateList.valueOf(Color.parseColor("#F0B35A")));
        saveLoginCheck.setChecked(saveLoginEnabled);
        saveLoginCheck.setPadding(0, dp(14), 0, 0);
        card.addView(saveLoginCheck);

        loginButton = new Button(this);
        loginButton.setText("Entrar");
        loginButton.setAllCaps(false);
        loginButton.setTextColor(Color.parseColor("#1A120D"));
        loginButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 16);
        loginButton.setTypeface(Typeface.DEFAULT_BOLD);
        loginButton.setEnabled(false);
        loginButton.setAlpha(0.55f);
        loginButton.setBackground(makeVerticalGradient("#F0B35A", "#D99545"));
        loginButton.setPadding(dp(16), dp(16), dp(16), dp(16));
        loginButton.setOnClickListener(v -> submitLogin());

        LinearLayout.LayoutParams buttonParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        buttonParams.topMargin = dp(20);
        card.addView(loginButton, buttonParams);

        infoButton = new Button(this);
        infoButton.setText("Ver novidades");
        infoButton.setAllCaps(false);
        infoButton.setTextColor(Color.parseColor("#D7DDE2"));
        infoButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        infoButton.setTypeface(Typeface.DEFAULT_BOLD);
        infoButton.setBackground(makeRounded("#11161B", "#2F3943", 18, 1));
        infoButton.setPadding(dp(14), dp(14), dp(14), dp(14));
        infoButton.setEnabled(false);
        infoButton.setAlpha(0.55f);
        infoButton.setOnClickListener(v -> {
            if (cachedModInfo != null) {
                showModInfoDialog(cachedModInfo);
                return;
            }
            startModInfoLoad();
        });

        LinearLayout.LayoutParams infoButtonParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        infoButtonParams.topMargin = dp(10);
        card.addView(infoButton, infoButtonParams);

        detailsButton = new Button(this);
        detailsButton.setText("Abrir página do mod");
        detailsButton.setAllCaps(false);
        detailsButton.setTextColor(Color.parseColor("#D7DDE2"));
        detailsButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        detailsButton.setTypeface(Typeface.DEFAULT_BOLD);
        detailsButton.setBackground(makeRounded("#11161B", "#2F3943", 18, 1));
        detailsButton.setPadding(dp(14), dp(14), dp(14), dp(14));
        detailsButton.setEnabled(false);
        detailsButton.setAlpha(0.55f);
        detailsButton.setOnClickListener(v -> openModDetailsPage());

        LinearLayout.LayoutParams detailsButtonParams = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        detailsButtonParams.topMargin = dp(10);
        card.addView(detailsButton, detailsButtonParams);

        TextView helper = new TextView(this);
        helper.setText("Depois da validacao, o carregamento do overlay e encaminhado direto ao jogo.");
        helper.setTextColor(Color.parseColor("#6F7C88"));
        helper.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
        helper.setLineSpacing(dp(2), 1.0f);
        helper.setPadding(0, dp(14), 0, 0);
        card.addView(helper);

        bindInputFocus(emailInput, emailBackground);
        bindInputFocus(passwordInput, passwordBackground);
        card.animate()
                .alpha(1f)
                .translationY(0f)
                .setDuration(320)
                .start();

        setContentView(frame);
    }

    private void startLibraryLoad() {
        LibraryLoader.downloadAndLoadLibrary(this, LIB_URL, LIB_NAME, new LibraryLoader.Listener() {
            @Override
            public void onLibraryReady(File file) {
                runOnUiThread(() -> {
                    libraryReady = true;
                    libraryReadyPill.setText("Biblioteca pronta");
                    libraryReadyPill.setTextColor(Color.parseColor("#7CFCB2"));
                    libraryReadyPill.setBackground(makeRounded("#10251B", "#225235", 14, 1));
                    setStatus("Biblioteca carregada. Faça login para continuar.", "#7CFCB2", "#10251B", "#225235");
                    setLoginButtonState(true, "Entrar");
                });
            }

            @Override
            public void onLibraryError(String message) {
                runOnUiThread(() -> {
                    libraryReady = false;
                    libraryReadyPill.setText("Falha ao carregar");
                    libraryReadyPill.setTextColor(Color.parseColor("#FF8A80"));
                    libraryReadyPill.setBackground(makeRounded("#2B1517", "#5C2B31", 14, 1));
                    setStatus(message, "#FF8A80", "#2B1517", "#5C2B31");
                    setLoginButtonState(false, "Entrar");
                    Toast.makeText(MainActivity.this, message, Toast.LENGTH_LONG).show();
                });
            }
        });
    }

    private void startModInfoLoad() {
        executor.execute(() -> {
            try {
                String requestUrl = MOD_INFO_URL + "?packageName=" + URLEncoder.encode(MOD_LOOKUP_PACKAGE, "UTF-8");
                HttpURLConnection connection = (HttpURLConnection) new URL(requestUrl).openConnection();
                connection.setRequestMethod("GET");
                connection.setConnectTimeout(8000);
                connection.setReadTimeout(8000);
                connection.setRequestProperty("accept", "application/json");

                int statusCode = connection.getResponseCode();
                InputStream stream = statusCode >= 200 && statusCode < 300
                        ? connection.getInputStream()
                        : connection.getErrorStream();
                String payload = readStream(stream);
                if (statusCode < 200 || statusCode >= 300) {
                    Log.w(TAG, "Falha ao buscar resumo do mod: " + statusCode + " " + payload);
                    return;
                }

                JSONObject root = new JSONObject(payload);
                if (!root.optBoolean("ok")) {
                    return;
                }

                JSONObject mod = root.optJSONObject("mod");
                if (mod == null) {
                    return;
                }

                runOnUiThread(() -> {
                    cachedModInfo = mod;
                    infoButton.setEnabled(true);
                    infoButton.setAlpha(1.0f);
                    detailsButton.setEnabled(true);
                    detailsButton.setAlpha(1.0f);
                    maybeShowApkUpdateDialog(mod);
                    if (!modInfoDialogShown) {
                        modInfoDialogShown = true;
                        showModInfoDialog(mod);
                    }
                });
            } catch (Exception exception) {
                Log.w(TAG, "Falha ao carregar novidades do mod", exception);
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
            showInlineError("Preencha email e senha para continuar.");
            Toast.makeText(this, "Preencha email e senha.", Toast.LENGTH_LONG).show();
            return;
        }

        loginInFlight = true;
        clearInlineError();
        setStatus("Validando credenciais no backend...", "#F4D37B", "#2A2411", "#5E5130");
        setLoginButtonState(false, "Validando...");

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

                    setStatus("Login validado. Abrindo jogo...", "#7CFCB2", "#10251B", "#225235");
                    loginInFlight = false;
                    showLoginSuccessDialog();
                    return;
                }

                loginInFlight = false;
                showInlineError(error);
                setStatus(error, "#FF8A80", "#2B1517", "#5C2B31");
                setLoginButtonState(true, "Entrar");
            });
        });
    }

    private void addFieldLabel(LinearLayout parent, String text) {
        TextView label = new TextView(this);
        label.setText(text);
        label.setTextColor(Color.parseColor("#D8DEE4"));
        label.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
        label.setTypeface(Typeface.DEFAULT_BOLD);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        params.topMargin = dp(16);
        parent.addView(label, params);
    }

    private void togglePasswordVisibility() {
        passwordVisible = !passwordVisible;
        int selection = Math.max(passwordInput.getSelectionEnd(), 0);
        passwordInput.setInputType(passwordVisible
                ? InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD
                : InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD);
        passwordInput.setSelection(selection);
        passwordToggleButton.setText(passwordVisible ? "Ocultar" : "Mostrar");
    }

    private void setStatus(String text, String textColor, String fillColor, String strokeColor) {
        statusCard.animate().alpha(0.78f).setDuration(90).withEndAction(() -> {
            statusText.setText(text);
            statusText.setTextColor(Color.parseColor(textColor));
            statusCard.setBackground(makeRounded(fillColor, strokeColor, 18, 1));
            statusCard.animate().alpha(1f).setDuration(140).start();
        }).start();
    }

    private void setLoginButtonState(boolean enabled, String text) {
        loginButton.setEnabled(enabled);
        loginButton.setText(text);
        loginButton.setAlpha(enabled ? 1.0f : 0.55f);
    }

    private void showModInfoDialog(JSONObject mod) {
        try {
            ScrollView scrollView = new ScrollView(this);
            scrollView.setFillViewport(true);

            LinearLayout content = new LinearLayout(this);
            content.setOrientation(LinearLayout.VERTICAL);
            content.setPadding(dp(20), dp(18), dp(20), dp(8));
            content.setBackground(makeRounded("#171C21", "#44F0B35A", 24, 1));
            scrollView.addView(content, new ScrollView.LayoutParams(
                    ScrollView.LayoutParams.MATCH_PARENT,
                    ScrollView.LayoutParams.WRAP_CONTENT
            ));

            TextView eyebrow = new TextView(this);
            eyebrow.setText("Resumo do site");
            eyebrow.setTextColor(Color.parseColor("#F0B35A"));
            eyebrow.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            eyebrow.setTypeface(Typeface.DEFAULT_BOLD);
            eyebrow.setLetterSpacing(0.08f);
            content.addView(eyebrow);

            TextView title = new TextView(this);
            title.setText(mod.optString("title", "Mod"));
            title.setTextColor(Color.WHITE);
            title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
            title.setTypeface(Typeface.DEFAULT_BOLD);
            title.setPadding(0, dp(10), 0, dp(8));
            content.addView(title);

            TextView summary = new TextView(this);
            summary.setText(mod.optString("summary", "Sem resumo disponível."));
            summary.setTextColor(Color.parseColor("#D6DDE3"));
            summary.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
            summary.setLineSpacing(dp(2), 1.0f);
            content.addView(summary);

            addDialogMeta(content, "Status", mod.optString("status", "Indefinido"));
            addDialogMeta(content, "Acesso", mod.optString("availability", "Não informado"));
            addDialogMeta(content, "Downloads", String.valueOf(mod.optInt("downloadCount", 0)));

            JSONArray highlights = mod.optJSONArray("highlights");
            if (highlights != null && highlights.length() > 0) {
                TextView highlightsTitle = new TextView(this);
                highlightsTitle.setText("Novidades");
                highlightsTitle.setTextColor(Color.parseColor("#F0B35A"));
                highlightsTitle.setTextSize(TypedValue.COMPLEX_UNIT_SP, 13);
                highlightsTitle.setTypeface(Typeface.DEFAULT_BOLD);
                highlightsTitle.setPadding(0, dp(16), 0, dp(8));
                content.addView(highlightsTitle);

                for (int i = 0; i < highlights.length(); i++) {
                    String item = highlights.optString(i, "").trim();
                    if (item.isEmpty()) {
                        continue;
                    }
                    TextView bullet = new TextView(this);
                    bullet.setText("• " + item);
                    bullet.setTextColor(Color.parseColor("#D6DDE3"));
                    bullet.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
                    bullet.setLineSpacing(dp(2), 1.0f);
                    bullet.setPadding(0, 0, 0, dp(6));
                    content.addView(bullet);
                }
            }

            AlertDialog dialog = new AlertDialog.Builder(this)
                    .setView(scrollView)
                    .setPositiveButton("Fechar", null)
                    .create();
            dialog.show();
            if (dialog.getWindow() != null) {
                dialog.getWindow().setBackgroundDrawable(makeRounded("#101419", "#44F0B35A", 28, 1));
            }
        } catch (Exception exception) {
            Log.w(TAG, "Falha ao abrir dialog de resumo do mod", exception);
        }
    }

    private void maybeShowApkUpdateDialog(JSONObject mod) {
        if (apkUpdateDialogShown || mod == null) {
            return;
        }

        try {
            String publishedApkSha256 = mod.optString("apkSha256", "").trim();
            if (publishedApkSha256.isEmpty()) {
                return;
            }

            String installedApkSha256 = calculateInstalledApkSha256();
            if (installedApkSha256.isEmpty()) {
                return;
            }

            if (publishedApkSha256.equalsIgnoreCase(installedApkSha256)) {
                return;
            }

            apkUpdateDialogShown = true;

            String title = mod.optString("title", "Mod");
            String summary = mod.optString("summary", "Uma nova compilação do APK foi publicada.");
            String updatedAt = mod.optString("updatedAt", "").trim();
            String message = "Detectamos um APK mais recente para " + title + ".\n\n"
                    + summary
                    + (updatedAt.isEmpty() ? "" : "\n\nPublicação mais recente: " + updatedAt)
                    + "\n\nAtualize o APK para manter o conteúdo em sincronia.";

            new AlertDialog.Builder(this)
                    .setTitle("Atualização disponível")
                    .setMessage(message)
                    .setPositiveButton("Entendi", null)
                    .show();
        } catch (Exception exception) {
            Log.w(TAG, "Falha ao comparar SHA-256 do APK", exception);
        }
    }

    private void openModDetailsPage() {
        if (cachedModInfo == null) {
            Toast.makeText(this, "Os dados do mod ainda nao foram carregados.", Toast.LENGTH_SHORT).show();
            return;
        }

        String slug = cachedModInfo.optString("slug", "").trim();
        if (slug.isEmpty()) {
            Toast.makeText(this, "Slug do mod indisponivel.", Toast.LENGTH_SHORT).show();
            return;
        }

        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(MOD_DETAILS_BASE_URL + Uri.encode(slug)));
            startActivity(intent);
        } catch (Exception exception) {
            Log.w(TAG, "Falha ao abrir pagina do mod", exception);
            Toast.makeText(this, "Nao foi possivel abrir a pagina do mod.", Toast.LENGTH_LONG).show();
        }
    }

    private void showLoginSuccessDialog() {
        try {
            JSONObject summary = new JSONObject(GetNativeLoginSummary());
            String displayName = summary.optString("displayName", "Assinante");
            int remainingDays = summary.optInt("remainingDays", -1);
            String expiresAt = summary.optString("expiresAt", "").trim();
            String deviceFingerprint = summary.optString("deviceFingerprint", "").trim();
            String fingerprintShort = deviceFingerprint.length() > 12
                    ? deviceFingerprint.substring(0, 12) + "..."
                    : (deviceFingerprint.isEmpty() ? "Não informado" : deviceFingerprint);

            LinearLayout content = new LinearLayout(this);
            content.setOrientation(LinearLayout.VERTICAL);
            content.setPadding(dp(20), dp(18), dp(20), dp(8));
            content.setBackground(makeRounded("#171C21", "#44F0B35A", 24, 1));

            TextView eyebrow = new TextView(this);
            eyebrow.setText("Acesso liberado");
            eyebrow.setTextColor(Color.parseColor("#F0B35A"));
            eyebrow.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
            eyebrow.setTypeface(Typeface.DEFAULT_BOLD);
            eyebrow.setLetterSpacing(0.08f);
            content.addView(eyebrow);

            TextView title = new TextView(this);
            title.setText(displayName);
            title.setTextColor(Color.WHITE);
            title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 24);
            title.setTypeface(Typeface.DEFAULT_BOLD);
            title.setPadding(0, dp(10), 0, dp(8));
            content.addView(title);

            addDialogMeta(content, "Expiração", remainingDays >= 0 ? remainingDays + " dias" : "Não informada");
            addDialogMeta(content, "Validade", expiresAt.isEmpty() ? "Não informada" : expiresAt);
            addDialogMeta(content, "Dispositivo", fingerprintShort);

            AlertDialog dialog = new AlertDialog.Builder(this)
                    .setView(content)
                    .setPositiveButton("Abrir jogo", (dialogInterface, which) -> openGameAfterLogin())
                    .setCancelable(false)
                    .create();
            dialog.show();
            if (dialog.getWindow() != null) {
                dialog.getWindow().setBackgroundDrawable(makeRounded("#101419", "#44F0B35A", 28, 1));
            }
        } catch (Exception exception) {
            Log.w(TAG, "Falha ao montar dialog de sucesso do login", exception);
            openGameAfterLogin();
        }
    }

    private void openGameAfterLogin() {
        Main.Start(MainActivity.this);

        try {
            Intent intent = new Intent();
            intent.setClassName(MainActivity.this, GAME_ACTIVITY);
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            startActivity(intent);
            finish();
        } catch (Exception exception) {
            Log.e(TAG, "Falha ao abrir game activity", exception);
            showInlineError("O login foi aceito, mas a abertura do jogo falhou.");
            setStatus("Login ok, mas falhou ao abrir o jogo.", "#FF8A80", "#2B1517", "#5C2B31");
            setLoginButtonState(true, "Entrar");
        }
    }

    private void addDialogMeta(LinearLayout parent, String label, String value) {
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.VERTICAL);
        row.setBackground(makeRounded("#101419", "#2F3943", 16, 1));
        row.setPadding(dp(12), dp(10), dp(12), dp(10));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
                LinearLayout.LayoutParams.MATCH_PARENT,
                LinearLayout.LayoutParams.WRAP_CONTENT
        );
        params.topMargin = dp(10);
        parent.addView(row, params);

        TextView title = new TextView(this);
        title.setText(label);
        title.setTextColor(Color.parseColor("#8AA6BC"));
        title.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        row.addView(title);

        TextView body = new TextView(this);
        body.setText(value);
        body.setTextColor(Color.WHITE);
        body.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        body.setPadding(0, dp(4), 0, 0);
        row.addView(body);
    }

    private String readStream(InputStream stream) throws Exception {
        if (stream == null) {
            return "";
        }

        BufferedReader reader = new BufferedReader(new InputStreamReader(stream));
        StringBuilder builder = new StringBuilder();
        String line;
        while ((line = reader.readLine()) != null) {
            builder.append(line);
        }
        reader.close();
        return builder.toString();
    }

    private String calculateInstalledApkSha256() {
        try {
            String apkPath = getPackageCodePath();
            if (apkPath == null || apkPath.isEmpty()) {
                return "";
            }

            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            FileInputStream input = new FileInputStream(apkPath);
            try {
                byte[] buffer = new byte[8192];
                int read;
                while ((read = input.read(buffer)) != -1) {
                    digest.update(buffer, 0, read);
                }
            } finally {
                input.close();
            }

            byte[] hash = digest.digest();
            StringBuilder builder = new StringBuilder(hash.length * 2);
            for (byte value : hash) {
                builder.append(String.format("%02X", value));
            }
            return builder.toString();
        } catch (Exception exception) {
            Log.w(TAG, "Falha ao calcular SHA-256 do APK instalado", exception);
            return "";
        }
    }

    private void showInlineError(String message) {
        inlineErrorText.setText(message);
        inlineErrorText.setVisibility(View.VISIBLE);
        inlineErrorText.setAlpha(0f);
        inlineErrorText.animate().alpha(1f).setDuration(160).start();
    }

    private void clearInlineError() {
        inlineErrorText.setText("");
        inlineErrorText.setVisibility(View.GONE);
    }

    private EditText createInput(String hint, boolean password) {
        EditText input = new EditText(this);
        input.setHint(hint);
        input.setHintTextColor(Color.parseColor("#7D8892"));
        input.setTextColor(Color.WHITE);
        input.setTextSize(TypedValue.COMPLEX_UNIT_SP, 15);
        input.setBackground(makeRounded("#101419", "#2F3943", 18, 1));
        input.setPadding(dp(14), dp(14), dp(14), dp(14));
        input.setSingleLine(true);
        input.setInputType(password
                ? InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_PASSWORD
                : InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
        return input;
    }

    private void bindInputFocus(EditText input, GradientDrawable background) {
        input.setOnFocusChangeListener((view, hasFocus) -> {
            if (hasFocus) {
                background.setStroke(dp(2), Color.parseColor("#F0B35A"));
            } else {
                background.setStroke(dp(1), Color.parseColor("#2F3943"));
            }
        });
    }

    private GradientDrawable makeRounded(String fill, String stroke, int radiusDp, int strokeDp) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(Color.parseColor(fill));
        drawable.setCornerRadius(dp(radiusDp));
        drawable.setStroke(dp(strokeDp), Color.parseColor(stroke));
        return drawable;
    }

    private GradientDrawable makeVerticalGradient(String top, String bottom) {
        GradientDrawable drawable = new GradientDrawable(
                GradientDrawable.Orientation.TOP_BOTTOM,
                new int[]{
                        Color.parseColor(top),
                        Color.parseColor(bottom)
                }
        );
        drawable.setCornerRadius(dp(28));
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
