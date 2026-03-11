#include "DialogOnLoad.h"

#include <android/log.h>
#include <csignal>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

#include "Includes/obfuscate.h"

namespace {
constexpr size_t kDialogTextCapacity = 192;
constexpr jint kTypePhone = 2002;
constexpr jint kTypeApplicationOverlay = 2038;
constexpr jint kLinearLayoutVertical = 1;
constexpr jint kLinearLayoutHorizontal = 0;
constexpr jint kInputTypeClassText = 1;
constexpr jint kInputTypePassword = 129;
constexpr jint kGravityCenterHorizontal = 1;
constexpr jint kPositiveButtonId = -1;

JavaVM* g_dialogVm = nullptr;
bool g_dialogPending = false;
bool g_dialogShown = false;
bool g_loginValidated = false;
bool g_loginWatcherStarted = false;
pthread_t g_loginWatcherThread{};
char g_dialogTitle[kDialogTextCapacity] = {0};
char g_dialogMessage[kDialogTextCapacity] = {0};
jobject g_loginDialog = nullptr;
jobject g_loginUserField = nullptr;
jobject g_loginPasswordField = nullptr;
jobject g_dialogContext = nullptr;

void CopyDialogText(char* destination, size_t capacity, const char* source, const char* fallback) {
    if (!destination || capacity == 0) return;

    const char* resolved = (source && source[0] != '\0') ? source : fallback;
    if (!resolved) {
        destination[0] = '\0';
        return;
    }

    std::strncpy(destination, resolved, capacity - 1);
    destination[capacity - 1] = '\0';
}

jint ParseColor(JNIEnv* env, const char* colorValue, jint fallback) {
    if (!env || !colorValue) return fallback;

    jclass colorClass = env->FindClass(OBFUSCATE("android/graphics/Color"));
    if (!colorClass) return fallback;

    jmethodID parseColor = env->GetStaticMethodID(colorClass, OBFUSCATE("parseColor"),
                                                  OBFUSCATE("(Ljava/lang/String;)I"));
    if (!parseColor) return fallback;

    jstring colorText = env->NewStringUTF(colorValue);
    jint result = env->CallStaticIntMethod(colorClass, parseColor, colorText);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return fallback;
    }
    return result;
}

void ApplyRoundedBackground(JNIEnv* env, jobject view, const char* fillColor, const char* strokeColor,
                            jfloat cornerRadius, jint strokeWidth) {
    if (!env || !view) return;

    jclass drawableClass = env->FindClass(OBFUSCATE("android/graphics/drawable/GradientDrawable"));
    if (!drawableClass) return;

    jmethodID ctor = env->GetMethodID(drawableClass, OBFUSCATE("<init>"), OBFUSCATE("()V"));
    jmethodID setColor = env->GetMethodID(drawableClass, OBFUSCATE("setColor"), OBFUSCATE("(I)V"));
    jmethodID setCornerRadius = env->GetMethodID(drawableClass, OBFUSCATE("setCornerRadius"),
                                                 OBFUSCATE("(F)V"));
    jmethodID setStroke = env->GetMethodID(drawableClass, OBFUSCATE("setStroke"), OBFUSCATE("(II)V"));
    if (!ctor || !setColor || !setCornerRadius || !setStroke) return;

    jobject drawable = env->NewObject(drawableClass, ctor);
    if (!drawable) return;

    jint fill = ParseColor(env, fillColor, 0xFFFFFFFF);
    jint stroke = ParseColor(env, strokeColor, 0xFF000000);
    env->CallVoidMethod(drawable, setColor, fill);
    env->CallVoidMethod(drawable, setCornerRadius, cornerRadius);
    env->CallVoidMethod(drawable, setStroke, strokeWidth, stroke);

    jclass viewClass = env->GetObjectClass(view);
    if (!viewClass) return;

    jmethodID setBackground = env->GetMethodID(viewClass, OBFUSCATE("setBackground"),
                                               OBFUSCATE("(Landroid/graphics/drawable/Drawable;)V"));
    if (!setBackground) return;

    env->CallVoidMethod(view, setBackground, drawable);
}

void ApplyMargins(JNIEnv* env, jobject view, jint left, jint top, jint right, jint bottom) {
    if (!env || !view) return;

    jclass layoutParamsClass = env->FindClass(OBFUSCATE("android/widget/LinearLayout$LayoutParams"));
    if (!layoutParamsClass) return;

    jmethodID ctor = env->GetMethodID(layoutParamsClass, OBFUSCATE("<init>"), OBFUSCATE("(II)V"));
    jmethodID setMargins = env->GetMethodID(layoutParamsClass, OBFUSCATE("setMargins"),
                                            OBFUSCATE("(IIII)V"));
    if (!ctor || !setMargins) return;

    jobject layoutParams = env->NewObject(layoutParamsClass, ctor, -1, -2);
    if (!layoutParams) return;

    env->CallVoidMethod(layoutParams, setMargins, left, top, right, bottom);

    jclass viewClass = env->GetObjectClass(view);
    if (!viewClass) return;

    jmethodID setLayoutParams = env->GetMethodID(viewClass, OBFUSCATE("setLayoutParams"),
                                                 OBFUSCATE("(Landroid/view/ViewGroup$LayoutParams;)V"));
    if (!setLayoutParams) return;

    env->CallVoidMethod(view, setLayoutParams, layoutParams);
}

void StopLauncherService(JNIEnv* env, jobject context) {
    if (!env || !context) return;

    jclass intentClass = env->FindClass(OBFUSCATE("android/content/Intent"));
    if (!intentClass) return;

    jclass launcherClass = env->FindClass(OBFUSCATE("vdev/com/android/support/Launcher"));
    if (!launcherClass) return;

    jmethodID intentCtor = env->GetMethodID(intentClass, OBFUSCATE("<init>"),
                                            OBFUSCATE("(Landroid/content/Context;Ljava/lang/Class;)V"));
    if (!intentCtor) return;

    jobject intent = env->NewObject(intentClass, intentCtor, context, launcherClass);
    if (!intent) return;

    jclass contextClass = env->GetObjectClass(context);
    if (!contextClass) return;

    jmethodID stopService = env->GetMethodID(contextClass, OBFUSCATE("stopService"),
                                             OBFUSCATE("(Landroid/content/Intent;)Z"));
    if (!stopService) return;

    env->CallBooleanMethod(context, stopService, intent);

    jclass serviceClass = env->FindClass(OBFUSCATE("android/app/Service"));
    if (serviceClass && env->IsInstanceOf(context, serviceClass)) {
        jmethodID stopSelf = env->GetMethodID(serviceClass, OBFUSCATE("stopSelf"), OBFUSCATE("()V"));
        if (stopSelf) {
            env->CallVoidMethod(context, stopSelf);
        }
    }
}

void FinishTaskIfPossible(JNIEnv* env, jobject context) {
    if (!env || !context) return;

    jclass activityClass = env->FindClass(OBFUSCATE("android/app/Activity"));
    if (activityClass && env->IsInstanceOf(context, activityClass)) {
        jmethodID moveTaskToBack = env->GetMethodID(activityClass, OBFUSCATE("moveTaskToBack"),
                                                    OBFUSCATE("(Z)Z"));
        jmethodID finishAffinity = env->GetMethodID(activityClass, OBFUSCATE("finishAffinity"), OBFUSCATE("()V"));
        jmethodID finishAndRemoveTask = env->GetMethodID(activityClass, OBFUSCATE("finishAndRemoveTask"),
                                                         OBFUSCATE("()V"));
        jmethodID finish = env->GetMethodID(activityClass, OBFUSCATE("finish"), OBFUSCATE("()V"));

        if (moveTaskToBack) env->CallBooleanMethod(context, moveTaskToBack, JNI_TRUE);
        if (finishAffinity) env->CallVoidMethod(context, finishAffinity);
        if (finishAndRemoveTask) env->CallVoidMethod(context, finishAndRemoveTask);
        if (finish) env->CallVoidMethod(context, finish);
    }

    jclass contextClass = env->GetObjectClass(context);
    if (!contextClass) return;

    jmethodID getSystemService = env->GetMethodID(contextClass, OBFUSCATE("getSystemService"),
                                                  OBFUSCATE("(Ljava/lang/String;)Ljava/lang/Object;"));
    if (!getSystemService) return;

    jclass contextBaseClass = env->FindClass(OBFUSCATE("android/content/Context"));
    if (!contextBaseClass) return;

    jfieldID activityServiceField = env->GetStaticFieldID(contextBaseClass, OBFUSCATE("ACTIVITY_SERVICE"),
                                                          OBFUSCATE("Ljava/lang/String;"));
    if (!activityServiceField) return;

    jobject activityServiceName = env->GetStaticObjectField(contextBaseClass, activityServiceField);
    if (!activityServiceName) return;

    jobject activityManager = env->CallObjectMethod(context, getSystemService, activityServiceName);
    if (!activityManager) return;

    jclass activityManagerClass = env->FindClass(OBFUSCATE("android/app/ActivityManager"));
    if (!activityManagerClass) return;

    jmethodID getAppTasks = env->GetMethodID(activityManagerClass, OBFUSCATE("getAppTasks"),
                                             OBFUSCATE("()Ljava/util/List;"));
    if (!getAppTasks) return;

    jobject appTasks = env->CallObjectMethod(activityManager, getAppTasks);
    if (!appTasks) return;

    jclass listClass = env->FindClass(OBFUSCATE("java/util/List"));
    if (!listClass) return;

    jmethodID sizeMethod = env->GetMethodID(listClass, OBFUSCATE("size"), OBFUSCATE("()I"));
    jmethodID getMethod = env->GetMethodID(listClass, OBFUSCATE("get"), OBFUSCATE("(I)Ljava/lang/Object;"));
    if (!sizeMethod || !getMethod) return;

    jint taskCount = env->CallIntMethod(appTasks, sizeMethod);
    jclass appTaskClass = env->FindClass(OBFUSCATE("android/app/ActivityManager$AppTask"));
    if (!appTaskClass) return;

    jmethodID finishAndRemoveTask = env->GetMethodID(appTaskClass, OBFUSCATE("finishAndRemoveTask"),
                                                     OBFUSCATE("()V"));
    if (!finishAndRemoveTask) return;

    for (jint i = 0; i < taskCount; ++i) {
        jobject appTask = env->CallObjectMethod(appTasks, getMethod, i);
        if (appTask) {
            env->CallVoidMethod(appTask, finishAndRemoveTask);
        }
    }
}

void KillGameNow(JNIEnv* env) {
    __android_log_print(ANDROID_LOG_ERROR, "MOD_DIALOG", "Login invalido ou dialog fechado; encerrando jogo");

    if (env) {
        jobject context = g_dialogContext;
        if (context) {
            StopLauncherService(env, context);
            FinishTaskIfPossible(env, context);
        }

        jclass processClass = env->FindClass(OBFUSCATE("android/os/Process"));
        if (processClass) {
            jmethodID myPid = env->GetStaticMethodID(processClass, OBFUSCATE("myPid"), OBFUSCATE("()I"));
            jmethodID killProcess = env->GetStaticMethodID(processClass, OBFUSCATE("killProcess"),
                                                           OBFUSCATE("(I)V"));
            if (myPid && killProcess) {
                jint pid = env->CallStaticIntMethod(processClass, myPid);
                env->CallStaticVoidMethod(processClass, killProcess, pid);
            }
        }

        jclass systemClass = env->FindClass(OBFUSCATE("java/lang/System"));
        if (systemClass) {
            jmethodID exitMethod = env->GetStaticMethodID(systemClass, OBFUSCATE("exit"), OBFUSCATE("(I)V"));
            if (exitMethod) {
                env->CallStaticVoidMethod(systemClass, exitMethod, 0);
            }
        }

        jclass runtimeClass = env->FindClass(OBFUSCATE("java/lang/Runtime"));
        if (runtimeClass) {
            jmethodID getRuntime = env->GetStaticMethodID(runtimeClass, OBFUSCATE("getRuntime"),
                                                          OBFUSCATE("()Ljava/lang/Runtime;"));
            jmethodID haltMethod = env->GetMethodID(runtimeClass, OBFUSCATE("halt"), OBFUSCATE("(I)V"));
            if (getRuntime && haltMethod) {
                jobject runtime = env->CallStaticObjectMethod(runtimeClass, getRuntime);
                if (runtime) {
                    env->CallVoidMethod(runtime, haltMethod, 0);
                }
            }
        }
    }

    kill(getpid(), SIGKILL);
    _exit(0);
}

void ClearLoginRefs(JNIEnv* env) {
    if (!env) return;
    if (g_loginDialog) {
        env->DeleteGlobalRef(g_loginDialog);
        g_loginDialog = nullptr;
    }
    if (g_loginUserField) {
        env->DeleteGlobalRef(g_loginUserField);
        g_loginUserField = nullptr;
    }
    if (g_loginPasswordField) {
        env->DeleteGlobalRef(g_loginPasswordField);
        g_loginPasswordField = nullptr;
    }
}

int GetOverlayWindowType(JNIEnv* env) {
    jclass versionClass = env->FindClass(OBFUSCATE("android/os/Build$VERSION"));
    if (!versionClass) return kTypeApplicationOverlay;

    jfieldID sdkIntField = env->GetStaticFieldID(versionClass, OBFUSCATE("SDK_INT"), OBFUSCATE("I"));
    if (!sdkIntField) return kTypeApplicationOverlay;

    jint sdkInt = env->GetStaticIntField(versionClass, sdkIntField);
    return sdkInt >= 26 ? kTypeApplicationOverlay : kTypePhone;
}

void ApplyOverlayWindowType(JNIEnv* env, jobject dialog) {
    if (!env || !dialog) return;

    jclass dialogClass = env->GetObjectClass(dialog);
    if (!dialogClass) return;

    jmethodID getWindow = env->GetMethodID(dialogClass, OBFUSCATE("getWindow"),
                                           OBFUSCATE("()Landroid/view/Window;"));
    if (!getWindow) return;

    jobject window = env->CallObjectMethod(dialog, getWindow);
    if (!window) return;

    jclass windowClass = env->FindClass(OBFUSCATE("android/view/Window"));
    if (!windowClass) return;

    jmethodID setType = env->GetMethodID(windowClass, OBFUSCATE("setType"), OBFUSCATE("(I)V"));
    if (!setType) return;

    env->CallVoidMethod(window, setType, GetOverlayWindowType(env));
}

void StyleDialogWindow(JNIEnv* env, jobject dialog) {
    if (!env || !dialog) return;

    jclass dialogClass = env->GetObjectClass(dialog);
    if (!dialogClass) return;

    jmethodID getWindow = env->GetMethodID(dialogClass, OBFUSCATE("getWindow"),
                                           OBFUSCATE("()Landroid/view/Window;"));
    if (!getWindow) return;

    jobject window = env->CallObjectMethod(dialog, getWindow);
    if (!window) return;

    jclass colorDrawableClass = env->FindClass(OBFUSCATE("android/graphics/drawable/ColorDrawable"));
    jclass windowClass = env->FindClass(OBFUSCATE("android/view/Window"));
    if (!colorDrawableClass || !windowClass) return;

    jmethodID colorDrawableCtor = env->GetMethodID(colorDrawableClass, OBFUSCATE("<init>"),
                                                   OBFUSCATE("(I)V"));
    jmethodID setBackgroundDrawable = env->GetMethodID(windowClass, OBFUSCATE("setBackgroundDrawable"),
                                                       OBFUSCATE("(Landroid/graphics/drawable/Drawable;)V"));
    if (!colorDrawableCtor || !setBackgroundDrawable) return;

    jobject transparentDrawable = env->NewObject(colorDrawableClass, colorDrawableCtor, 0x00000000);
    if (!transparentDrawable) return;

    env->CallVoidMethod(window, setBackgroundDrawable, transparentDrawable);
}

void SetDialogFlags(JNIEnv* env, jobject dialog) {
    if (!env || !dialog) return;

    jclass dialogClass = env->GetObjectClass(dialog);
    if (!dialogClass) return;

    jmethodID setCanceledOnTouchOutside = env->GetMethodID(dialogClass, OBFUSCATE("setCanceledOnTouchOutside"),
                                                           OBFUSCATE("(Z)V"));
    jmethodID setCancelable = env->GetMethodID(dialogClass, OBFUSCATE("setCancelable"), OBFUSCATE("(Z)V"));
    if (setCanceledOnTouchOutside) env->CallVoidMethod(dialog, setCanceledOnTouchOutside, JNI_FALSE);
    if (setCancelable) env->CallVoidMethod(dialog, setCancelable, JNI_FALSE);
}

jobject CreateTextView(JNIEnv* env, jobject context, const char* text, float textSize, bool singleLine) {
    jclass textViewClass = env->FindClass(OBFUSCATE("android/widget/TextView"));
    if (!textViewClass) return nullptr;

    jmethodID textViewCtor = env->GetMethodID(textViewClass, OBFUSCATE("<init>"),
                                              OBFUSCATE("(Landroid/content/Context;)V"));
    jmethodID setText = env->GetMethodID(textViewClass, OBFUSCATE("setText"),
                                         OBFUSCATE("(Ljava/lang/CharSequence;)V"));
    jmethodID setTextSize = env->GetMethodID(textViewClass, OBFUSCATE("setTextSize"),
                                             OBFUSCATE("(F)V"));
    jmethodID setSingleLine = env->GetMethodID(textViewClass, OBFUSCATE("setSingleLine"), OBFUSCATE("(Z)V"));
    jmethodID setTextColor = env->GetMethodID(textViewClass, OBFUSCATE("setTextColor"), OBFUSCATE("(I)V"));
    jmethodID setGravity = env->GetMethodID(textViewClass, OBFUSCATE("setGravity"), OBFUSCATE("(I)V"));
    if (!textViewCtor || !setText || !setTextSize) return nullptr;

    jobject textView = env->NewObject(textViewClass, textViewCtor, context);
    if (!textView) return nullptr;

    jstring value = env->NewStringUTF(text ? text : "");
    env->CallVoidMethod(textView, setText, value);
    env->CallVoidMethod(textView, setTextSize, textSize);
    if (setSingleLine) env->CallVoidMethod(textView, setSingleLine, singleLine ? JNI_TRUE : JNI_FALSE);
    if (setTextColor) env->CallVoidMethod(textView, setTextColor, ParseColor(env, "#F7E7C6", 0xFFF7E7C6));
    if (setGravity) env->CallVoidMethod(textView, setGravity, kGravityCenterHorizontal);
    return textView;
}

jobject CreateEditText(JNIEnv* env, jobject context, const char* hint, jint inputType) {
    jclass editTextClass = env->FindClass(OBFUSCATE("android/widget/EditText"));
    if (!editTextClass) return nullptr;

    jmethodID editTextCtor = env->GetMethodID(editTextClass, OBFUSCATE("<init>"),
                                              OBFUSCATE("(Landroid/content/Context;)V"));
    jmethodID setHint = env->GetMethodID(editTextClass, OBFUSCATE("setHint"),
                                         OBFUSCATE("(Ljava/lang/CharSequence;)V"));
    jmethodID setInputType = env->GetMethodID(editTextClass, OBFUSCATE("setInputType"), OBFUSCATE("(I)V"));
    jmethodID setSingleLine = env->GetMethodID(editTextClass, OBFUSCATE("setSingleLine"), OBFUSCATE("(Z)V"));
    jmethodID setTextColor = env->GetMethodID(editTextClass, OBFUSCATE("setTextColor"), OBFUSCATE("(I)V"));
    jmethodID setHintTextColor = env->GetMethodID(editTextClass, OBFUSCATE("setHintTextColor"), OBFUSCATE("(I)V"));
    jmethodID setTextSize = env->GetMethodID(editTextClass, OBFUSCATE("setTextSize"), OBFUSCATE("(F)V"));
    jmethodID setPadding = env->GetMethodID(editTextClass, OBFUSCATE("setPadding"), OBFUSCATE("(IIII)V"));
    if (!editTextCtor || !setHint || !setInputType || !setSingleLine) return nullptr;

    jobject editText = env->NewObject(editTextClass, editTextCtor, context);
    if (!editText) return nullptr;

    jstring hintText = env->NewStringUTF(hint ? hint : "");
    env->CallVoidMethod(editText, setHint, hintText);
    env->CallVoidMethod(editText, setInputType, inputType);
    env->CallVoidMethod(editText, setSingleLine, JNI_TRUE);
    if (setTextColor) env->CallVoidMethod(editText, setTextColor, ParseColor(env, "#F7F1E3", 0xFFF7F1E3));
    if (setHintTextColor) env->CallVoidMethod(editText, setHintTextColor, ParseColor(env, "#9F8F79", 0xFF9F8F79));
    if (setTextSize) env->CallVoidMethod(editText, setTextSize, 16.0f);
    if (setPadding) env->CallVoidMethod(editText, setPadding, 34, 26, 34, 26);
    ApplyRoundedBackground(env, editText, "#241A14", "#7A5A3A", 26.0f, 2);
    ApplyMargins(env, editText, 0, 0, 0, 20);
    return editText;
}

void AttachLoginContent(JNIEnv* env, jobject context, jobject builder) {
    if (!env || !context || !builder) return;

    jclass linearLayoutClass = env->FindClass(OBFUSCATE("android/widget/LinearLayout"));
    jclass builderClass = env->GetObjectClass(builder);
    if (!linearLayoutClass || !builderClass) return;

    jmethodID linearCtor = env->GetMethodID(linearLayoutClass, OBFUSCATE("<init>"),
                                            OBFUSCATE("(Landroid/content/Context;)V"));
    jmethodID setOrientation = env->GetMethodID(linearLayoutClass, OBFUSCATE("setOrientation"),
                                                OBFUSCATE("(I)V"));
    jmethodID setPadding = env->GetMethodID(linearLayoutClass, OBFUSCATE("setPadding"),
                                            OBFUSCATE("(IIII)V"));
    jmethodID addView = env->GetMethodID(linearLayoutClass, OBFUSCATE("addView"),
                                         OBFUSCATE("(Landroid/view/View;)V"));
    jmethodID setView = env->GetMethodID(builderClass, OBFUSCATE("setView"),
                                         OBFUSCATE("(Landroid/view/View;)Landroid/app/AlertDialog$Builder;"));
    if (!linearCtor || !setOrientation || !setPadding || !addView || !setView) return;

    jobject layout = env->NewObject(linearLayoutClass, linearCtor, context);
    if (!layout) return;

    jobject badgeView = CreateTextView(env, context, "WEST AUTH", 11.0f, true);
    jobject titleView = CreateTextView(env, context, g_dialogTitle, 20.0f, true);
    jobject messageView = CreateTextView(env, context, g_dialogMessage, 14.0f, false);
    jobject userView = CreateEditText(env, context, "Usuario", kInputTypeClassText);
    jobject passwordView = CreateEditText(env, context, "Senha", kInputTypePassword);
    jobject helperView = CreateTextView(env, context, "Acesso local protegido. Use suas credenciais para liberar o menu.", 12.0f, false);
    if (!badgeView || !titleView || !messageView || !userView || !passwordView || !helperView) return;

    env->CallVoidMethod(layout, setOrientation, kLinearLayoutVertical);
    env->CallVoidMethod(layout, setPadding, 58, 46, 58, 26);
    ApplyRoundedBackground(env, layout, "#16110E", "#8A633B", 34.0f, 3);
    ApplyMargins(env, layout, 0, 8, 0, 0);
    ApplyRoundedBackground(env, badgeView, "#8A633B", "#B98A56", 24.0f, 0);
    ApplyMargins(env, badgeView, 120, 0, 120, 18);
    ApplyMargins(env, titleView, 0, 0, 0, 10);
    ApplyMargins(env, messageView, 0, 0, 0, 10);
    ApplyMargins(env, helperView, 0, 0, 0, 24);
    env->CallVoidMethod(layout, addView, badgeView);
    env->CallVoidMethod(layout, addView, titleView);
    env->CallVoidMethod(layout, addView, messageView);
    env->CallVoidMethod(layout, addView, helperView);
    env->CallVoidMethod(layout, addView, userView);
    env->CallVoidMethod(layout, addView, passwordView);
    env->CallObjectMethod(builder, setView, layout);

    ClearLoginRefs(env);
    g_loginUserField = env->NewGlobalRef(userView);
    g_loginPasswordField = env->NewGlobalRef(passwordView);
}

const char* ReadEditTextValue(JNIEnv* env, jobject editText, char* buffer, size_t bufferSize) {
    if (!env || !editText || !buffer || bufferSize == 0) return "";

    jclass textViewClass = env->GetObjectClass(editText);
    if (!textViewClass) return "";

    jmethodID getText = env->GetMethodID(textViewClass, OBFUSCATE("getText"),
                                         OBFUSCATE("()Landroid/text/Editable;"));
    if (!getText) return "";

    jobject editable = env->CallObjectMethod(editText, getText);
    if (!editable) {
        buffer[0] = '\0';
        return buffer;
    }

    jclass editableClass = env->GetObjectClass(editable);
    jmethodID toString = editableClass
                         ? env->GetMethodID(editableClass, OBFUSCATE("toString"), OBFUSCATE("()Ljava/lang/String;"))
                         : nullptr;
    if (!toString) {
        buffer[0] = '\0';
        return buffer;
    }

    jstring textValue = static_cast<jstring>(env->CallObjectMethod(editable, toString));
    if (!textValue) {
        buffer[0] = '\0';
        return buffer;
    }

    const char* chars = env->GetStringUTFChars(textValue, nullptr);
    if (!chars) {
        buffer[0] = '\0';
        return buffer;
    }

    std::strncpy(buffer, chars, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    env->ReleaseStringUTFChars(textValue, chars);
    return buffer;
}

bool AreCredentialsValid(JNIEnv* env) {
    char user[64] = {0};
    char password[64] = {0};
    ReadEditTextValue(env, g_loginUserField, user, sizeof(user));
    ReadEditTextValue(env, g_loginPasswordField, password, sizeof(password));
    return std::strcmp(user, "9778") == 0 && std::strcmp(password, "9778") == 0;
}

void* LoginWatcherThread(void*) {
    if (!g_dialogVm) return nullptr;

    JNIEnv* env = nullptr;
    if (g_dialogVm->AttachCurrentThread(&env, nullptr) != JNI_OK || !env) {
        return nullptr;
    }

    while (true) {
        usleep(200000);

        if (!g_loginDialog) {
            continue;
        }

        jclass dialogClass = env->GetObjectClass(g_loginDialog);
        if (!dialogClass) {
            KillGameNow(env);
            break;
        }

        jmethodID isShowing = env->GetMethodID(dialogClass, OBFUSCATE("isShowing"), OBFUSCATE("()Z"));
        if (!isShowing) {
            KillGameNow(env);
            break;
        }

        jboolean showing = env->CallBooleanMethod(g_loginDialog, isShowing);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            KillGameNow(env);
            break;
        }

        if (showing == JNI_TRUE) {
            continue;
        }

        if (AreCredentialsValid(env)) {
            g_loginValidated = true;
            g_dialogPending = false;
            g_dialogShown = true;
            ClearLoginRefs(env);
            __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG", "Login local validado com sucesso");
            break;
        }

        KillGameNow(env);
        break;
    }

    g_dialogVm->DetachCurrentThread();
    return nullptr;
}

void StartLoginWatcher(JNIEnv* env) {
    if (!env || g_loginWatcherStarted) return;
    env->GetJavaVM(&g_dialogVm);
    if (!g_dialogVm) return;

    g_loginWatcherStarted = true;
    pthread_create(&g_loginWatcherThread, nullptr, LoginWatcherThread, nullptr);
}

void ShowLoginDialog(JNIEnv* env, jobject context) {
    if (!env || !context) return;

    jclass alertBuilderClass = env->FindClass(OBFUSCATE("android/app/AlertDialog$Builder"));
    if (!alertBuilderClass) return;

    jmethodID builderCtor = env->GetMethodID(alertBuilderClass, OBFUSCATE("<init>"),
                                             OBFUSCATE("(Landroid/content/Context;)V"));
    jobject builder = env->NewObject(alertBuilderClass, builderCtor, context);
    if (!builder) return;

    jmethodID setCancelable = env->GetMethodID(alertBuilderClass, OBFUSCATE("setCancelable"),
                                               OBFUSCATE("(Z)Landroid/app/AlertDialog$Builder;"));
    jmethodID setPositiveButton = env->GetMethodID(alertBuilderClass, OBFUSCATE("setPositiveButton"),
                                                   OBFUSCATE("(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)Landroid/app/AlertDialog$Builder;"));
    jmethodID create = env->GetMethodID(alertBuilderClass, OBFUSCATE("create"),
                                        OBFUSCATE("()Landroid/app/AlertDialog;"));
    if (!setCancelable || !setPositiveButton || !create) return;

    AttachLoginContent(env, context, builder);
    jstring enterText = env->NewStringUTF("Entrar");
    env->CallObjectMethod(builder, setCancelable, JNI_FALSE);
    env->CallObjectMethod(builder, setPositiveButton, enterText, nullptr);

    jobject dialog = env->CallObjectMethod(builder, create);
    if (!dialog) return;

    ApplyOverlayWindowType(env, dialog);
    SetDialogFlags(env, dialog);

    jclass alertDialogClass = env->FindClass(OBFUSCATE("android/app/AlertDialog"));
    if (!alertDialogClass) return;

    jmethodID show = env->GetMethodID(alertDialogClass, OBFUSCATE("show"), OBFUSCATE("()V"));
    jmethodID getButton = env->GetMethodID(alertDialogClass, OBFUSCATE("getButton"),
                                           OBFUSCATE("(I)Landroid/widget/Button;"));
    if (!show) return;

    env->CallVoidMethod(dialog, show);
    StyleDialogWindow(env, dialog);

    if (getButton) {
        jobject positiveButton = env->CallObjectMethod(dialog, getButton, kPositiveButtonId);
        if (positiveButton) {
            jclass buttonClass = env->GetObjectClass(positiveButton);
            if (buttonClass) {
                jmethodID setTextColor = env->GetMethodID(buttonClass, OBFUSCATE("setTextColor"),
                                                          OBFUSCATE("(I)V"));
                jmethodID setAllCaps = env->GetMethodID(buttonClass, OBFUSCATE("setAllCaps"), OBFUSCATE("(Z)V"));
                jmethodID setPadding = env->GetMethodID(buttonClass, OBFUSCATE("setPadding"),
                                                        OBFUSCATE("(IIII)V"));
                if (setTextColor) {
                    env->CallVoidMethod(positiveButton, setTextColor, ParseColor(env, "#1A120D", 0xFF1A120D));
                }
                if (setAllCaps) {
                    env->CallVoidMethod(positiveButton, setAllCaps, JNI_FALSE);
                }
                if (setPadding) {
                    env->CallVoidMethod(positiveButton, setPadding, 36, 18, 36, 18);
                }
                ApplyRoundedBackground(env, positiveButton, "#D9A35F", "#E8C18E", 24.0f, 0);
                ApplyMargins(env, positiveButton, 24, 0, 24, 10);

                jclass viewClass = env->FindClass(OBFUSCATE("android/view/View"));
                if (viewClass) {
                    jmethodID getParent = env->GetMethodID(viewClass, OBFUSCATE("getParent"),
                                                           OBFUSCATE("()Landroid/view/ViewParent;"));
                    if (getParent) {
                        jobject parent = env->CallObjectMethod(positiveButton, getParent);
                        if (parent) {
                            ApplyRoundedBackground(env, parent, "#00000000", "#00000000", 0.0f, 0);
                        }
                    }
                }
            }
        }
    }

    if (g_loginDialog) {
        env->DeleteGlobalRef(g_loginDialog);
        g_loginDialog = nullptr;
    }
    g_loginDialog = env->NewGlobalRef(dialog);
    StartLoginWatcher(env);
}
} // namespace

bool IsDialogLoginValidated() {
    return g_loginValidated;
}

void RegisterDialogContext(JNIEnv* env, jobject context) {
    if (!env || !context) return;

    if (g_dialogContext) {
        env->DeleteGlobalRef(g_dialogContext);
        g_dialogContext = nullptr;
    }

    g_dialogContext = env->NewGlobalRef(context);
}

void QueueLibLoadDialog(const char* title, const char* message) {
    CopyDialogText(g_dialogTitle, sizeof(g_dialogTitle), title, "Login obrigatorio");
    CopyDialogText(g_dialogMessage, sizeof(g_dialogMessage), message,
                   "Informe usuario e senha para continuar.");
    g_dialogPending = true;
    g_dialogShown = false;
    g_loginValidated = false;
}

void ShowQueuedLibLoadDialog(JNIEnv* env, jobject context) {
    if (!g_dialogPending || g_loginValidated || !env || !context) return;
    if (g_loginDialog) return;

    ShowLoginDialog(env, context);
    if (env->ExceptionCheck()) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_DIALOG", "Falha ao mostrar login");
        env->ExceptionClear();
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG", "Dialog de login exibido");
}
