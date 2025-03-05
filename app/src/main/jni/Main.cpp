#include <pthread.h>
#include <jni.h>
#include <unistd.h>
#include <android/log.h>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "KittyMemory/MemoryPatch.h"
#include "Menu/Setup.h"

// Define o nome da biblioteca alvo
#define targetLibName OBFUSCATE("libil2cpp.so")

#include "Includes/Macros.h"

// Enumeração de tipos de itens do jogo
enum DropGoodsType {
    Null = 0,
    BloodVial = 1,
    BigBloodVial = 2,
    PistolAmmo = 3,
    ShotgunAmmo = 4,
    RifleAmmo = 5,
    DeerSkin = 6,
    CheetahSkin = 7,
    BearSkin = 8,
    WolfSkin = 9,
    FoxSkin = 10,
    GunPart1 = 11,
    GunPart2 = 12,
    GunPart3 = 13,
    GunPart4 = 14,
    WHISKY = 15,
    Gold = 16,
    Diamond = 17
};

// Variáveis globais para controlar recursos
bool Health = false;
bool debugEnemyPos = false;
bool infiniteGold = false;
bool infiniteAmmo = false;
bool infiniteHealth = false;
bool infiniteResources = false;
int sliderValue = 1, Moedas = 0, Gems = 0;

// Estrutura para dados do jogador em tempo real
struct MyPlayerRealtimeData {
    int maxBlood;    // Vida máxima no offset 0x8
    int currentBlood;// Vida atual no offset 0xC
};

// Estrutura para representar Vector3
struct Vector3 {
    float x;
    float y;
    float z;
};

// Tipos de função para as chamadas de salvamento
typedef void (*SaveGoldFunc)(int);
typedef void (*SaveGemFunc)(int);
typedef void (*SaveRifleBulletFunc)(int);
typedef void (*SaveShotgunBulletFunc)(int);
typedef void (*SavePistolBulletFunc)(int);

/**
 * Salva a quantidade de munição de pistola
 * @param bulletAmount Quantidade de munição
 */
void CallSavePistolBullet(int bulletAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52B9DC);
    auto savePistolBullet = reinterpret_cast<SavePistolBulletFunc>(baseAddress);
    savePistolBullet(bulletAmount);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Chamada SavePistolBullet com sucesso. Valor: %d", bulletAmount);
}

/**
 * Salva a quantidade de munição de rifle
 * @param bulletAmount Quantidade de munição
 */
void CallSaveRifleBullet(int bulletAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52BB24);
    auto saveRifleBullet = reinterpret_cast<SaveRifleBulletFunc>(baseAddress);
    saveRifleBullet(bulletAmount);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Chamada SaveRifleBullet com sucesso. Valor: %d", bulletAmount);
}

/**
 * Salva a quantidade de munição de espingarda
 * @param bulletAmount Quantidade de munição
 */
void CallSaveShotgunBullet(int bulletAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52BC6C);
    auto saveShotgunBullet = reinterpret_cast<SaveShotgunBulletFunc>(baseAddress);
    saveShotgunBullet(bulletAmount);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Chamada SaveShotgunBullet com sucesso. Valor: %d", bulletAmount);
}

/**
 * Salva a quantidade de ouro
 * @param goldAmount Quantidade de ouro
 */
void CallSaveGold(int goldAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52AF9C);
    auto saveGold = reinterpret_cast<SaveGoldFunc>(baseAddress);
    saveGold(goldAmount);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Chamada SaveGold com sucesso. Valor: %d", goldAmount);
}

/**
 * Salva a quantidade de gemas
 * @param gemAmount Quantidade de gemas
 */
void CallSaveGem(int gemAmount) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52B040);
    auto saveGem = reinterpret_cast<SaveGemFunc>(baseAddress);
    saveGem(gemAmount);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Chamada SaveGem com sucesso. Valor: %d", gemAmount);
}

/**
 * Adiciona um item específico ao inventário
 * @param goodType Tipo do item a ser adicionado
 * @param amount Quantidade a ser adicionada
 */
void AddItemToInventory(DropGoodsType goodType, int amount) {
    typedef int (*GetNumFunc)(int);
    typedef void (*SetNumFunc)(int, int);

    uintptr_t addr_GetNum = getAbsoluteAddress(targetLibName, 0x53B1A0); // GetDropGoodNumber
    uintptr_t addr_SetNum = getAbsoluteAddress(targetLibName, 0x53B71C); // SetDropGoodNumber

    auto getNum = reinterpret_cast<GetNumFunc>(addr_GetNum);
    auto setNum = reinterpret_cast<SetNumFunc>(addr_SetNum);

    int currentAmount = getNum((int)goodType);
    setNum((int)goodType, currentAmount + amount);

    __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Adicionado %d do item tipo %d. Total: %d",
                        amount, (int)goodType, currentAmount + amount);
}

// Ponteiro para função original GetMyPlayerRealtimeData
MyPlayerRealtimeData *(*original_GetMyPlayerRealtimeData)();

/**
 * Hook para a função GetMyPlayerRealtimeData
 * Modifica a vida do jogador quando o recurso está ativado
 */
MyPlayerRealtimeData *hook_GetMyPlayerRealtimeData() {
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Acessando MyPlayerRealtimeData...");
    MyPlayerRealtimeData *data = original_GetMyPlayerRealtimeData();
    if (!data) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD", "Erro: Ponteiro para MyPlayerRealtimeData é nulo.");
        return nullptr;
    }

    // Acessa os ponteiros para vida máxima e atual
    int *maxBloodPtr = reinterpret_cast<int *>(reinterpret_cast<char *>(data) + 0x8);
    int *currentBloodPtr = reinterpret_cast<int *>(reinterpret_cast<char *>(data) + 0xC);

    // Aplica vida infinita se a opção estiver ativada
    if (Health) {
        *maxBloodPtr = 999;
        *currentBloodPtr = 999;
    }

    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Valores: maxBlood=%d, currentBlood=%d",
                        *maxBloodPtr, *currentBloodPtr);
    return data;
}

// Ponteiro para função original GetHitBlood
int (*original_GetHitBlood)(void *thisPtr, int part, int type, float enemy, float myPosition, int modelType);

/**
 * Hook para a função GetHitBlood
 * Modifica o dano causado pelas balas
 */
int hook_GetHitBlood(void *thisPtr, int part, int type, float enemy, float myPosition, int modelType) {
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Entrada na função GetHitBlood");
    int result = original_GetHitBlood(thisPtr, part, type, enemy, myPosition, modelType);
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Dano calculado: %d", result);
    return sliderValue; // Retorna o valor definido pelo usuário no slider
}

// ================ HOOKS PARA SISTEMA DE POSIÇÃO DE INIMIGOS =================

// Ponteiro para função original GetEnemyBurnPos
Vector3 (*original_GetEnemyBurnPos)(void* thisPtr, int modelBonesType, int scenePosType);

/**
 * Hook para a função GetEnemyBurnPos
 * Monitora e loga as posições de inimigos
 */
Vector3 hook_GetEnemyBurnPos(void* thisPtr, int modelBonesType, int scenePosType) {
    // Obtem o resultado original
    Vector3 result = original_GetEnemyBurnPos(thisPtr, modelBonesType, scenePosType);

    // Se o debug estiver ativado, registra os valores no log
    if (debugEnemyPos) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_DEBUG",
                            "GetEnemyBurnPos: ModelBonesType=%d, ScenePosType=%d, Position=(%.2f, %.2f, %.2f)",
                            modelBonesType, scenePosType, result.x, result.y, result.z);
    }

    // Retorna o resultado original sem modificação
    return result;
}

// Ponteiro para função original GetPoliceBurnPos
Vector3 (*original_GetPoliceBurnPos)(void* thisPtr, Vector3 playerPos, float minSqr, float maxSqr);

/**
 * Hook para a função GetPoliceBurnPos
 * Monitora e loga as posições de policiais
 */
Vector3 hook_GetPoliceBurnPos(void* thisPtr, Vector3 playerPos, float minSqr, float maxSqr) {
    // Obtem o resultado original
    Vector3 result = original_GetPoliceBurnPos(thisPtr, playerPos, minSqr, maxSqr);

    // Se o debug estiver ativado, registra os valores no log
    if (debugEnemyPos) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_DEBUG",
                            "GetPoliceBurnPos: PlayerPos=(%.2f, %.2f, %.2f), MinSqr=%.2f, MaxSqr=%.2f, Result=(%.2f, %.2f, %.2f)",
                            playerPos.x, playerPos.y, playerPos.z, minSqr, maxSqr, result.x, result.y, result.z);
    }

    // Retorna o resultado original sem modificação
    return result;
}

// Ponteiro para função original ClearHasEnemy
void (*original_ClearHasEnemy)(void* thisPtr);

/**
 * Hook para a função ClearHasEnemy
 * Monitora quando os inimigos são limpos
 */
void hook_ClearHasEnemy(void* thisPtr) {
    // Log da chamada
    if (debugEnemyPos) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_DEBUG", "ClearHasEnemy foi chamado - Removendo inimigos");
    }

    // Chama a função original sem modificação
    original_ClearHasEnemy(thisPtr);
}

// Ponteiro para função original HasScenePosType
bool (*original_HasScenePosType)(void* thisPtr, int scenePosType);

/**
 * Hook para a função HasScenePosType
 * Monitora verificações de posições disponíveis
 */
bool hook_HasScenePosType(void* thisPtr, int scenePosType) {
    bool result = original_HasScenePosType(thisPtr, scenePosType);

    if (debugEnemyPos) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_DEBUG",
                            "HasScenePosType: ScenePosType=%d, Result=%s",
                            scenePosType, result ? "true" : "false");
    }

    return result;
}

// ================ HOOKS PARA SISTEMA DE ITENS DO JOGO =================

// Ponteiro para função original GetDropGoodNumber
int (*original_GetDropGoodNumber)(int goodType);

/**
 * Hook para a função GetDropGoodNumber
 * Modifica a quantidade de itens retornada
 */
int hook_GetDropGoodNumber(int goodType) {
    DropGoodsType type = (DropGoodsType)goodType;

    if (debugEnemyPos) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_ITEMS", "GetDropGoodNumber solicitado: Tipo=%d", goodType);
    }

    // Se o hack de recursos infinitos estiver ativado, retorna um valor alto para certos itens
    if (infiniteResources) {
        // Retorna valor alto para todos os recursos (peles, partes, etc)
        if (type >= DeerSkin && type <= GunPart4) {
            return 999;
        }
    }

    // Se estiver com hack de ouro/diamantes ativado
    if (infiniteGold && (type == Gold || type == Diamond)) {
        return 9999;
    }

    // Se estiver com hack de munição ativado
    if (infiniteAmmo && (type == PistolAmmo || type == ShotgunAmmo || type == RifleAmmo)) {
        return 9999;
    }

    // Se estiver com hack de vida ativado (via itens)
    if (infiniteHealth && (type == BloodVial || type == BigBloodVial)) {
        return 999;
    }

    // Caso contrário, chama a função original
    return original_GetDropGoodNumber(goodType);
}

// Ponteiro para função original SetDropGoodNumber
void (*original_SetDropGoodNumber)(int goodType, int num);

/**
 * Hook para a função SetDropGoodNumber
 * Monitora quando a quantidade de itens é alterada
 */
void hook_SetDropGoodNumber(int goodType, int num) {
    if (debugEnemyPos) {
        const char* itemNames[] = {
                "Null", "BloodVial", "BigBloodVial", "PistolAmmo", "ShotgunAmmo",
                "RifleAmmo", "DeerSkin", "CheetahSkin", "BearSkin", "WolfSkin",
                "FoxSkin", "GunPart1", "GunPart2", "GunPart3", "GunPart4",
                "WHISKY", "Gold", "Diamond"
        };

        const char* itemName = "Desconhecido";
        if (goodType >= 0 && goodType < 18) {
            itemName = itemNames[goodType];
        }

        __android_log_print(ANDROID_LOG_DEBUG, "MOD_ITEMS",
                            "SetDropGoodNumber: Item=%s (Tipo=%d), Quantidade=%d",
                            itemName, goodType, num);
    }

    // Chama a função original
    original_SetDropGoodNumber(goodType, num);
}

/**
 * Thread principal para aplicar os hacks
 * Espera que a biblioteca alvo seja carregada antes de aplicar os hooks
 */
void *hack_thread(void *) {
    LOGI(OBFUSCATE("Thread de hack criada"));

    // Aguarda o carregamento da biblioteca alvo
    do {
        sleep(1);
    } while (!isLibraryLoaded(targetLibName));

    LOGI(OBFUSCATE("%s foi carregada"), (const char *) targetLibName);

#if defined(__aarch64__) // Código para arquitetura ARM64
    // Hooks para ARM64 seriam definidos aqui

#else // Código para arquitetura ARMv7

    // ===== Hooks originais =====

    // Hook para modificar o dano das balas
    uintptr_t addr_GetHitBlood = getAbsoluteAddress(targetLibName, 0x4591F8);
    MSHookFunction((void *) addr_GetHitBlood, (void *) &hook_GetHitBlood,
                   (void **) &original_GetHitBlood);

    // Hook para modificar a vida do jogador
    uintptr_t addr_GetMyPlayerRealtimeData = getAbsoluteAddress(targetLibName, 0x490C28);
    MSHookFunction((void *) addr_GetMyPlayerRealtimeData, (void *) &hook_GetMyPlayerRealtimeData,
                   (void **) &original_GetMyPlayerRealtimeData);

    // ====== Hooks do sistema de posição de inimigos ======

    // Hook para monitorar posições de inimigos
    uintptr_t addr_GetEnemyBurnPos = getAbsoluteAddress(targetLibName, 0x2E678C);
    MSHookFunction((void *) addr_GetEnemyBurnPos, (void *) &hook_GetEnemyBurnPos,
                   (void **) &original_GetEnemyBurnPos);

    // Hook para monitorar posições de policiais
    uintptr_t addr_GetPoliceBurnPos = getAbsoluteAddress(targetLibName, 0x2E6B40);
    MSHookFunction((void *) addr_GetPoliceBurnPos, (void *) &hook_GetPoliceBurnPos,
                   (void **) &original_GetPoliceBurnPos);

    // Hook para monitorar limpeza de inimigos
    uintptr_t addr_ClearHasEnemy = getAbsoluteAddress(targetLibName, 0x2E6EC0);
    MSHookFunction((void *) addr_ClearHasEnemy, (void *) &hook_ClearHasEnemy,
                   (void **) &original_ClearHasEnemy);

    // Hook para monitorar verificação de tipos de posição
    uintptr_t addr_HasScenePosType = getAbsoluteAddress(targetLibName, 0x2E6568);
    MSHookFunction((void *) addr_HasScenePosType, (void *) &hook_HasScenePosType,
                   (void **) &original_HasScenePosType);

    // ====== Hooks do sistema de itens ======

    // Hook para modificar a quantidade de itens
    uintptr_t addr_GetDropGoodNumber = getAbsoluteAddress(targetLibName, 0x53B1A0);
    MSHookFunction((void *) addr_GetDropGoodNumber, (void *) &hook_GetDropGoodNumber,
                   (void **) &original_GetDropGoodNumber);

    // Hook para monitorar mudanças na quantidade de itens
    uintptr_t addr_SetDropGoodNumber = getAbsoluteAddress(targetLibName, 0x53B71C);
    MSHookFunction((void *) addr_SetDropGoodNumber, (void *) &hook_SetDropGoodNumber,
                   (void **) &original_SetDropGoodNumber);

    LOGI(OBFUSCATE("Hooks aplicados com sucesso"));
#endif

    return NULL;
}

/**
 * Define a lista de recursos disponíveis no menu de hacks
 */
jobjectArray GetFeatureList(JNIEnv *env, jobject context) {
    jobjectArray ret;

    const char *features[] = {
            // Recursos originais
            OBFUSCATE("Category_Menu de Modificações"),
            OBFUSCATE("4_Toggle_Vida Infinita"),
            OBFUSCATE("1_SeekBar_Dano de bala_1_999"),
            OBFUSCATE("2_InputValue_Adicionar Moedas"),
            OBFUSCATE("3_InputValue_Adicionar Gems"),
            OBFUSCATE("5_SeekBar_Balas das Armas_1_999999"),

            // Debug de inimigos
            OBFUSCATE("Category_Debug de Inimigos"),
            OBFUSCATE("6_Toggle_Debug Posições de Inimigos"),
            OBFUSCATE("7_Button_Forçar Remoção de Inimigos"),

            // Gerenciamento de itens
            OBFUSCATE("Category_Gerenciamento de Itens"),
            OBFUSCATE("8_Toggle_Ouro/Diamantes Infinitos"),
            OBFUSCATE("9_Toggle_Munição Infinita"),
            OBFUSCATE("10_Toggle_Vida Infinita (Via Itens)"),
            OBFUSCATE("11_Toggle_Recursos Infinitos"),
            OBFUSCATE("12_Button_Adicionar Todas as Partes de Armas"),
            OBFUSCATE("13_Button_Adicionar Todas as Peles"),
            OBFUSCATE("14_Button_Adicionar 10 Whisky"),
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
}

// Função para encontrar e obter a instância do EnemyPosCtrl
void* GetEnemyPosCtrlInstance() {
    // Esta função simula a chamada para GetInstance() de EnemyPosCtrl
    typedef void* (*GetInstanceFunc)();
    uintptr_t addr_GetInstance = getAbsoluteAddress(targetLibName, 0x2E66C4); // Endereço do GetInstance
    auto getInstance = reinterpret_cast<GetInstanceFunc>(addr_GetInstance);
    return getInstance();
}

/**
 * Manipula as mudanças feitas pelo usuário no menu
 * Aplica as configurações selecionadas
 */
void Changes(JNIEnv *env, jclass clazz, jobject obj,
             jint featNum, jstring featName, jint value,
             jboolean boolean, jstring str) {

    LOGD(OBFUSCATE("Recurso: %d - %s | Valor: = %d | Bool: = %d | Texto: = %s"), featNum,
         env->GetStringUTFChars(featName, 0), value,
         boolean, str != NULL ? env->GetStringUTFChars(str, 0) : "");

    void* enemyPosCtrl = nullptr;
    switch (featNum) {
        case 1: // Dano de bala
            if (value >= 1) {
                sliderValue = value;
            }
            break;
        case 2: // Adicionar moedas
            if (value >= 1) {
                Moedas = value;
                CallSaveGold(value);
            }
            break;
        case 3: // Adicionar gemas
            if (value >= 1) {
                Gems = value;
                CallSaveGem(value);
            }
            break;
        case 4: // Vida infinita
            Health = boolean;
            break;
        case 5: // Modificar balas de todas as armas
            CallSaveShotgunBullet(value);
            CallSaveRifleBullet(value);
            CallSavePistolBullet(value);
            break;
        case 6: // Debug de posições de inimigos
            debugEnemyPos = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_DEBUG", "Debug de posições de inimigos ativado");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_DEBUG", "Debug de posições de inimigos desativado");
            }
            break;
        case 7: // Forçar remoção de inimigos
            // Tenta obter a instância do controlador e chamar ClearHasEnemy
            enemyPosCtrl = GetEnemyPosCtrlInstance();
            if (enemyPosCtrl) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_DEBUG", "Executando remoção forçada de inimigos");
                hook_ClearHasEnemy(enemyPosCtrl);
            } else {
                __android_log_print(ANDROID_LOG_ERROR, "MOD_DEBUG", "Não foi possível obter a instância do EnemyPosCtrl");
            }
            break;
        case 8: // Ouro/Diamantes infinitos
            infiniteGold = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Ouro/Diamantes infinitos ativado");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Ouro/Diamantes infinitos desativado");
            }
            break;
        case 9: // Munição infinita
            infiniteAmmo = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Munição infinita ativada");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Munição infinita desativada");
            }
            break;
        case 10: // Vida infinita via itens
            infiniteHealth = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Vida infinita via itens ativada");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Vida infinita via itens desativada");
            }
            break;
        case 11: // Recursos infinitos
            infiniteResources = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Recursos infinitos ativado");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Recursos infinitos desativado");
            }
            break;
        case 12: // Adicionar todas as partes de armas
            // Adiciona 5 de cada parte de arma
            for (int i = GunPart1; i <= GunPart4; i++) {
                AddItemToInventory((DropGoodsType)i, 5);
            }
            __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Adicionadas 5 unidades de cada parte de arma");
            break;
        case 13: // Adicionar todas as peles
            // Adiciona 5 de cada tipo de pele
            for (int i = DeerSkin; i <= FoxSkin; i++) {
                AddItemToInventory((DropGoodsType)i, 5);
            }
            __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Adicionadas 5 unidades de cada tipo de pele");
            break;
        case 14: // Adicionar Whisky
            AddItemToInventory(WHISKY, 10);
            __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Adicionadas 10 unidades de Whisky");
            break;
    }
}

/**
 * Ponto de entrada da biblioteca
 * Cria uma nova thread para executar os hacks
 */
__attribute__((constructor))
void lib_main() {
    pthread_t ptid;
    pthread_create(&ptid, nullptr, hack_thread, nullptr);
}

/**
 * Registra o menu no ambiente JNI
 */
int RegisterMenu(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("Icon"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void *>(Icon)},
            {OBFUSCATE("IconWebViewData"), OBFUSCATE("()Ljava/lang/String;"), reinterpret_cast<void *>(IconWebViewData)},
            {OBFUSCATE("IsGameLibLoaded"), OBFUSCATE("()Z"), reinterpret_cast<void *>(isGameLibLoaded)},
            {OBFUSCATE("Init"), OBFUSCATE("(Landroid/content/Context;Landroid/widget/TextView;Landroid/widget/TextView;)V"), reinterpret_cast<void *>(Init)},
            {OBFUSCATE("SettingsList"), OBFUSCATE("()[Ljava/lang/String;"), reinterpret_cast<void *>(SettingsList)},
            {OBFUSCATE("GetFeatureList"), OBFUSCATE("()[Ljava/lang/String;"), reinterpret_cast<void *>(GetFeatureList)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/Menu"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;
    return JNI_OK;
}

/**
 * Registra as preferências no ambiente JNI
 */
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

/**
 * Registra a classe principal no ambiente JNI
 */
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

/**
 * Ponto de entrada JNI
 * Registra todas as classes nativas
 */
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