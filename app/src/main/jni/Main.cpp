#include <list>
#include <vector>
#include <cstring>
#include <pthread.h>
#include <thread>
#include <cstring>
#include <jni.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <dlfcn.h>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "KittyMemory/MemoryPatch.h"
#include "Menu/Setup.h"


#define targetLibName OBFUSCATE("libil2cpp.so")

#include "Includes/Macros.h"


bool feature2, featureHookToggle, Health, UnlockArmas, LevelMaximo;
int sliderValue = 1, level = 0, Moedas = 0, Gems = 0;
bool MatarTodosInimigos = false;
void *instanceBtn;
struct MyPlayerRealtimeData {
    int maxBlood;    // Assumindo que maxBlood está no offset 0x8
    int currentBlood;// Assumindo que currentBlood está no offset 0xC
};
JavaVM *g_JavaVM;
jobject g_AppContext;

typedef void (*SaveGoldFunc)(int);

typedef void (*SaveGemFunc)(int);

typedef void (*SaveRifleBulletFunc)(int);

typedef void (*SaveShotgunBulletFunc)(int);

typedef void (*SavePistolBulletFunc)(int);

void CallSavePistolBullet(int bulletAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52B9DC);
    auto savePistolBullet = reinterpret_cast<SavePistolBulletFunc>(baseAddress); // Ajuste com base no endereço base

    savePistolBullet(bulletAmount);

    __android_log_print(ANDROID_LOG_INFO, "ModMenu",
                        "Chamada SavePistolBullet com sucesso. Valor: %d", bulletAmount);
}

void CallSaveRifleBullet(int bulletAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52BB24);
    auto saveRifleBullet = reinterpret_cast<SaveRifleBulletFunc>(baseAddress);

    saveRifleBullet(bulletAmount);

    __android_log_print(ANDROID_LOG_INFO, "ModMenu",
                        "Chamada SaveRifleBullet com sucesso. Valor: %d", bulletAmount);
}

void CallSaveShotgunBullet(int bulletAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52BC6C);
    auto saveShotgunBullet = reinterpret_cast<SaveShotgunBulletFunc>(baseAddress);

    saveShotgunBullet(bulletAmount);

    __android_log_print(ANDROID_LOG_INFO, "ModMenu",
                        "Chamada SaveShotgunBullet com sucesso. Valor: %d", bulletAmount);
}

void CallSaveGold(int goldAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52AF9C);
    auto saveGold = reinterpret_cast<SaveGoldFunc>(baseAddress); // Offset ajustado ao base address do Unity

    saveGold(goldAmount);

    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Chamada SaveGold com sucesso. Valor: %d",
                        goldAmount);
}

void CallSaveGem(int goldAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52B040);
    auto saveGem = reinterpret_cast<SaveGemFunc>(baseAddress); // Offset ajustado ao base address do Unity

    saveGem(goldAmount);

    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Chamada SaveGem com sucesso. Valor: %d",
                        goldAmount);
}


MyPlayerRealtimeData *(*original_GetMyPlayerRealtimeData)();

MyPlayerRealtimeData *hook_GetMyPlayerRealtimeData() {
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Acessando MyPlayerRealtimeData...");
    MyPlayerRealtimeData *data = original_GetMyPlayerRealtimeData();
    if (!data) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD",
                            "Erro: Ponteiro para MyPlayerRealtimeData é nulo.");
        return nullptr;
    }
    int *maxBloodPtr = reinterpret_cast<int *>(reinterpret_cast<char *>(data) + 0x8);
    int *currentBloodPtr = reinterpret_cast<int *>(reinterpret_cast<char *>(data) + 0xC);
    if (Health) {
        *maxBloodPtr = 999;
        *currentBloodPtr = 999;
    }
    __android_log_print(ANDROID_LOG_DEBUG, "MOD",
                        "Valores modificados: maxBlood=%d, currentBlood=%d", *maxBloodPtr,
                        *currentBloodPtr);
    return data;
}


// Estrutura para List<T> similar à do Unity/IL2CPP
struct Il2CppArray {
    void* klass;
    void* monitor;
    void* bounds;
    int   max_length;
    void* vector[0];
};

struct List_PlayerBase {
    void* klass;
    void* monitor;
    Il2CppArray* _items;
    int _size;
    int _version;
    void* _syncRoot;
};
int getListSize(void* list) {
    if (list == NULL) return 0;
    List_PlayerBase* typedList = (List_PlayerBase*)list;
    return typedList->_size;
}

// Função para obter um item de uma lista
void* getListItem(void* list, int index) {
    if (list == NULL) return NULL;
    List_PlayerBase* typedList = (List_PlayerBase*)list;
    if (index < 0 || index >= typedList->_size || typedList->_items == NULL) return NULL;
    return typedList->_items->vector[index];
}
void* getPlayerRealtimeData(void* player) {
    if (player == NULL) return NULL;

    // Baseado no código, parece que poderíamos precisar chamar um método como GetMyPlayerRealtimeData
    // Vamos assumir que este método está em um offset conhecido ou acessível via vtable

    // Abordagem 1: Se houver um campo direto para realtimeData
    return *(void**)((uint64_t)player + 0x48); // Offset hipotético, ajuste conforme necessário

    // Abordagem 2: Se precisar chamar um método
    // return ((void* (*)(void*))getAbsoluteAddress(targetLibName, 0x490C28))(player);
}
void matarTodosOsInimigos() {
    if (!MatarTodosInimigos) return;

    // Obter GameCtrl instance
    void* gameCtrl = ((void* (*)())getAbsoluteAddress(targetLibName, 0x2DDD44))();
    if (!gameCtrl) return;

    // Obter lista de inimigos ativos
    void* enemyList = ((void* (*)(void*))getAbsoluteAddress(targetLibName, 0x2F1D7C))(gameCtrl);
    if (!enemyList) return;

    LOGI(OBFUSCATE("Obtendo lista de inimigos"));

    int count = getListSize(enemyList);
    LOGI(OBFUSCATE("Número de inimigos: %d"), count);

    for (int i = 0; i < count; i++) {
        void* enemy = getListItem(enemyList, i);
        if (enemy) {
            LOGI(OBFUSCATE("Processando inimigo %d"), i);

            // Definir saúde como 0 para matar
            void* realtimeData = getPlayerRealtimeData(enemy);
            if (realtimeData) {
                LOGI(OBFUSCATE("Definindo vida como 0"));
                *(int*)((uint64_t)realtimeData + 0xC) = 0; // currentBlood = 0

                // Forçar animação de morte
                // Isso invoca diretamente a função de morte do inimigo (PlayReadyToDie_Player)
                LOGI(OBFUSCATE("Invocando animação de morte"));
                ((void (*)(void*))getAbsoluteAddress(targetLibName, 0x456BD8))(enemy);
            }
        }
    }

    LOGI(OBFUSCATE("Todos os inimigos foram eliminados"));
}


int (*original_GetHitBlood)(void *thisPtr, int part, int type, float enemy, float myPosition,
                            int modelType);

int
hook_GetHitBlood(void *thisPtr, int part, int type, float enemy, float myPosition, int modelType) {
    // Logar a chamada
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Entrada na função GetHitBlood");

    // Chamada da função original
    int result = original_GetHitBlood(thisPtr, part, type, enemy, myPosition, modelType);

    // Logar o resultado e substituir
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Dano calculado: %d", result);
    return sliderValue; // Retornar um valor modificado
}


void (*AddMoneyExample)(void *instance, int amount);

bool (*old_get_BoolExample)(void *instance);

bool get_BoolExample(void *instance) {
    if (instance != NULL && featureHookToggle) {
        return true;
    }
    return old_get_BoolExample(instance);
}

float (*old_get_FloatExample)(void *instance);

float get_FloatExample(void *instance) {
    if (instance != NULL && sliderValue > 1) {
        return (float) sliderValue;
    }
    return old_get_FloatExample(instance);
}

int (*old_Level)(void *instance);

int Level(void *instance) {
    if (instance != NULL && level) {
        return (int) level;
    }
    return old_Level(instance);
}

void (*old_FunctionExample)(void *instance);

void FunctionExample(void *instance) {
    instanceBtn = instance;
    if (instance != NULL) {
        if (Health) {
            *(int *) ((uint64_t) instance + 0x48) = 999;
        }
    }
    return old_FunctionExample(instance);
}

// we will run our hacks in a new thread so our while loop doesn't block process main thread
void *hack_thread(void *) {
    LOGI(OBFUSCATE("pthread created"));

    //Check if target lib is loaded
    do {
        sleep(1);
    } while (!isLibraryLoaded(targetLibName));

    //Anti-lib rename
    /*
    do {
        sleep(1);
    } while (!isLibraryLoaded("libYOURNAME.so"));*/

    LOGI(OBFUSCATE("%s has been loaded"), (const char *) targetLibName);

#if defined(__aarch64__) //To compile this code for arm64 lib only. Do not worry about greyed out highlighting code, it still works
    // Hook example. Comment out if you don't use hook
    // Strings in macros are automatically obfuscated. No need to obfuscate!
    HOOK("str", FunctionExample, old_FunctionExample);
    HOOK_LIB("libFileB.so", "0x123456", FunctionExample, old_FunctionExample);
    HOOK_NO_ORIG("0x123456", FunctionExample);
    HOOK_LIB_NO_ORIG("libFileC.so", "0x123456", FunctionExample);
    HOOKSYM("__SymbolNameExample", FunctionExample, old_FunctionExample);
    HOOKSYM_LIB("libFileB.so", "__SymbolNameExample", FunctionExample, old_FunctionExample);
    HOOKSYM_NO_ORIG("__SymbolNameExample", FunctionExample);
    HOOKSYM_LIB_NO_ORIG("libFileB.so", "__SymbolNameExample", FunctionExample);

    // Patching offsets directly. Strings are automatically obfuscated too!
    PATCH("0x20D3A8", "00 00 A0 E3 1E FF 2F E1");
    PATCH_LIB("libFileB.so", "0x20D3A8", "00 00 A0 E3 1E FF 2F E1");

    AddMoneyExample = (void(*)(void *,int))getAbsoluteAddress(targetLibName, 0x123456);

#else //To compile this code for armv7 lib only.



    if(sliderValue > 1){
        ///HOOKS DE DANO

    }


    uintptr_t addr_GetHitBlood = getAbsoluteAddress(targetLibName, 0x4591F8);
    MSHookFunction((void *) addr_GetHitBlood, (void *) &hook_GetHitBlood,
                   (void **) &original_GetHitBlood);

    ///HOOKS DE VIDA EM 9999


    uintptr_t addr_GetMyPlayerRealtimeData = getAbsoluteAddress(targetLibName, 0x490C28);
    MSHookFunction((void *) addr_GetMyPlayerRealtimeData, (void *) &hook_GetMyPlayerRealtimeData,
                   (void **) &original_GetMyPlayerRealtimeData);


    LOGI(OBFUSCATE("Done"));
#endif

    //Anti-leech
    /*if (!iconValid || !initValid || !settingsValid) {
        //Bad function to make it crash
        sleep(5);
        int *p = 0;
        *p = 0;
    }*/

    return NULL;
}

// Do not change or translate the first text unless you know what you are doing
// Assigning feature numbers is optional. Without it, it will automatically count for you, starting from 0
// Assigned feature numbers can be like any numbers 1,3,200,10... instead in order 0,1,2,3,4,5...
// ButtonLink, Category, RichTextView and RichWebView is not counted. They can't have feature number assigned
// Toggle, ButtonOnOff and Checkbox can be switched on by default, if you add True_. Example: CheckBox_True_The Check Box
// To learn HTML, go to this page: https://www.w3schools.com/

jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
            OBFUSCATE("Category_Nunca pensou em dar para um amigo"), //Not counted
            OBFUSCATE("4_Toggle_Vida Infinita"),
            OBFUSCATE("1_SeekBar_Dano de bala_1_999"),
            OBFUSCATE("2_InputValue_Adicionar Moedas"),
            OBFUSCATE("3_InputValue_Adicionar Gems"),
            OBFUSCATE("5_SeekBar_Balas das Armas_1_999999"),
            OBFUSCATE("6_Toggle_Matar Todos Inimigos"),


    };

    //Now you dont have to manually update the number everytime;
    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
}

uintptr_t addr_SaveGold = 0x52AF9C; // Supondo que esse seja o endereço correto no ambiente alvo



// Chamar a função 'SaveGold' com um valor exemplo

void Changes(JNIEnv *env, jclass clazz, jobject obj,
             jint featNum, jstring featName, jint value,
             jboolean boolean, jstring str) {

    LOGD(OBFUSCATE("Feature name: %d - %s | Value: = %d | Bool: = %d | Text: = %s"), featNum,
         env->GetStringUTFChars(featName, 0), value,
         boolean, str != NULL ? env->GetStringUTFChars(str, 0) : "");

    //BE CAREFUL NOT TO ACCIDENTLY REMOVE break;

    switch (featNum) {
        case 1:
            if (value >= 1) {
                sliderValue = value;
            }
            break;
        case 2:
            if (value >= 1) {
                Moedas = value;
                CallSaveGold(value); // Chamando com o valor 1000
            }
            break;
        case 3:
            if (value >= 1) {
                Gems = value;
                CallSaveGem(value);
            }
            break;
        case 4:
            Health = boolean;
            break;
        case 5://balas shotgun
            CallSaveShotgunBullet(value);
            CallSaveRifleBullet(value);
            CallSavePistolBullet(value);

            break;
        case 6:
            MatarTodosInimigos = boolean;
            break;

    }
}

__attribute__((constructor))
void lib_main() {
    // Create a new thread so it does not block the main thread, means the game would not freeze
    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);
}

int RegisterMenu(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Icon"),            OBFUSCATE(
                                                   "()Ljava/lang/String;"),                                                           reinterpret_cast<void *>(Icon)},
            {OBFUSCATE("IconWebViewData"), OBFUSCATE(
                                                   "()Ljava/lang/String;"),                                                           reinterpret_cast<void *>(IconWebViewData)},
            {OBFUSCATE("IsGameLibLoaded"), OBFUSCATE(
                                                   "()Z"),                                                                            reinterpret_cast<void *>(isGameLibLoaded)},
            {OBFUSCATE("Init"),            OBFUSCATE(
                                                   "(Landroid/content/Context;Landroid/widget/TextView;Landroid/widget/TextView;)V"), reinterpret_cast<void *>(Init)},
            {OBFUSCATE("SettingsList"),    OBFUSCATE(
                                                   "()[Ljava/lang/String;"),                                                          reinterpret_cast<void *>(SettingsList)},
            {OBFUSCATE("GetFeatureList"),  OBFUSCATE(
                                                   "()[Ljava/lang/String;"),                                                          reinterpret_cast<void *>(GetFeatureList)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/Menu"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;
    return JNI_OK;
}

int RegisterPreferences(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Changes"),
             OBFUSCATE("(Landroid/content/Context;ILjava/lang/String;IZLjava/lang/String;)V"),
             reinterpret_cast<void *>(Changes)},
    };
    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/Preferences"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;
    return JNI_OK;
}

int RegisterMain(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("CheckOverlayPermission"), OBFUSCATE("(Landroid/content/Context;)V"),
             reinterpret_cast<void *>(CheckOverlayPermission)},
    };
    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/Main"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;

    return JNI_OK;
}




// Global application context

extern "C"
JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env;
    vm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (RegisterMenu(env) != 0)
        return JNI_ERR;
    if (RegisterPreferences(env) != 0)
        return JNI_ERR;
    if (RegisterMain(env) != 0)
        return JNI_ERR;
    return JNI_VERSION_1_6;
}
