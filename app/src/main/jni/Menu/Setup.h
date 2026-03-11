#include <sstream>
#include "Menu/Menu.h"
#include "Menu/get_device_api_level_inlines.h"
#include "../DialogOnLoad.h"
#include <curl/curl.h>
using namespace std;
struct MemoryStruct {
    char *memory;
    size_t size;
} chunk;




jstring readFile(JNIEnv *env, jobject ctx, jstring filename) {
    // Obter a classe Context
    jclass contextClass = env->GetObjectClass(ctx);

    // Obter o método getFilesDir()
    jmethodID getFilesDirMethod = env->GetMethodID(contextClass, OBFUSCATE("getFilesDir"), OBFUSCATE("()Ljava/io/File;"));
    jobject filesDir = env->CallObjectMethod(ctx, getFilesDirMethod);

    // Obter a classe File
    jclass fileClass = env->FindClass(OBFUSCATE("java/io/File"));

    // Construtor da classe File (File parent, String child)
    jmethodID fileCtor = env->GetMethodID(fileClass, OBFUSCATE("<init>"), OBFUSCATE("(Ljava/io/File;Ljava/lang/String;)V"));

    // Criar um novo objeto File para o arquivo a ser lido
    jobject file = env->NewObject(fileClass, fileCtor, filesDir, filename);

    // Verificar se o arquivo existe
    jmethodID existsMethod = env->GetMethodID(fileClass, OBFUSCATE("exists"), OBFUSCATE("()Z"));
    jboolean exists = env->CallBooleanMethod(file, existsMethod);
    if (!exists) {
        return env->NewStringUTF("");
    }

    // Obter a classe FileInputStream
    jclass fisClass = env->FindClass(OBFUSCATE("java/io/FileInputStream"));

    // Construtor da classe FileInputStream (File file)
    jmethodID fisCtor = env->GetMethodID(fisClass, OBFUSCATE("<init>"), OBFUSCATE("(Ljava/io/File;)V"));

    // Criar um novo objeto FileInputStream para o arquivo
    jobject fis = env->NewObject(fisClass, fisCtor, file);

    // Obter a classe InputStreamReader
    jclass isrClass = env->FindClass(OBFUSCATE("java/io/InputStreamReader"));

    // Construtor da classe InputStreamReader (InputStream in)
    jmethodID isrCtor = env->GetMethodID(isrClass, OBFUSCATE("<init>"), OBFUSCATE("(Ljava/io/InputStream;)V"));

    // Criar um novo objeto InputStreamReader
    jobject isr = env->NewObject(isrClass, isrCtor, fis);

    // Obter a classe BufferedReader
    jclass brClass = env->FindClass(OBFUSCATE("java/io/BufferedReader"));

    // Construtor da classe BufferedReader (Reader in)
    jmethodID brCtor = env->GetMethodID(brClass, OBFUSCATE("<init>"), OBFUSCATE("(Ljava/io/Reader;)V"));

    // Criar um novo objeto BufferedReader
    jobject br = env->NewObject(brClass, brCtor, isr);

    // Obter o método readLine() da classe BufferedReader
    jmethodID readLineMethod = env->GetMethodID(brClass, OBFUSCATE("readLine"), OBFUSCATE("()Ljava/lang/String;"));

    // Criar um StringBuilder para armazenar o conteúdo lido
    jclass sbClass = env->FindClass(OBFUSCATE("java/lang/StringBuilder"));
    jmethodID sbCtor = env->GetMethodID(sbClass, OBFUSCATE("<init>"), OBFUSCATE("()V"));
    jobject sb = env->NewObject(sbClass, sbCtor);

    // Método append da classe StringBuilder
    jmethodID appendMethod = env->GetMethodID(sbClass, OBFUSCATE("append"), OBFUSCATE("(Ljava/lang/String;)Ljava/lang/StringBuilder;"));

    // Ler o arquivo linha por linha
    jstring line;
    while ((line = (jstring)env->CallObjectMethod(br, readLineMethod)) != NULL) {
        env->CallObjectMethod(sb, appendMethod, line);
        env->DeleteLocalRef(line);
    }

    // Converter o StringBuilder para String
    jmethodID toStringMethod = env->GetMethodID(sbClass, OBFUSCATE("toString"), OBFUSCATE("()Ljava/lang/String;"));
    jstring result = (jstring)env->CallObjectMethod(sb, toStringMethod);

    // Liberar referências locais
    env->DeleteLocalRef(sb);
    env->DeleteLocalRef(br);
    env->DeleteLocalRef(brClass);
    env->DeleteLocalRef(isr);
    env->DeleteLocalRef(isrClass);
    env->DeleteLocalRef(fis);
    env->DeleteLocalRef(fisClass);
    env->DeleteLocalRef(file);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(filesDir);
    env->DeleteLocalRef(contextClass);

    return result;
}


void writeFile(JNIEnv *env, jobject ctx, jstring filename, jstring content) {
    // Obter a classe Context
    jclass contextClass = env->GetObjectClass(ctx);

    // Obter o método getFilesDir()
    jmethodID getFilesDirMethod = env->GetMethodID(contextClass, OBFUSCATE("getFilesDir"), OBFUSCATE("()Ljava/io/File;"));
    jobject filesDir = env->CallObjectMethod(ctx, getFilesDirMethod);

    // Obter a classe File
    jclass fileClass = env->FindClass(OBFUSCATE("java/io/File"));

    // Construtor da classe File (File parent, String child)
    jmethodID fileCtor = env->GetMethodID(fileClass, OBFUSCATE("<init>"), OBFUSCATE("(Ljava/io/File;Ljava/lang/String;)V"));

    // Criar um novo objeto File para o arquivo a ser escrito
    jobject file = env->NewObject(fileClass, fileCtor, filesDir, filename);

    // Obter a classe FileOutputStream
    jclass fosClass = env->FindClass(OBFUSCATE("java/io/FileOutputStream"));

    // Construtor da classe FileOutputStream (File file)
    jmethodID fosCtor = env->GetMethodID(fosClass, OBFUSCATE("<init>"), OBFUSCATE("(Ljava/io/File;)V"));

    // Criar um novo objeto FileOutputStream para o arquivo
    jobject fos = env->NewObject(fosClass, fosCtor, file);

    // Obter o método write(byte[])
    jmethodID writeMethod = env->GetMethodID(fosClass, OBFUSCATE("write"), OBFUSCATE("([B)V"));

    // Converter o conteúdo (jstring) em um array de bytes
    const char *nativeContent = env->GetStringUTFChars(content, NULL);
    jbyteArray byteArray = env->NewByteArray(strlen(nativeContent));
    env->SetByteArrayRegion(byteArray, 0, strlen(nativeContent), (jbyte *)nativeContent);

    // Escrever no arquivo
    env->CallVoidMethod(fos, writeMethod, byteArray);

    // Liberar recursos
    env->ReleaseStringUTFChars(content, nativeContent);
    env->DeleteLocalRef(byteArray);
    env->DeleteLocalRef(fos);
    env->DeleteLocalRef(file);
    env->DeleteLocalRef(fileClass);
    env->DeleteLocalRef(fosClass);
    env->DeleteLocalRef(filesDir);
    env->DeleteLocalRef(contextClass);
}


static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;
    mem->memory = (char *) realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory == NULL) {
        return 0;
    }
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    return realsize;
}

std::string getVersion(const std::string& url) {
    CURL* curl;
    CURLcode res;
    struct MemoryStruct chunk;
    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    std::string response;
    LOGD("Iniciando a função de obter versão");

    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        LOGE("curl_global_init() failed: %s", curl_easy_strerror(res));
        return "";
    }
    LOGD("curl_global_init");

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MyApp/1.0");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOGE("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
            LOGD("Response: %s", chunk.memory);

            response = std::string(chunk.memory);
            LOGD("Obter versão com sucesso");
        }

        curl_easy_cleanup(curl);
        LOGD("Cleanup curl");
    } else {
        LOGE("curl_easy_init() failed");
    }

    curl_global_cleanup();
    LOGD("Cleanup global curl");

    return response;
}
// Função para fazer login
std::string login(const std::string& url, const std::string& username, const std::string& password) {
    CURL* curl;
    CURLcode res;
    bool loginSuccess = false;
    chunk.memory = (char *)malloc(1);
    chunk.size = 0;

    std::string response;
    LOGD("Iniciando a função de login");

    std::string postFields = "username=" + username + "&password=" + password;
    LOGD("Post fields: %s", postFields.c_str());

    res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        LOGE("curl_global_init() failed: %s", curl_easy_strerror(res));
    }
    LOGD("curl_global_init");

    curl = curl_easy_init();
    if (curl) {
        std::string readBuffer;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        LOGD("Set POST fields");
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "MyApp/1.0");
        LOGD("Set User-Agent: MyApp/1.0");

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Custom-Header: CustomValue");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        LOGD("Set custom header: Custom-Header: CustomValue");

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            LOGE("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
            LOGD("Response: %s", chunk.memory);

            response = std::string(chunk.memory);
            LOGD("Login successful");
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        LOGD("Cleanup curl");
    } else {
        LOGE("curl_easy_init() failed");
    }

    curl_global_cleanup();
    LOGD("Cleanup global curl");

    return response;
}

// Jni stuff from MrDarkRX https://github.com/MrDarkRXx/DarkMod-Floating
void setDialog(jobject ctx, JNIEnv *env, const char *title, const char *msg){
    jclass Alert = env->FindClass(OBFUSCATE("android/app/AlertDialog$Builder"));
    jmethodID AlertCons = env->GetMethodID(Alert, OBFUSCATE("<init>"), OBFUSCATE("(Landroid/content/Context;)V"));

    jobject MainAlert = env->NewObject(Alert, AlertCons, ctx);

    jmethodID setTitle = env->GetMethodID(Alert, OBFUSCATE("setTitle"), OBFUSCATE("(Ljava/lang/CharSequence;)Landroid/app/AlertDialog$Builder;"));
    env->CallObjectMethod(MainAlert, setTitle, env->NewStringUTF(title));

    jmethodID setMsg = env->GetMethodID(Alert, OBFUSCATE("setMessage"), OBFUSCATE("(Ljava/lang/CharSequence;)Landroid/app/AlertDialog$Builder;"));
    env->CallObjectMethod(MainAlert, setMsg, env->NewStringUTF(msg));

    jmethodID setCa = env->GetMethodID(Alert, OBFUSCATE("setCancelable"), OBFUSCATE("(Z)Landroid/app/AlertDialog$Builder;"));
    env->CallObjectMethod(MainAlert, setCa, false);

    jmethodID setPB = env->GetMethodID(Alert, OBFUSCATE("setPositiveButton"), OBFUSCATE("(Ljava/lang/CharSequence;Landroid/content/DialogInterface$OnClickListener;)Landroid/app/AlertDialog$Builder;"));
    env->CallObjectMethod(MainAlert, setPB, env->NewStringUTF("Ok"), static_cast<jobject>(NULL));

    jmethodID create = env->GetMethodID(Alert, OBFUSCATE("create"), OBFUSCATE("()Landroid/app/AlertDialog;"));
    jobject creaetob = env->CallObjectMethod(MainAlert, create);

    jclass AlertN = env->FindClass(OBFUSCATE("android/app/AlertDialog"));

    jmethodID show = env->GetMethodID(AlertN, OBFUSCATE("show"), OBFUSCATE("()V"));
    env->CallVoidMethod(creaetob, show);
}

void Toast(JNIEnv *env, jobject thiz, const char *text, int length) {
    jstring jstr = env->NewStringUTF(text);
    jclass toast = env->FindClass(OBFUSCATE("android/widget/Toast"));
    jmethodID methodMakeText = env->GetStaticMethodID(toast, OBFUSCATE("makeText"), OBFUSCATE("(Landroid/content/Context;Ljava/lang/CharSequence;I)Landroid/widget/Toast;"));
    jobject toastobj = env->CallStaticObjectMethod(toast, methodMakeText, thiz, jstr, length);
    jmethodID methodShow = env->GetMethodID(toast, OBFUSCATE("show"), OBFUSCATE("()V"));
    env->CallVoidMethod(toastobj, methodShow);
}

void GetAppVersion(JNIEnv *env, jobject thiz) {
    // Obter Context
    jclass contextClass = env->GetObjectClass(thiz);

    // Obter o PackageManager
    jmethodID getPackageManager = env->GetMethodID(contextClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    jobject packageManager = env->CallObjectMethod(thiz, getPackageManager);

    // Obter o nome do pacote
    jmethodID getPackageName = env->GetMethodID(contextClass, "getPackageName", "()Ljava/lang/String;");
    jstring packageName = (jstring)env->CallObjectMethod(thiz, getPackageName);

    // Obter PackageInfo
    jclass packageManagerClass = env->GetObjectClass(packageManager);
    jmethodID getPackageInfo = env->GetMethodID(packageManagerClass, "getPackageInfo", "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    jobject packageInfo = env->CallObjectMethod(packageManager, getPackageInfo, packageName, 0);

    // Obter versionCode e versionName
    jclass packageInfoClass = env->GetObjectClass(packageInfo);
    jfieldID versionCodeField = env->GetFieldID(packageInfoClass, "versionCode", "I");
    jint versionCode = env->GetIntField(packageInfo, versionCodeField);

    jfieldID versionNameField = env->GetFieldID(packageInfoClass, "versionName", "Ljava/lang/String;");
    jstring versionName = (jstring)env->GetObjectField(packageInfo, versionNameField);

    // Converter versionName para C string
    const char *versionNameStr = env->GetStringUTFChars(versionName, NULL);

    // Exibir versionCode e versionName
    char versionInfo[256];
    sprintf(versionInfo, "Version Code: %d\nVersion Name: %s", versionCode, versionNameStr);
    Toast(env, thiz, versionInfo, 1);

    // Liberar a string
    env->ReleaseStringUTFChars(versionName, versionNameStr);
}



void startActivityPermission(JNIEnv *env, jobject ctx){
    jclass native_context = env->GetObjectClass(ctx);
    jmethodID startActivity = env->GetMethodID(native_context, OBFUSCATE("startActivity"), OBFUSCATE("(Landroid/content/Intent;)V"));

    jmethodID pack = env->GetMethodID(native_context, OBFUSCATE("getPackageName"), OBFUSCATE("()Ljava/lang/String;"));
    jstring packageName = static_cast<jstring>(env->CallObjectMethod(ctx, pack));

    const char *pkg = env->GetStringUTFChars(packageName, 0);

    std::stringstream pkgg;
    pkgg << OBFUSCATE("package:");
    pkgg << pkg;
    std::string pakg = pkgg.str();

    jclass Uri = env->FindClass(OBFUSCATE("android/net/Uri"));
    jmethodID Parce = env->GetStaticMethodID(Uri, OBFUSCATE("parse"), OBFUSCATE("(Ljava/lang/String;)Landroid/net/Uri;"));
    jobject UriMethod = env->CallStaticObjectMethod(Uri, Parce, env->NewStringUTF(pakg.c_str()));

    jclass intentclass = env->FindClass(OBFUSCATE("android/content/Intent"));
    jmethodID newIntent = env->GetMethodID(intentclass, OBFUSCATE("<init>"), OBFUSCATE("(Ljava/lang/String;Landroid/net/Uri;)V"));
    jobject intent = env->NewObject(intentclass, newIntent, env->NewStringUTF(OBFUSCATE("android.settings.action.MANAGE_OVERLAY_PERMISSION")), UriMethod);

    env->CallVoidMethod(ctx, startActivity, intent);
}

void startService(JNIEnv *env, jobject ctx){
    jclass native_context = env->GetObjectClass(ctx);
    jclass intentClass = env->FindClass(OBFUSCATE("android/content/Intent"));
    jclass actionString = env->FindClass(OBFUSCATE("vdev/com/android/support/Launcher"));
    jmethodID newIntent = env->GetMethodID(intentClass, OBFUSCATE("<init>"), OBFUSCATE("(Landroid/content/Context;Ljava/lang/Class;)V"));
    jobject intent = env->NewObject(intentClass, newIntent, ctx, actionString);
    jmethodID startActivityMethodId = env->GetMethodID(native_context, OBFUSCATE("startService"), OBFUSCATE("(Landroid/content/Intent;)Landroid/content/ComponentName;"));
    env->CallObjectMethod(ctx, startActivityMethodId, intent);
}



void *exit_thread(void *) {
    sleep(5);
    exit(0);
}

//Needed jclass parameter because this is a static java method
void CheckOverlayPermission(JNIEnv *env, jclass thiz, jobject ctx){
    //If overlay permission option is greyed out, make sure to add android.permission.SYSTEM_ALERT_WINDOW in manifest

    LOGI(OBFUSCATE("Check overlay permission"));

    int sdkVer = api_level();
    if (sdkVer >= 23){ //Android 6.0
        jclass Settings = env->FindClass(OBFUSCATE("android/provider/Settings"));
        jmethodID canDraw = env->GetStaticMethodID(Settings, OBFUSCATE("canDrawOverlays"), OBFUSCATE("(Landroid/content/Context;)Z"));
        if (!env->CallStaticBooleanMethod(Settings, canDraw, ctx)){
            Toast(env, ctx, OBFUSCATE("Overlay permission is required in order to show mod menu."), 1);
            startActivityPermission(env, ctx);

            pthread_t ptid;
            pthread_create(&ptid, NULL, exit_thread, NULL);
            return;
        }
    }

    LOGI(OBFUSCATE("Start service"));

    //StartMod Normal
    startService(env, ctx);
}

void Init(JNIEnv *env, jobject thiz, jobject ctx, jobject title, jobject subtitle){
    RegisterDialogContext(env, ctx);
    ShowQueuedLibLoadDialog(env, ctx);

    setText(env, title, OBFUSCATE("West Gunfighter"));
    setText(env, subtitle, OBFUSCATE("<b><marquee><p style=\"font-size:30\">"
                                     "<p style=\"color:green;\">Mod by Vdev</p> "
                                     "</marquee></b>"));
    //Dialog Example
    //setDialog(ctx, env, OBFUSCATE("Dialog da Lib"), OBFUSCATE("Lib carregada Versão 1.1.1"));

    //Toast Example
    //Toast(env, ctx, OBFUSCATE("Modded by VDEV"), ToastLength::LENGTH_LONG);
    //Toast(env, ctx, OBFUSCATE("lib version 1.0"), ToastLength::LENGTH_LONG);
    //Toast(env, ctx, OBFUSCATE("lib version 1.0"), ToastLength::LENGTH_LONG);

    std::string username = "9778";
    std::string password = "9778";

    GetAppVersion(env,ctx);
    // Faz login
    //std::string response = login(OBFUSCATE("https://remotelibrary.viniciusdev.com.br/login.php"), username, password);
    //Toast(env, ctx, response.c_str(), ToastLength::LENGTH_LONG);

    initValid = true;
}
