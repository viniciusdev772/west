#include "DialogOnLoad.h"

#include <android/log.h>
#include <csignal>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <pthread.h>
#include <sstream>
#include <string>
#include <sys/system_properties.h>
#include <unistd.h>
#include <curl/curl.h>

#include "Includes/obfuscate.h"

void Toast(JNIEnv *env, jobject thiz, const char *text, int length);

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
bool g_warningPending = false;
bool g_toastPending = false;
int g_loginGeneration = 0;
int g_validatedGeneration = 0;
pthread_t g_loginWatcherThread{};
char g_dialogTitle[kDialogTextCapacity] = {0};
char g_dialogMessage[kDialogTextCapacity] = {0};
char g_warningTitle[kDialogTextCapacity] = {0};
char g_warningMessage[kDialogTextCapacity] = {0};
char g_toastMessage[kDialogTextCapacity] = {0};
jobject g_loginDialog = nullptr;
jobject g_warningDialog = nullptr;
jobject g_loginUserField = nullptr;
jobject g_loginPasswordField = nullptr;
jobject g_dialogContext = nullptr;

constexpr const char* kBackendBaseUrl = "https://vinaomods.vercel.app";
constexpr jint kHttpConnectTimeoutMs = 8000;
constexpr jint kHttpReadTimeoutMs = 12000;
char g_modSessionToken[256] = {0};
char g_modDeviceFingerprint[128] = {0};
char g_javaLoginError[kDialogTextCapacity] = {0};
char g_loginDisplayName[128] = {0};
char g_loginPolicyExpiresAt[64] = {0};
char g_loginSuccessSummary[512] = {0};
int g_loginRemainingDays = -1;

void ClearLoginRefs(JNIEnv* env);
void ClearDialogRef(JNIEnv* env);
void RecoverLoginState(JNIEnv* env, const char* reason);
void ClearWarningRefs(JNIEnv* env);
void ShowWarningDialog(JNIEnv* env, jobject context);
bool IsDialogCurrentlyShowing(JNIEnv* env, jobject dialog);

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

std::string JStringToStdString(JNIEnv* env, jstring value) {
    if (!env || !value) return "";

    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return "";

    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

std::string JsonEscape(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);

    for (char ch : value) {
        switch (ch) {
            case '\\': escaped += "\\\\"; break;
            case '"': escaped += "\\\""; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }

    return escaped;
}

std::string ExtractJsonString(const std::string& json, const char* key) {
    if (!key || !key[0]) return "";

    const std::string token = std::string("\"") + key + "\":\"";
    const size_t start = json.find(token);
    if (start == std::string::npos) return "";

    size_t cursor = start + token.size();
    std::string value;
    bool escaped = false;

    while (cursor < json.size()) {
        const char ch = json[cursor++];
        if (escaped) {
            value += ch;
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') break;
        value += ch;
    }

    return value;
}

std::string ExtractJsonStringAfterAnchor(const std::string& json, const char* anchor, const char* key) {
    if (!anchor || !anchor[0] || !key || !key[0]) return "";

    const size_t anchorStart = json.find(anchor);
    if (anchorStart == std::string::npos) return "";

    const std::string token = std::string("\"") + key + "\":\"";
    const size_t start = json.find(token, anchorStart);
    if (start == std::string::npos) return "";

    size_t cursor = start + token.size();
    std::string value;
    bool escaped = false;

    while (cursor < json.size()) {
        const char ch = json[cursor++];
        if (escaped) {
            value += ch;
            escaped = false;
            continue;
        }
        if (ch == '\\') {
            escaped = true;
            continue;
        }
        if (ch == '"') break;
        value += ch;
    }

    return value;
}

bool ExtractJsonBool(const std::string& json, const char* key, bool fallback) {
    if (!key || !key[0]) return fallback;

    const std::string token = std::string("\"") + key + "\":";
    const size_t start = json.find(token);
    if (start == std::string::npos) return fallback;

    const size_t valueStart = start + token.size();
    if (json.compare(valueStart, 4, "true") == 0) return true;
    if (json.compare(valueStart, 5, "false") == 0) return false;
    return fallback;
}

int CalculateRemainingDaysFromIsoUtc(const std::string& isoText) {
    if (isoText.size() < 10) return -1;

    int year = 0;
    int month = 0;
    int day = 0;
    if (std::sscanf(isoText.c_str(), "%d-%d-%d", &year, &month, &day) != 3) {
        return -1;
    }

    std::tm expiryTm{};
    expiryTm.tm_year = year - 1900;
    expiryTm.tm_mon = month - 1;
    expiryTm.tm_mday = day;
    expiryTm.tm_hour = 23;
    expiryTm.tm_min = 59;
    expiryTm.tm_sec = 59;

    const time_t expiryTime = timegm(&expiryTm);
    if (expiryTime <= 0) return -1;

    const time_t now = std::time(nullptr);
    const double remainingSeconds = std::difftime(expiryTime, now);
    if (remainingSeconds <= 0) return 0;

    return static_cast<int>(remainingSeconds / 86400.0) + 1;
}

size_t CurlWriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    if (!userp || !contents) return 0;
    const size_t totalSize = size * nmemb;
    std::string* buffer = static_cast<std::string*>(userp);
    buffer->append(static_cast<const char*>(contents), totalSize);
    return totalSize;
}

bool HttpPostJson(JNIEnv* env, const std::string& urlText, const std::string& body, int* statusCode,
                  std::string* responseText) {
    if (!env || !statusCode || !responseText) return false;

    *statusCode = 0;
    responseText->clear();
    CURLcode globalInitResult = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (globalInitResult != CURLE_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_DIALOG", "curl_global_init failed: %s",
                            curl_easy_strerror(globalInitResult));
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl) {
        curl_global_cleanup();
        __android_log_print(ANDROID_LOG_ERROR, "MOD_DIALOG", "curl_easy_init failed");
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, urlText.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, responseText);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(kHttpConnectTimeoutMs));
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(kHttpReadTimeoutMs));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "VinaoModsNative/1.0");
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    char errorBuffer[CURL_ERROR_SIZE] = {0};
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);

    const CURLcode performResult = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    *statusCode = static_cast<int>(httpCode);

    if (performResult != CURLE_OK) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_DIALOG",
                            "curl_easy_perform failed url=%s code=%d err=%s detail=%s",
                            urlText.c_str(), static_cast<int>(performResult), curl_easy_strerror(performResult),
                            errorBuffer[0] ? errorBuffer : "n/a");
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return false;
    }

    __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG",
                        "curl POST ok url=%s status=%d response=%s",
                        urlText.c_str(), *statusCode,
                        responseText->empty() ? "<empty>" : responseText->c_str());

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return true;
}

std::string GetStaticStringField(JNIEnv* env, const char* className, const char* fieldName) {
    jclass clazz = env->FindClass(className);
    jfieldID field = clazz ? env->GetStaticFieldID(clazz, fieldName, OBFUSCATE("Ljava/lang/String;")) : nullptr;
    return field ? JStringToStdString(env, static_cast<jstring>(env->GetStaticObjectField(clazz, field))) : "";
}

std::string GetPackageName(JNIEnv* env, jobject context) {
    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPackageNameMethod = contextClass
                                     ? env->GetMethodID(contextClass, OBFUSCATE("getPackageName"),
                                                        OBFUSCATE("()Ljava/lang/String;"))
                                     : nullptr;
    return getPackageNameMethod
           ? JStringToStdString(env, static_cast<jstring>(env->CallObjectMethod(context, getPackageNameMethod)))
           : "";
}

int GetSdkInt(JNIEnv* env) {
    jclass versionClass = env->FindClass(OBFUSCATE("android/os/Build$VERSION"));
    jfieldID sdkField = versionClass
                        ? env->GetStaticFieldID(versionClass, OBFUSCATE("SDK_INT"), OBFUSCATE("I"))
                        : nullptr;
    return sdkField ? env->GetStaticIntField(versionClass, sdkField) : 0;
}

bool GetPackageVersionInfo(JNIEnv* env, jobject context, const std::string& packageName, int* versionCode,
                           std::string* versionName) {
    if (!env || !context || packageName.empty() || !versionCode || !versionName) return false;

    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPackageManager = contextClass
                                  ? env->GetMethodID(contextClass, OBFUSCATE("getPackageManager"),
                                                     OBFUSCATE("()Landroid/content/pm/PackageManager;"))
                                  : nullptr;
    if (!getPackageManager) return false;

    jobject packageManager = env->CallObjectMethod(context, getPackageManager);
    jclass pmClass = packageManager ? env->GetObjectClass(packageManager) : nullptr;
    jmethodID getPackageInfo = pmClass
                               ? env->GetMethodID(pmClass, OBFUSCATE("getPackageInfo"),
                                                  OBFUSCATE("(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;"))
                               : nullptr;
    if (!getPackageInfo) return false;

    jstring packageNameValue = env->NewStringUTF(packageName.c_str());
    jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfo, packageNameValue, 0);
    if (!packageInfo) {
        env->ExceptionClear();
        return false;
    }

    jclass packageInfoClass = env->GetObjectClass(packageInfo);
    jfieldID versionNameField = packageInfoClass
                                ? env->GetFieldID(packageInfoClass, OBFUSCATE("versionName"),
                                                  OBFUSCATE("Ljava/lang/String;"))
                                : nullptr;
    if (versionNameField) {
        *versionName = JStringToStdString(env, static_cast<jstring>(env->GetObjectField(packageInfo, versionNameField)));
    }

    jmethodID getLongVersionCode = packageInfoClass
                                   ? env->GetMethodID(packageInfoClass, OBFUSCATE("getLongVersionCode"), OBFUSCATE("()J"))
                                   : nullptr;
    if (getLongVersionCode) {
        *versionCode = static_cast<int>(env->CallLongMethod(packageInfo, getLongVersionCode));
        return true;
    }

    jfieldID versionCodeField = packageInfoClass
                                ? env->GetFieldID(packageInfoClass, OBFUSCATE("versionCode"), OBFUSCATE("I"))
                                : nullptr;
    *versionCode = versionCodeField ? env->GetIntField(packageInfo, versionCodeField) : 0;
    return true;
}

std::string DigestSignatureSha256(JNIEnv* env, jobject signature) {
    if (!env || !signature) return "";

    jclass signatureClass = env->GetObjectClass(signature);
    jmethodID toByteArray = signatureClass
                            ? env->GetMethodID(signatureClass, OBFUSCATE("toByteArray"), OBFUSCATE("()[B"))
                            : nullptr;
    if (!toByteArray) return "";

    jbyteArray certBytes = static_cast<jbyteArray>(env->CallObjectMethod(signature, toByteArray));
    if (!certBytes) return "";

    jclass digestClass = env->FindClass(OBFUSCATE("java/security/MessageDigest"));
    jmethodID getInstance = digestClass
                            ? env->GetStaticMethodID(digestClass, OBFUSCATE("getInstance"),
                                                     OBFUSCATE("(Ljava/lang/String;)Ljava/security/MessageDigest;"))
                            : nullptr;
    jmethodID digestMethod = digestClass
                             ? env->GetMethodID(digestClass, OBFUSCATE("digest"), OBFUSCATE("([B)[B"))
                             : nullptr;
    if (!getInstance || !digestMethod) return "";

    jstring algorithm = env->NewStringUTF("SHA-256");
    jobject digestInstance = env->CallStaticObjectMethod(digestClass, getInstance, algorithm);
    jbyteArray digestBytes = digestInstance
                             ? static_cast<jbyteArray>(env->CallObjectMethod(digestInstance, digestMethod, certBytes))
                             : nullptr;
    if (!digestBytes) return "";

    const jsize length = env->GetArrayLength(digestBytes);
    jbyte* raw = env->GetByteArrayElements(digestBytes, nullptr);
    static const char* kHex = "0123456789ABCDEF";
    std::string hex;
    hex.reserve(length * 2);
    for (jsize i = 0; i < length; ++i) {
        const unsigned char value = static_cast<unsigned char>(raw[i]);
        hex.push_back(kHex[(value >> 4) & 0x0F]);
        hex.push_back(kHex[value & 0x0F]);
    }
    env->ReleaseByteArrayElements(digestBytes, raw, JNI_ABORT);
    return hex;
}

std::string GetSigningDigestSha256(JNIEnv* env, jobject context, const std::string& packageName, int sdkInt) {
    if (!env || !context || packageName.empty()) return "";

    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPackageManager = contextClass
                                  ? env->GetMethodID(contextClass, OBFUSCATE("getPackageManager"),
                                                     OBFUSCATE("()Landroid/content/pm/PackageManager;"))
                                  : nullptr;
    if (!getPackageManager) return "";

    jobject packageManager = env->CallObjectMethod(context, getPackageManager);
    jclass pmClass = packageManager ? env->GetObjectClass(packageManager) : nullptr;
    jmethodID getPackageInfo = pmClass
                               ? env->GetMethodID(pmClass, OBFUSCATE("getPackageInfo"),
                                                  OBFUSCATE("(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;"))
                               : nullptr;
    if (!getPackageInfo) return "";

    const jint flags = sdkInt >= 28 ? 0x08000000 : 64;
    jstring packageNameValue = env->NewStringUTF(packageName.c_str());
    jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfo, packageNameValue, flags);
    if (!packageInfo) {
        env->ExceptionClear();
        return "";
    }

    jclass packageInfoClass = env->GetObjectClass(packageInfo);
    if (sdkInt >= 28) {
        jfieldID signingInfoField = env->GetFieldID(packageInfoClass, OBFUSCATE("signingInfo"),
                                                    OBFUSCATE("Landroid/content/pm/SigningInfo;"));
        jobject signingInfo = signingInfoField ? env->GetObjectField(packageInfo, signingInfoField) : nullptr;
        if (!signingInfo) return "";

        jclass signingInfoClass = env->GetObjectClass(signingInfo);
        jmethodID getSigners = signingInfoClass
                               ? env->GetMethodID(signingInfoClass, OBFUSCATE("getApkContentsSigners"),
                                                  OBFUSCATE("()[Landroid/content/pm/Signature;"))
                               : nullptr;
        jobjectArray signers = getSigners
                               ? static_cast<jobjectArray>(env->CallObjectMethod(signingInfo, getSigners))
                               : nullptr;
        jobject signer = signers && env->GetArrayLength(signers) > 0
                         ? env->GetObjectArrayElement(signers, 0)
                         : nullptr;
        return DigestSignatureSha256(env, signer);
    }

    jfieldID signaturesField = env->GetFieldID(packageInfoClass, OBFUSCATE("signatures"),
                                               OBFUSCATE("[Landroid/content/pm/Signature;"));
    jobjectArray signatures = signaturesField
                              ? static_cast<jobjectArray>(env->GetObjectField(packageInfo, signaturesField))
                              : nullptr;
    jobject signer = signatures && env->GetArrayLength(signatures) > 0
                     ? env->GetObjectArrayElement(signatures, 0)
                     : nullptr;
    return DigestSignatureSha256(env, signer);
}

std::string GetInstallerPackage(JNIEnv* env, jobject context, const std::string& packageName, int sdkInt) {
    if (!env || !context || packageName.empty()) return "unknown";

    jclass contextClass = env->GetObjectClass(context);
    jmethodID getPackageManager = contextClass
                                  ? env->GetMethodID(contextClass, OBFUSCATE("getPackageManager"),
                                                     OBFUSCATE("()Landroid/content/pm/PackageManager;"))
                                  : nullptr;
    if (!getPackageManager) return "unknown";

    jobject packageManager = env->CallObjectMethod(context, getPackageManager);
    jclass pmClass = packageManager ? env->GetObjectClass(packageManager) : nullptr;
    if (!pmClass) return "unknown";

    jstring packageNameValue = env->NewStringUTF(packageName.c_str());
    if (sdkInt >= 30) {
        jmethodID getInstallSourceInfo = env->GetMethodID(
                pmClass, OBFUSCATE("getInstallSourceInfo"),
                OBFUSCATE("(Ljava/lang/String;)Landroid/content/pm/InstallSourceInfo;"));
        if (getInstallSourceInfo) {
            jobject sourceInfo = env->CallObjectMethod(packageManager, getInstallSourceInfo, packageNameValue);
            if (sourceInfo && !env->ExceptionCheck()) {
                jclass sourceInfoClass = env->GetObjectClass(sourceInfo);
                jmethodID getInstallingPackageName = sourceInfoClass
                                                     ? env->GetMethodID(sourceInfoClass,
                                                                        OBFUSCATE("getInstallingPackageName"),
                                                                        OBFUSCATE("()Ljava/lang/String;"))
                                                     : nullptr;
                std::string installer = getInstallingPackageName
                                        ? JStringToStdString(env, static_cast<jstring>(
                                                env->CallObjectMethod(sourceInfo, getInstallingPackageName)))
                                        : "";
                if (!installer.empty()) return installer;
            }
            env->ExceptionClear();
        }
    }

    jmethodID getInstallerPackageName = env->GetMethodID(
            pmClass, OBFUSCATE("getInstallerPackageName"),
            OBFUSCATE("(Ljava/lang/String;)Ljava/lang/String;"));
    if (!getInstallerPackageName) return "unknown";

    std::string installer = JStringToStdString(
            env, static_cast<jstring>(env->CallObjectMethod(packageManager, getInstallerPackageName, packageNameValue)));
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
    return installer.empty() ? "manual" : installer;
}

bool GetApplicationDebuggable(JNIEnv* env, jobject context) {
    if (!env || !context) return false;

    jclass contextClass = env->GetObjectClass(context);
    jmethodID getApplicationInfo = contextClass
                                   ? env->GetMethodID(contextClass, OBFUSCATE("getApplicationInfo"),
                                                      OBFUSCATE("()Landroid/content/pm/ApplicationInfo;"))
                                   : nullptr;
    if (!getApplicationInfo) return false;

    jobject applicationInfo = env->CallObjectMethod(context, getApplicationInfo);
    jclass applicationInfoClass = applicationInfo ? env->GetObjectClass(applicationInfo) : nullptr;
    jfieldID flagsField = applicationInfoClass
                          ? env->GetFieldID(applicationInfoClass, OBFUSCATE("flags"), OBFUSCATE("I"))
                          : nullptr;
    if (!flagsField) return false;

    const jint flags = env->GetIntField(applicationInfo, flagsField);
    return (flags & 0x2) != 0;
}

int GetSettingsInt(JNIEnv* env, jobject context, const char* settingsClassName, const char* fieldName) {
    if (!env || !context || !settingsClassName || !fieldName) return 0;

    jclass contextClass = env->GetObjectClass(context);
    jmethodID getContentResolver = contextClass
                                   ? env->GetMethodID(contextClass, OBFUSCATE("getContentResolver"),
                                                      OBFUSCATE("()Landroid/content/ContentResolver;"))
                                   : nullptr;
    if (!getContentResolver) return 0;

    jobject contentResolver = env->CallObjectMethod(context, getContentResolver);
    jclass settingsClass = env->FindClass(settingsClassName);
    jfieldID field = settingsClass
                     ? env->GetStaticFieldID(settingsClass, fieldName, OBFUSCATE("Ljava/lang/String;"))
                     : nullptr;
    jmethodID getInt = settingsClass
                       ? env->GetStaticMethodID(settingsClass, OBFUSCATE("getInt"),
                                                OBFUSCATE("(Landroid/content/ContentResolver;Ljava/lang/String;I)I"))
                       : nullptr;
    if (!field || !getInt) return 0;

    jobject key = env->GetStaticObjectField(settingsClass, field);
    return env->CallStaticIntMethod(settingsClass, getInt, contentResolver, key, 0);
}

std::string GetAndroidId(JNIEnv* env, jobject context) {
    if (!env || !context) return "";

    jclass contextClass = env->GetObjectClass(context);
    jmethodID getContentResolver = contextClass
                                   ? env->GetMethodID(contextClass, OBFUSCATE("getContentResolver"),
                                                      OBFUSCATE("()Landroid/content/ContentResolver;"))
                                   : nullptr;
    if (!getContentResolver) return "";

    jobject contentResolver = env->CallObjectMethod(context, getContentResolver);
    jclass secureClass = env->FindClass(OBFUSCATE("android/provider/Settings$Secure"));
    jfieldID androidIdField = secureClass
                              ? env->GetStaticFieldID(secureClass, OBFUSCATE("ANDROID_ID"),
                                                      OBFUSCATE("Ljava/lang/String;"))
                              : nullptr;
    jmethodID getString = secureClass
                          ? env->GetStaticMethodID(secureClass, OBFUSCATE("getString"),
                                                   OBFUSCATE("(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;"))
                          : nullptr;
    if (!androidIdField || !getString) return "";

    jobject key = env->GetStaticObjectField(secureClass, androidIdField);
    return JStringToStdString(
            env, static_cast<jstring>(env->CallStaticObjectMethod(secureClass, getString, contentResolver, key)));
}

std::string GetPrimaryAbi(JNIEnv* env) {
    std::string abi = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("CPU_ABI"));
    if (!abi.empty()) return abi;

    jclass buildClass = env->FindClass(OBFUSCATE("android/os/Build"));
    jfieldID supportedAbisField = buildClass
                                  ? env->GetStaticFieldID(buildClass, OBFUSCATE("SUPPORTED_ABIS"),
                                                          OBFUSCATE("[Ljava/lang/String;"))
                                  : nullptr;
    jobjectArray supportedAbis = supportedAbisField
                                 ? static_cast<jobjectArray>(env->GetStaticObjectField(buildClass, supportedAbisField))
                                 : nullptr;
    return supportedAbis && env->GetArrayLength(supportedAbis) > 0
           ? JStringToStdString(env, static_cast<jstring>(env->GetObjectArrayElement(supportedAbis, 0)))
           : "";
}

bool FileExists(const char* path) {
    return path && access(path, F_OK) == 0;
}

bool DetectRoot() {
    static const char* kRootPaths[] = {
            "/system/bin/su",
            "/system/xbin/su",
            "/sbin/su",
            "/vendor/bin/su",
            "/system/app/Superuser.apk",
            "/system/app/Magisk.apk",
            "/data/adb/magisk",
    };

    for (const char* path : kRootPaths) {
        if (FileExists(path)) return true;
    }

    char prop[PROP_VALUE_MAX] = {0};
    if (__system_property_get("ro.debuggable", prop) > 0 && std::strcmp(prop, "1") == 0) return true;
    if (__system_property_get("ro.secure", prop) > 0 && std::strcmp(prop, "0") == 0) return true;
    return false;
}

bool DetectPtrace() {
    std::ifstream status("/proc/self/status");
    if (!status.is_open()) return false;

    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind("TracerPid:", 0) == 0) {
            const char* value = line.c_str() + 10;
            while (*value == ' ' || *value == '\t') ++value;
            return std::atoi(value) > 0;
        }
    }
    return false;
}

bool DetectHooksInMaps() {
    std::ifstream maps("/proc/self/maps");
    if (!maps.is_open()) return false;

    std::string line;
    while (std::getline(maps, line)) {
        std::string lowered = line;
        for (char& ch : lowered) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        if (lowered.find("frida") != std::string::npos ||
            lowered.find("xposed") != std::string::npos ||
            lowered.find("substrate") != std::string::npos ||
            lowered.find("zygisk") != std::string::npos ||
            lowered.find("lsposed") != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool DetectEmulator(JNIEnv* env) {
    const std::string fingerprint = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("FINGERPRINT"));
    const std::string model = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("MODEL"));
    const std::string brand = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("BRAND"));
    const std::string manufacturer = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("MANUFACTURER"));
    const std::string product = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("PRODUCT"));

    auto containsToken = [](std::string value, const char* token) {
        for (char& ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return value.find(token) != std::string::npos;
    };

    return containsToken(fingerprint, "generic") ||
           containsToken(fingerprint, "emulator") ||
           containsToken(model, "sdk") ||
           containsToken(model, "emulator") ||
           containsToken(brand, "generic") ||
           containsToken(manufacturer, "genymotion") ||
           containsToken(product, "sdk");
}

std::string BuildSuspiciousPropsJson() {
    static const char* kProps[] = {
            "ro.kernel.qemu",
            "ro.debuggable",
            "ro.secure",
            "ro.build.tags",
    };

    std::string json = "[";
    bool first = true;
    char value[PROP_VALUE_MAX] = {0};

    for (const char* key : kProps) {
        value[0] = '\0';
        if (__system_property_get(key, value) <= 0) continue;

        const bool suspicious = (std::strcmp(key, "ro.kernel.qemu") == 0 && std::strcmp(value, "1") == 0) ||
                                (std::strcmp(key, "ro.debuggable") == 0 && std::strcmp(value, "1") == 0) ||
                                (std::strcmp(key, "ro.secure") == 0 && std::strcmp(value, "0") == 0) ||
                                (std::strcmp(key, "ro.build.tags") == 0 && std::strstr(value, "test-keys"));
        if (!suspicious) continue;

        if (!first) json += ",";
        json += "\"";
        json += JsonEscape(std::string(key) + "=" + value);
        json += "\"";
        first = false;
    }

    json += "]";
    return json;
}

std::string MapBackendLoginError(const std::string& errorCode) {
    if (errorCode == "invalid_credentials") return "Usuario ou senha invalidos.";
    if (errorCode == "unauthorized") return "Usuario ou senha invalidos.";
    if (errorCode == "device_verification_required") return "Servidor indisponivel no momento. Tente novamente.";
    if (errorCode == "account_blocked") return "Conta bloqueada. Fale com o suporte.";
    if (errorCode == "license_inactive") return "Sua licenca nao esta ativa.";
    if (errorCode == "device_limit_reached") return "Limite de dispositivos atingido.";
    if (errorCode == "integrity_policy_denied") return "Ambiente inseguro detectado. Feche depuradores e tente novamente.";
    if (errorCode == "rate_limited") return "Muitas tentativas. Aguarde um pouco e tente novamente.";
    if (errorCode == "server_error") return "Servidor indisponivel no momento. Tente novamente.";
    if (errorCode == "challenge_not_found" || errorCode == "challenge_already_used" ||
        errorCode == "challenge_nonce_mismatch" || errorCode == "challenge_expired" ||
        errorCode == "challenge_ip_mismatch") {
        return "Falha ao iniciar a sessao segura. Tente novamente.";
    }
    if (errorCode == "integrity_request_hash_mismatch") return "Integridade da requisicao invalida.";
    return "Falha na autenticacao do mod.";
}

std::string BuildModLoginPayload(JNIEnv* env, jobject context, const std::string& email,
                                 const std::string& password, const std::string& challengeId,
                                 const std::string& challengeNonce) {
    const int sdkInt = GetSdkInt(env);
    const std::string packageName = GetPackageName(env, context);
    int versionCode = 0;
    std::string versionName;
    GetPackageVersionInfo(env, context, packageName, &versionCode, &versionName);

    const std::string installerPackage = GetInstallerPackage(env, context, packageName, sdkInt);
    const std::string signingDigest = GetSigningDigestSha256(env, context, packageName, sdkInt);
    const bool debuggable = GetApplicationDebuggable(env, context);
    const bool hooksDetected = DetectHooksInMaps();
    const bool ptraceDetected = DetectPtrace();
    const bool rootDetected = DetectRoot();
    const bool emulatorDetected = DetectEmulator(env);
    const bool adbEnabled = GetSettingsInt(env, context, OBFUSCATE("android/provider/Settings$Global"),
                                           OBFUSCATE("ADB_ENABLED")) == 1;
    const bool devOptionsEnabled = GetSettingsInt(env, context, OBFUSCATE("android/provider/Settings$Global"),
                                                  OBFUSCATE("DEVELOPMENT_SETTINGS_ENABLED")) == 1;
    const std::string androidId = GetAndroidId(env, context);
    const std::string release = GetStaticStringField(env, OBFUSCATE("android/os/Build$VERSION"), OBFUSCATE("RELEASE"));
    const std::string securityPatch = GetStaticStringField(env, OBFUSCATE("android/os/Build$VERSION"),
                                                           OBFUSCATE("SECURITY_PATCH"));
    const std::string abi = GetPrimaryAbi(env);
    const std::string brand = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("BRAND"));
    const std::string model = GetStaticStringField(env, OBFUSCATE("android/os/Build"), OBFUSCATE("MODEL"));
    const std::string manufacturer = GetStaticStringField(env, OBFUSCATE("android/os/Build"),
                                                          OBFUSCATE("MANUFACTURER"));
    const std::string fingerprint = GetStaticStringField(env, OBFUSCATE("android/os/Build"),
                                                         OBFUSCATE("FINGERPRINT"));
    const std::string suspiciousProps = BuildSuspiciousPropsJson();
    const bool tamperDetected = hooksDetected || ptraceDetected ||
                                signingDigest.empty() || packageName.empty() || versionCode <= 0;

    std::ostringstream json;
    json << "{"
         << "\"email\":\"" << JsonEscape(email) << "\","
         << "\"password\":\"" << JsonEscape(password) << "\","
         << "\"challengeId\":\"" << JsonEscape(challengeId) << "\","
         << "\"challengeNonce\":\"" << JsonEscape(challengeNonce) << "\","
         << "\"app\":{"
         << "\"packageName\":\"" << JsonEscape(packageName) << "\","
         << "\"versionName\":\"" << JsonEscape(versionName) << "\","
         << "\"versionCode\":" << versionCode << ","
         << "\"installerPackage\":\"" << JsonEscape(installerPackage.empty() ? "manual" : installerPackage) << "\","
         << "\"signingDigestSha256\":\"" << JsonEscape(signingDigest) << "\","
         << "\"debuggable\":" << (debuggable ? "true" : "false") << ","
         << "\"hooksDetected\":" << (hooksDetected ? "true" : "false") << ","
         << "\"tamperDetected\":" << (tamperDetected ? "true" : "false") << ","
         << "\"ptraceDetected\":" << (ptraceDetected ? "true" : "false")
         << "},"
         << "\"device\":{"
         << "\"androidId\":\"" << JsonEscape(androidId) << "\","
         << "\"sdkInt\":" << sdkInt << ","
         << "\"release\":\"" << JsonEscape(release) << "\","
         << "\"securityPatch\":\"" << JsonEscape(securityPatch) << "\","
         << "\"abi\":\"" << JsonEscape(abi) << "\","
         << "\"brand\":\"" << JsonEscape(brand) << "\","
         << "\"model\":\"" << JsonEscape(model) << "\","
         << "\"manufacturer\":\"" << JsonEscape(manufacturer) << "\","
         << "\"fingerprint\":\"" << JsonEscape(fingerprint) << "\","
         << "\"emulatorDetected\":" << (emulatorDetected ? "true" : "false") << ","
         << "\"rootDetected\":" << (rootDetected ? "true" : "false") << ","
         << "\"adbEnabled\":" << (adbEnabled ? "true" : "false") << ","
         << "\"devOptionsEnabled\":" << (devOptionsEnabled ? "true" : "false") << ","
         << "\"suspiciousProps\":" << suspiciousProps
         << "}"
         << "}";
    return json.str();
}

bool PerformBackendLogin(JNIEnv* env, jobject context, const std::string& email,
                         const std::string& password, std::string* failureReason) {
    if (failureReason) failureReason->clear();
    if (!env || !context || email.empty() || password.empty()) {
        if (failureReason) *failureReason = "Preencha usuario e senha.";
        return false;
    }

    int challengeStatus = 0;
    std::string challengeResponse;
    if (!HttpPostJson(env, std::string(kBackendBaseUrl) + "/api/mod/challenge", "{}", &challengeStatus,
                      &challengeResponse) || challengeStatus < 200 || challengeStatus >= 300) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_DIALOG",
                            "Challenge request failed status=%d response=%s",
                            challengeStatus,
                            challengeResponse.empty() ? "<empty>" : challengeResponse.c_str());
        if (failureReason) *failureReason = "Falha ao obter desafio seguro do servidor.";
        return false;
    }

    const std::string challengeId = ExtractJsonString(challengeResponse, "challengeId");
    const std::string challengeNonce = ExtractJsonString(challengeResponse, "challengeNonce");
    if (challengeId.empty() || challengeNonce.empty()) {
        if (failureReason) *failureReason = "Resposta invalida do servidor.";
        return false;
    }

    const std::string payload = BuildModLoginPayload(env, context, email, password, challengeId, challengeNonce);
    int loginStatus = 0;
    std::string loginResponse;
    if (!HttpPostJson(env, std::string(kBackendBaseUrl) + "/api/mod/login", payload, &loginStatus, &loginResponse)) {
        if (failureReason) *failureReason = "Falha de rede ao validar a conta.";
        return false;
    }

    if (loginStatus < 200 || loginStatus >= 300) {
        const std::string errorCode = ExtractJsonString(loginResponse, "error");
        if (failureReason) {
            if (!errorCode.empty()) {
                *failureReason = MapBackendLoginError(errorCode);
            } else if (loginStatus == 401) {
                *failureReason = "Usuario ou senha invalidos.";
            } else if (loginStatus == 403) {
                *failureReason = "Acesso ao mod negado por politica da conta ou do dispositivo.";
            } else if (loginStatus >= 500) {
                *failureReason = "Servidor indisponivel no momento. Tente novamente.";
            } else {
                *failureReason = "Falha na autenticacao do mod.";
            }
        }
        return false;
    }

    const std::string token = ExtractJsonString(loginResponse, "token");
    const std::string deviceFingerprint = ExtractJsonString(loginResponse, "deviceFingerprint");
    const std::string displayName = ExtractJsonStringAfterAnchor(loginResponse, "\"user\":{", "name");
    const std::string policyExpiresAt = ExtractJsonStringAfterAnchor(loginResponse, "\"policy\":{", "expiresAt");
    const int remainingDays = CalculateRemainingDaysFromIsoUtc(policyExpiresAt);
    if (token.empty() || deviceFingerprint.empty()) {
        if (failureReason) *failureReason = "Sessao do mod nao foi emitida corretamente.";
        return false;
    }

    std::strncpy(g_modSessionToken, token.c_str(), sizeof(g_modSessionToken) - 1);
    g_modSessionToken[sizeof(g_modSessionToken) - 1] = '\0';
    std::strncpy(g_modDeviceFingerprint, deviceFingerprint.c_str(), sizeof(g_modDeviceFingerprint) - 1);
    g_modDeviceFingerprint[sizeof(g_modDeviceFingerprint) - 1] = '\0';
    std::strncpy(g_loginDisplayName, displayName.c_str(), sizeof(g_loginDisplayName) - 1);
    g_loginDisplayName[sizeof(g_loginDisplayName) - 1] = '\0';
    std::strncpy(g_loginPolicyExpiresAt, policyExpiresAt.c_str(), sizeof(g_loginPolicyExpiresAt) - 1);
    g_loginPolicyExpiresAt[sizeof(g_loginPolicyExpiresAt) - 1] = '\0';
    g_loginRemainingDays = remainingDays;
    std::snprintf(
            g_loginSuccessSummary,
            sizeof(g_loginSuccessSummary),
            "{\"displayName\":\"%s\",\"remainingDays\":%d,\"expiresAt\":\"%s\",\"deviceFingerprint\":\"%s\"}",
            JsonEscape(displayName).c_str(),
            remainingDays,
            JsonEscape(policyExpiresAt).c_str(),
            JsonEscape(deviceFingerprint).c_str()
    );
    QueueLoginSuccessHints(displayName.c_str(), remainingDays);
    return true;
}

void ClearDialogRef(JNIEnv* env) {
    if (!env || !g_loginDialog) return;
    env->DeleteGlobalRef(g_loginDialog);
    g_loginDialog = nullptr;
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

void RecoverLoginState(JNIEnv* env, const char* reason) {
    g_loginValidated = false;
    g_validatedGeneration = 0;
    g_dialogPending = true;
    g_dialogShown = false;
    g_warningPending = false;
    g_modSessionToken[0] = '\0';
    g_modDeviceFingerprint[0] = '\0';
    g_loginDisplayName[0] = '\0';
    g_loginPolicyExpiresAt[0] = '\0';
    g_loginSuccessSummary[0] = '\0';
    g_loginRemainingDays = -1;
    ClearWarningRefs(env);
    CopyDialogText(g_toastMessage, sizeof(g_toastMessage),
                   reason && reason[0] ? reason : "Falha na autenticacao. Tente novamente.",
                   "Falha na autenticacao. Tente novamente.");
    g_toastPending = true;
    ClearDialogRef(env);
    ClearLoginRefs(env);
    __android_log_print(ANDROID_LOG_WARN, "MOD_DIALOG",
                        "Login nao validado; dialog sera reapresentado. Motivo: %s",
                        reason ? reason : "desconhecido");
}

void ClearLoginRefs(JNIEnv* env) {
    if (!env) return;
    if (g_loginUserField) {
        env->DeleteGlobalRef(g_loginUserField);
        g_loginUserField = nullptr;
    }
    if (g_loginPasswordField) {
        env->DeleteGlobalRef(g_loginPasswordField);
        g_loginPasswordField = nullptr;
    }
}

void ClearWarningRefs(JNIEnv* env) {
    if (!env) return;
    if (g_warningDialog) {
        env->DeleteGlobalRef(g_warningDialog);
        g_warningDialog = nullptr;
    }
}

bool IsDialogCurrentlyShowing(JNIEnv* env, jobject dialog) {
    if (!env || !dialog) return false;

    jclass dialogClass = env->GetObjectClass(dialog);
    if (!dialogClass) return false;

    jmethodID isShowing = env->GetMethodID(dialogClass, OBFUSCATE("isShowing"), OBFUSCATE("()Z"));
    if (!isShowing) return false;

    jboolean showing = env->CallBooleanMethod(dialog, isShowing);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return false;
    }
    return showing == JNI_TRUE;
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

void AttachWarningContent(JNIEnv* env, jobject context, jobject builder) {
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
    jobject titleView = CreateTextView(env, context, g_warningTitle, 20.0f, true);
    jobject messageView = CreateTextView(env, context, g_warningMessage, 14.0f, false);
    if (!badgeView || !titleView || !messageView) return;

    env->CallVoidMethod(layout, setOrientation, kLinearLayoutVertical);
    env->CallVoidMethod(layout, setPadding, 58, 46, 58, 26);
    ApplyRoundedBackground(env, layout, "#16110E", "#8A633B", 34.0f, 3);
    ApplyMargins(env, layout, 0, 8, 0, 0);
    ApplyRoundedBackground(env, badgeView, "#8A633B", "#B98A56", 24.0f, 0);
    ApplyMargins(env, badgeView, 120, 0, 120, 18);
    ApplyMargins(env, titleView, 0, 0, 0, 10);
    ApplyMargins(env, messageView, 0, 0, 0, 14);
    env->CallVoidMethod(layout, addView, badgeView);
    env->CallVoidMethod(layout, addView, titleView);
    env->CallVoidMethod(layout, addView, messageView);
    env->CallObjectMethod(builder, setView, layout);
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
    jobject userView = CreateEditText(env, context, "Email", kInputTypeClassText);
    jobject passwordView = CreateEditText(env, context, "Senha", kInputTypePassword);
    jobject helperView = CreateTextView(env, context, "Use sua conta Vinao Mods. A validacao acontece online com challenge seguro.", 12.0f, false);
    jobject endpointView = CreateTextView(env, context, "Servidor: vinaomods.vercel.app", 11.0f, false);
    if (!badgeView || !titleView || !messageView || !userView || !passwordView || !helperView || !endpointView) return;

    env->CallVoidMethod(layout, setOrientation, kLinearLayoutVertical);
    env->CallVoidMethod(layout, setPadding, 58, 46, 58, 26);
    ApplyRoundedBackground(env, layout, "#16110E", "#8A633B", 34.0f, 3);
    ApplyMargins(env, layout, 0, 8, 0, 0);
    ApplyRoundedBackground(env, badgeView, "#8A633B", "#B98A56", 24.0f, 0);
    ApplyMargins(env, badgeView, 120, 0, 120, 18);
    ApplyMargins(env, titleView, 0, 0, 0, 10);
    ApplyMargins(env, messageView, 0, 0, 0, 10);
    ApplyMargins(env, helperView, 0, 0, 0, 8);
    ApplyMargins(env, endpointView, 0, 0, 0, 24);
    env->CallVoidMethod(layout, addView, badgeView);
    env->CallVoidMethod(layout, addView, titleView);
    env->CallVoidMethod(layout, addView, messageView);
    env->CallVoidMethod(layout, addView, helperView);
    env->CallVoidMethod(layout, addView, endpointView);
    env->CallVoidMethod(layout, addView, userView);
    env->CallVoidMethod(layout, addView, passwordView);
    env->CallObjectMethod(builder, setView, layout);

    ClearLoginRefs(env);
    g_loginUserField = env->NewGlobalRef(userView);
    g_loginPasswordField = env->NewGlobalRef(passwordView);
}

bool IsJavaCharSequenceEmpty(JNIEnv* env, jobject charSequence) {
    if (!env || !charSequence) return true;

    jclass textUtilsClass = env->FindClass(OBFUSCATE("android/text/TextUtils"));
    if (!textUtilsClass) return true;

    jmethodID isEmpty = env->GetStaticMethodID(textUtilsClass, OBFUSCATE("isEmpty"),
                                               OBFUSCATE("(Ljava/lang/CharSequence;)Z"));
    if (!isEmpty) return true;

    jboolean empty = env->CallStaticBooleanMethod(textUtilsClass, isEmpty, charSequence);
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        return true;
    }
    return empty == JNI_TRUE;
}

const char* ReadEditTextValue(JNIEnv* env, jobject editText, char* buffer, size_t bufferSize) {
    if (!buffer || bufferSize == 0) return "";
    buffer[0] = '\0';
    if (!env || !editText) return buffer;

    jclass textViewClass = env->GetObjectClass(editText);
    if (!textViewClass) return buffer;

    jmethodID getText = env->GetMethodID(textViewClass, OBFUSCATE("getText"),
                                         OBFUSCATE("()Landroid/text/Editable;"));
    if (!getText) return buffer;

    jobject editable = env->CallObjectMethod(editText, getText);
    if (!editable) return buffer;

    jclass editableClass = env->GetObjectClass(editable);
    jmethodID toString = editableClass
                         ? env->GetMethodID(editableClass, OBFUSCATE("toString"), OBFUSCATE("()Ljava/lang/String;"))
                         : nullptr;
    if (!toString) return buffer;

    jstring textValue = static_cast<jstring>(env->CallObjectMethod(editable, toString));
    if (!textValue) return buffer;

    jclass stringClass = env->FindClass(OBFUSCATE("java/lang/String"));
    jmethodID trim = stringClass
                     ? env->GetMethodID(stringClass, OBFUSCATE("trim"), OBFUSCATE("()Ljava/lang/String;"))
                     : nullptr;
    jobject normalizedText = trim ? env->CallObjectMethod(textValue, trim) : textValue;
    if (!normalizedText || IsJavaCharSequenceEmpty(env, normalizedText)) {
        return buffer;
    }

    const char* chars = env->GetStringUTFChars(static_cast<jstring>(normalizedText), nullptr);
    if (!chars) {
        return buffer;
    }

    std::strncpy(buffer, chars, bufferSize - 1);
    buffer[bufferSize - 1] = '\0';
    env->ReleaseStringUTFChars(static_cast<jstring>(normalizedText), chars);
    return buffer;
}

bool AreCredentialsValid(JNIEnv* env) {
    if (!env || !g_loginUserField || !g_loginPasswordField) {
        __android_log_print(ANDROID_LOG_WARN, "MOD_DIALOG", "Validacao de login sem campos validos");
        CopyDialogText(g_toastMessage, sizeof(g_toastMessage),
                       "Falha interna ao abrir os campos de login.", "Falha interna ao abrir os campos de login.");
        g_toastPending = true;
        return false;
    }

    char user[64] = {0};
    char password[64] = {0};
    ReadEditTextValue(env, g_loginUserField, user, sizeof(user));
    ReadEditTextValue(env, g_loginPasswordField, password, sizeof(password));

    if (user[0] == '\0' || password[0] == '\0') {
        __android_log_print(ANDROID_LOG_WARN, "MOD_DIALOG", "Login rejeitado: usuario ou senha vazios");
        CopyDialogText(g_toastMessage, sizeof(g_toastMessage),
                       "Preencha email e senha para continuar.", "Preencha email e senha para continuar.");
        g_toastPending = true;
        return false;
    }

    std::string failureReason;
    const bool valid = PerformBackendLogin(env, g_dialogContext, user, password, &failureReason);
    if (!valid && !failureReason.empty()) {
        CopyDialogText(g_toastMessage, sizeof(g_toastMessage), failureReason.c_str(),
                       "Falha na autenticacao do mod.");
        g_toastPending = true;
    }
    __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG",
                        "Resultado validacao login: valid=%d user_len=%zu pass_len=%zu gen=%d",
                        valid ? 1 : 0, std::strlen(user), std::strlen(password), g_loginGeneration);
    return valid;
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
            RecoverLoginState(env, "classe do dialog indisponivel");
            break;
        }

        jmethodID isShowing = env->GetMethodID(dialogClass, OBFUSCATE("isShowing"), OBFUSCATE("()Z"));
        if (!isShowing) {
            RecoverLoginState(env, "metodo isShowing indisponivel");
            break;
        }

        jboolean showing = env->CallBooleanMethod(g_loginDialog, isShowing);
        if (env->ExceptionCheck()) {
            env->ExceptionClear();
            RecoverLoginState(env, "falha ao consultar estado do dialog");
            break;
        }

        if (showing == JNI_TRUE) {
            continue;
        }

        const int generationAtClose = g_loginGeneration;
        if (AreCredentialsValid(env)) {
            g_loginValidated = true;
            g_validatedGeneration = generationAtClose;
            g_dialogPending = false;
            g_dialogShown = true;
            ClearDialogRef(env);
            ClearLoginRefs(env);
            __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG", "Login do mod validado pelo backend");
            break;
        }

        RecoverLoginState(env, g_toastMessage[0] ? g_toastMessage : "Credenciais invalidas.");
        break;
    }

    g_loginWatcherStarted = false;
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

void ShowWarningDialog(JNIEnv* env, jobject context) {
    if (!env || !context || g_warningDialog) return;

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

    AttachWarningContent(env, context, builder);
    jstring okText = env->NewStringUTF("Entendi");
    env->CallObjectMethod(builder, setCancelable, JNI_FALSE);
    env->CallObjectMethod(builder, setPositiveButton, okText, nullptr);

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
            ApplyRoundedBackground(env, positiveButton, "#D9A35F", "#E8C18E", 24.0f, 0);
            ApplyMargins(env, positiveButton, 24, 0, 24, 10);
        }
    }

    ClearWarningRefs(env);
    g_warningDialog = env->NewGlobalRef(dialog);
    g_warningPending = false;
}

void ShowLoginDialog(JNIEnv* env, jobject context) {
    if (!env || !context) return;
    g_loginValidated = false;
    g_validatedGeneration = 0;
    ++g_loginGeneration;

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

    ClearDialogRef(env);
    g_loginDialog = env->NewGlobalRef(dialog);
    StartLoginWatcher(env);
}
} // namespace

bool IsDialogLoginValidated() {
    return g_loginValidated && g_validatedGeneration != 0 && g_validatedGeneration == g_loginGeneration;
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
                   "Informe email e senha da sua conta Vinao Mods para continuar.");
    g_dialogPending = true;
    g_dialogShown = false;
    g_loginValidated = false;
    g_validatedGeneration = 0;
    g_toastPending = false;
    g_modSessionToken[0] = '\0';
    g_modDeviceFingerprint[0] = '\0';
    g_loginDisplayName[0] = '\0';
    g_loginPolicyExpiresAt[0] = '\0';
    g_loginSuccessSummary[0] = '\0';
    g_loginRemainingDays = -1;
}

void ShowQueuedLibLoadDialog(JNIEnv* env, jobject context) {
    if (!env || !context) return;

    if (g_toastPending) {
        Toast(env, context, g_toastMessage, 1);
        g_toastPending = false;
    }

    if (!g_dialogPending || g_loginValidated) return;
    if (g_loginDialog) return;

    ShowLoginDialog(env, context);
    if (env->ExceptionCheck()) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_DIALOG", "Falha ao mostrar login");
        env->ExceptionClear();
        return;
    }

    __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG", "Dialog de login exibido");
}

void ShowQueuedLibLoadDialogWithRegisteredContext(JNIEnv* env) {
    if (!env || !g_dialogContext) return;
    ShowQueuedLibLoadDialog(env, g_dialogContext);
}

const char* SubmitJavaLogin(JNIEnv* env, jobject context, const char* email, const char* password) {
    if (!env || !context) {
        return "Contexto de login invalido.";
    }

    RegisterDialogContext(env, context);
    g_loginValidated = false;
    g_validatedGeneration = 0;
    ++g_loginGeneration;

    const std::string safeEmail = email ? email : "";
    const std::string safePassword = password ? password : "";
    std::string failureReason;

    const bool valid = PerformBackendLogin(env, context, safeEmail, safePassword, &failureReason);
    if (valid) {
        g_dialogPending = false;
        g_dialogShown = true;
        g_loginValidated = true;
        g_validatedGeneration = g_loginGeneration;
        g_javaLoginError[0] = '\0';
        __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG", "Login Java validado com sucesso");
        return nullptr;
    }

    CopyDialogText(g_javaLoginError, sizeof(g_javaLoginError),
                   failureReason.empty() ? "Falha na autenticacao do mod." : failureReason.c_str(),
                   "Falha na autenticacao do mod.");
    __android_log_print(ANDROID_LOG_WARN, "MOD_DIALOG", "Login Java rejeitado: %s", g_javaLoginError);
    return g_javaLoginError;
}

const char* GetJavaLoginSuccessSummary() {
    return g_loginSuccessSummary[0] ? g_loginSuccessSummary : "{}";
}
