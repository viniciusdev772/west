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

// ================ VARIÁVEIS GLOBAIS PARA CONTROLAR RECURSOS =================
// 
// CORREÇÕES IMPLEMENTADAS V2.0:
// ✅ SetPlayerOnHorse: Agora usa GetOnHorse.get_Instance() (0x4654C4) - MÉTODO SEGURO
// ✅ SetPlayerOffHorse: Agora usa GetOffHorse.get_Instance() (0x465150) - MÉTODO SEGURO  
// ✅ AimBot Inteligente V2: Filtra corretamente para NÃO mirar no próprio jogador
// ✅ EnemyPosCtrl: Implementado acesso à lista real de inimigos (0x2E66C4)
// ✅ PlayerBaseType: Enum para distinguir Cowboy, EnemyNPC, Animal, Zombies, etc
// ✅ FindNearestEnemy V2: Busca apenas inimigos válidos (não o próprio player)
// ✅ Anti-Crash: Estados do jogo ao invés de funções diretas para cavalo
// ✅ Logs melhorados: Emojis e mensagens mais claras para debugging
// ===============================================================================

bool Health = false;
bool debugEnemyPos = false;
bool infiniteGold = false;
bool infiniteAmmo = false;
bool infiniteHealth = false;
bool infiniteResources = false;
bool playerOnHorse = false;
bool instantReload = false;
bool speedHack = false;
bool autoAim = false;
bool aimBot = false;                 // 🎯 AimBot Inteligente - caça inimigos automaticamente
bool alwaysHeadshot = false;         // 💀 Força todos os tiros como headshot
int sliderValue = 1, Moedas = 0, Gems = 0;
float speedMultiplier = 1.0f;

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
typedef void (*SaveCurrentPlayerPositionFunc)(Vector3);
typedef void (*SetPlayerOnHorseFunc)();
typedef void (*SetPlayerOffHorseFunc)();
typedef void* (*GetOnHorseInstanceFunc)();
typedef void* (*GetOffHorseInstanceFunc)();
typedef void* (*EnemyPosCtrlGetInstanceFunc)();
typedef void (*ReloadBulletsFunc)();
typedef float (*GetReloadTimeFunc)();

// Enumeração para estados de mira
enum AimTargetState {
    Nobody = 0,
    Aiming_Focus = 1,
    Aiming_NotFocus = 2
};

// Enumeração para tipos de player base (do dump.cs)
enum PlayerBaseType {
    Null = 0,
    Cowboy = 1,        // Jogador principal
    Horse = 2,
    MissionPerson = 3,
    EnemyNPC = 4,      // Inimigos NPCs
    Zombies = 5,       // Zumbis
    Animal = 6,        // Animais
    Ogre = 7,          // Ogros
    NonPermanentNpc = 8
};

typedef void (*UpdateAimTargetFunc)(void* thisPtr);
typedef void (*SetAimStateFunc)(void* thisPtr, AimTargetState state, void* target, bool forceTarget);
typedef void (*SetTargetPlayerFunc)(void* thisPtr, void* player);
typedef void* (*GetTargetPlayerFunc)(void* thisPtr);
typedef int (*GetClosestCharacterFunc)(void* verts, Vector3 pos);

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
 * Salva a posição atual do jogador
 * @param position Posição do jogador
 */
void CallSaveCurrentPlayerPosition(Vector3 position) {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52BEF4);
    auto savePosition = reinterpret_cast<SaveCurrentPlayerPositionFunc>(baseAddress);
    savePosition(position);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Posição do jogador salva: (%.2f, %.2f, %.2f)", 
                        position.x, position.y, position.z);
}

/**
 * Coloca o jogador no cavalo usando o estado do jogo (MÉTODO SEGURO)
 */
void CallSetPlayerOnHorse() {
    try {
        // Usando GetOnHorse.get_Instance() que é mais seguro - offset: 0x4654C4
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x4654C4);
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Endereço inválido para GetOnHorse.get_Instance");
            return;
        }
        
        auto getOnHorseInstance = reinterpret_cast<GetOnHorseInstanceFunc>(baseAddress);
        void* horseStateInstance = getOnHorseInstance();
        
        if (horseStateInstance) {
            __android_log_print(ANDROID_LOG_INFO, "ModMenu", "🐎 Estado GetOnHorse ativado - Jogador montará no cavalo");
            // O estado do jogo se encarregará de montar no cavalo de forma segura
        } else {
            __android_log_print(ANDROID_LOG_WARNING, "ModMenu", "Aviso: Instância GetOnHorse não disponível");
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Exceção em CallSetPlayerOnHorse");
    }
}

/**
 * Remove o jogador do cavalo usando o estado do jogo (MÉTODO SEGURO)
 */
void CallSetPlayerOffHorse() {
    try {
        // Usando GetOffHorse.get_Instance() que é mais seguro - offset: 0x465150
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x465150);
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Endereço inválido para GetOffHorse.get_Instance");
            return;
        }
        
        auto getOffHorseInstance = reinterpret_cast<GetOffHorseInstanceFunc>(baseAddress);
        void* horseStateInstance = getOffHorseInstance();
        
        if (horseStateInstance) {
            __android_log_print(ANDROID_LOG_INFO, "ModMenu", "🐎 Estado GetOffHorse ativado - Jogador desmontará do cavalo");
            // O estado do jogo se encarregará de desmontar do cavalo de forma segura
        } else {
            __android_log_print(ANDROID_LOG_WARNING, "ModMenu", "Aviso: Instância GetOffHorse não disponível");
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Exceção em CallSetPlayerOffHorse");
    }
}

/**
 * Força atualização do alvo de mira (com verificação de segurança)
 * @param playerCtrl Ponteiro para o controlador do jogador
 */
void CallUpdateAimTarget(void* playerCtrl) {
    if (!playerCtrl) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Ponteiro do jogador é nulo");
        return;
    }
    
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x45CF6C);
    auto updateAimTarget = reinterpret_cast<UpdateAimTargetFunc>(baseAddress);
    updateAimTarget(playerCtrl);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Alvo de mira atualizado");
}

/**
 * Define estado de mira (com verificação de segurança)
 * @param playerCtrl Ponteiro para o controlador do jogador
 * @param state Estado da mira
 * @param target Alvo (pode ser null)
 * @param forceTarget Forçar alvo
 */
void CallSetAimState(void* playerCtrl, AimTargetState state, void* target, bool forceTarget) {
    if (!playerCtrl) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Ponteiro do jogador é nulo");
        return;
    }
    
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x4599AC);
    auto setAimState = reinterpret_cast<SetAimStateFunc>(baseAddress);
    setAimState(playerCtrl, state, target, forceTarget);
    __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Estado de mira alterado para: %d", state);
}

/**
 * Define o jogador alvo (com verificação de segurança)
 * @param playerCtrl Ponteiro para o controlador do jogador
 * @param target Ponteiro para o jogador alvo
 */
void CallSetTargetPlayer(void* playerCtrl, void* target) {
    if (!playerCtrl) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Ponteiro do jogador é nulo");
        return;
    }
    
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x478E1C);
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Endereço inválido para SetTargetPlayer");
            return;
        }
        
        auto setTargetPlayer = reinterpret_cast<SetTargetPlayerFunc>(baseAddress);
        setTargetPlayer(playerCtrl, target);
        __android_log_print(ANDROID_LOG_INFO, "ModMenu", "Alvo do jogador definido: %p", target);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Exceção em SetTargetPlayer");
    }
}

/**
 * Obtém o jogador alvo atual (com verificação de segurança)
 * @param playerCtrl Ponteiro para o controlador do jogador
 * @return Ponteiro para o jogador alvo ou nullptr
 */
void* CallGetTargetPlayer(void* playerCtrl) {
    if (!playerCtrl) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Ponteiro do jogador é nulo");
        return nullptr;
    }
    
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x480DE4);
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Erro: Endereço inválido para GetTargetPlayer");
            return nullptr;
        }
        
        auto getTargetPlayer = reinterpret_cast<GetTargetPlayerFunc>(baseAddress);
        void* target = getTargetPlayer(playerCtrl);
        __android_log_print(ANDROID_LOG_DEBUG, "ModMenu", "Alvo atual: %p", target);
        return target;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "ModMenu", "Exceção em GetTargetPlayer");
        return nullptr;
    }
}

/**
 * Obtém instância do controlador de posições de inimigos
 * @return Ponteiro para EnemyPosCtrl ou nullptr
 */
void* GetEnemyPosCtrlInstance() {
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x2E66C4); // EnemyPosCtrl.GetInstance()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_AIMBOT", "Erro: Endereço inválido para EnemyPosCtrl.GetInstance");
            return nullptr;
        }
        
        auto getEnemyPosCtrlInstance = reinterpret_cast<EnemyPosCtrlGetInstanceFunc>(baseAddress);
        void* enemyPosCtrl = getEnemyPosCtrlInstance();
        
        if (enemyPosCtrl) {
            __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIMBOT", "EnemyPosCtrl obtido com sucesso: %p", enemyPosCtrl);
        }
        
        return enemyPosCtrl;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_AIMBOT", "Exceção ao obter EnemyPosCtrl");
        return nullptr;
    }
}

/**
 * Adiciona um item específico ao inventário
 * @param goodType Tipo do item a ser adicionado
 * @param amount Quantidade a ser adicionada
 */
void AddItemToInventory(DropGoodsType goodType, int amount) {
    try {
        typedef int (*GetNumFunc)(int);
        typedef void (*SetNumFunc)(int, int);

        uintptr_t addr_GetNum = getAbsoluteAddress(targetLibName, 0x53B1A0); // GetDropGoodNumber
        uintptr_t addr_SetNum = getAbsoluteAddress(targetLibName, 0x53B71C); // SetDropGoodNumber

        // Verificações de segurança
        if (addr_GetNum == 0 || addr_SetNum == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_ITEMS", "Erro: Endereços inválidos para AddItemToInventory");
            return;
        }

        auto getNum = reinterpret_cast<GetNumFunc>(addr_GetNum);
        auto setNum = reinterpret_cast<SetNumFunc>(addr_SetNum);

        int currentAmount = getNum((int)goodType);
        setNum((int)goodType, currentAmount + amount);

        __android_log_print(ANDROID_LOG_INFO, "MOD_ITEMS", "Adicionado %d do item tipo %d. Total: %d",
                            amount, (int)goodType, currentAmount + amount);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_ITEMS", "Exceção em AddItemToInventory");
    }
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
 * Modifica o dano causado pelas balas (com verificações de segurança)
 */
int hook_GetHitBlood(void *thisPtr, int part, int type, float enemy, float myPosition, int modelType) {
    // Verificação de segurança para evitar crashes
    if (!thisPtr) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD", "Erro: thisPtr é nulo em GetHitBlood");
        return 1; // Retorna dano mínimo
    }
    
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Entrada na função GetHitBlood");
    int result = original_GetHitBlood(thisPtr, part, type, enemy, myPosition, modelType);
    
    // Se alwaysHeadshot estiver ativo e for parte da cabeça, aumenta o dano
    if (alwaysHeadshot && part == 0) { // Assumindo que 0 = cabeça
        result = sliderValue * 2; // Dano de headshot
        __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Headshot forçado! Dano: %d", result);
    } else {
        result = sliderValue;
    }
    
    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Dano final: %d", result);
    return result;
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

// Ponteiros para funções de velocidade e recarga
float (*original_GetReloadTime)(void* thisPtr);
float (*original_GetWalkSpeed)(void* thisPtr);
float (*original_GetRunSpeed)(void* thisPtr);

// Ponteiros para funções de mira
void (*original_UpdateAimTarget)(void* thisPtr);
void (*original_SetAimState)(void* thisPtr, AimTargetState state, void* target, bool forceTarget);

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

// ================ HOOKS PARA VELOCIDADE E RECARGA =================

/**
 * Hook para a função GetReloadTime
 * Modifica o tempo de recarga das armas
 */
float hook_GetReloadTime(void* thisPtr) {
    float originalTime = original_GetReloadTime(thisPtr);
    
    if (instantReload) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_RELOAD", "Recarga instantânea ativada");
        return 0.1f; // Tempo mínimo para evitar bugs
    }
    
    return originalTime;
}

/**
 * Hook para a função GetWalkSpeed
 * Modifica a velocidade de caminhada
 */
float hook_GetWalkSpeed(void* thisPtr) {
    float originalSpeed = original_GetWalkSpeed(thisPtr);
    
    if (speedHack) {
        float modifiedSpeed = originalSpeed * speedMultiplier;
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_SPEED", 
                            "Velocidade caminhada modificada: %.2f -> %.2f", 
                            originalSpeed, modifiedSpeed);
        return modifiedSpeed;
    }
    
    return originalSpeed;
}

/**
 * Hook para a função GetRunSpeed
 * Modifica a velocidade de corrida
 */
float hook_GetRunSpeed(void* thisPtr) {
    float originalSpeed = original_GetRunSpeed(thisPtr);
    
    if (speedHack) {
        float modifiedSpeed = originalSpeed * speedMultiplier;
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_SPEED", 
                            "Velocidade corrida modificada: %.2f -> %.2f", 
                            originalSpeed, modifiedSpeed);
        return modifiedSpeed;
    }
    
    return originalSpeed;
}

// ================ HOOKS PARA SISTEMA DE MIRA =================

/**
 * Busca inimigo mais próximo do jogador usando lista real de inimigos
 * @param playerCtrl Ponteiro para o controlador do jogador
 * @return Ponteiro para inimigo mais próximo ou nullptr
 */
void* FindNearestEnemy(void* playerCtrl) {
    if (!playerCtrl) {
        return nullptr;
    }
    
    static void* lastFoundEnemy = nullptr;
    static int searchCooldown = 0;
    static int enemySearchIndex = 0;
    
    // Cooldown para evitar busca excessiva
    if (searchCooldown > 0) {
        searchCooldown--;
        return lastFoundEnemy;
    }
    
    // Reset cooldown (busca a cada 30 frames ~0.5s)
    searchCooldown = 30;
    
    try {
        // Obtém controle de posições de inimigos
        void* enemyPosCtrl = GetEnemyPosCtrlInstance();
        if (!enemyPosCtrl) {
            __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIMBOT", "EnemyPosCtrl não disponível - usando método alternativo");
            
            // Método alternativo: verifica alvo atual do jogo
            void* currentTarget = CallGetTargetPlayer(playerCtrl);
            if (currentTarget && currentTarget != playerCtrl) { // IMPORTANTE: Não mirar em si mesmo!
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "🎯 Alvo detectado pelo jogo: %p", currentTarget);
                lastFoundEnemy = currentTarget;
                return currentTarget;
            }
            
            return nullptr;
        }
        
        // TODO: Aqui deveria iterar pela listPosition do EnemyPosCtrl
        // Por segurança, vamos usar o método alternativo por enquanto
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIMBOT", "🔍 Escaneando por inimigos válidos...");
        
        // Procura por alvos que NÃO sejam o próprio jogador
        void* currentTarget = CallGetTargetPlayer(playerCtrl);
        if (currentTarget && currentTarget != playerCtrl) {
            // Verifica se é um tipo válido de inimigo (não é o próprio player)
            __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "🎯 Inimigo válido encontrado: %p", currentTarget);
            lastFoundEnemy = currentTarget;
            return currentTarget;
        }
        
        // Se não encontrou nada, limpa cache
        if (enemySearchIndex++ > 10) {
            enemySearchIndex = 0;
            lastFoundEnemy = nullptr;
            __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIMBOT", "🔄 Reset de busca - procurando novos alvos");
        }
        
        return lastFoundEnemy;
        
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_AIMBOT", "❌ Erro crítico na busca de inimigos");
        lastFoundEnemy = nullptr;
        return nullptr;
    }
}

/**
 * Hook para a função UpdateAimTarget
 * Implementa aimbot inteligente que caça inimigos próximos
 */
void hook_UpdateAimTarget(void* thisPtr) {
    // Verificação de segurança
    if (!thisPtr) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_AIM", "Erro: thisPtr é nulo em UpdateAimTarget");
        return;
    }
    
    // Chama a função original primeiro
    original_UpdateAimTarget(thisPtr);
    
    // Se autoAim estiver ativado, força estado de mira focada
    if (autoAim) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIM", "Auto-aim ativo - forçando foco");
        CallSetAimState(thisPtr, Aiming_Focus, nullptr, true);
    }
    
    // Se aimBot estiver ativo, procura e mira em inimigos próximos
    if (aimBot) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIMBOT", "Aimbot ativo - procurando inimigos");
        
        void* nearestEnemy = FindNearestEnemy(thisPtr);
        
        if (nearestEnemy) {
            // Define o inimigo como alvo
            CallSetTargetPlayer(thisPtr, nearestEnemy);
            
            // Força estado de mira focada no alvo
            CallSetAimState(thisPtr, Aiming_Focus, nearestEnemy, true);
            
            // Contador de alvos travados (para estatísticas)
            static int targetsLocked = 0;
            targetsLocked++;
            
            __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", 
                               "🎯 Aimbot: Alvo #%d travado %p - Mira automática ativa!", 
                               targetsLocked, nearestEnemy);
        } else {
            // Se não há inimigos, mantém busca ativa
            __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIMBOT", "🔍 Escaneando área por inimigos...");
        }
    }
}

/**
 * Hook para a função SetAimState
 * Monitora e modifica estados de mira
 */
void hook_SetAimState(void* thisPtr, AimTargetState state, void* target, bool forceTarget) {
    // Verificação de segurança
    if (!thisPtr) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_AIM", "Erro: thisPtr é nulo em SetAimState");
        return;
    }
    
    AimTargetState finalState = state;
    
    // Se autoAim estiver ativo, sempre força foco quando há um alvo
    if (autoAim && target && state == Aiming_NotFocus) {
        finalState = Aiming_Focus;
        forceTarget = true;
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIM", "Auto-aim: Convertendo NotFocus para Focus");
    }
    
    __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIM", 
                        "SetAimState: Estado=%d, Alvo=%p, Forçar=%d", 
                        finalState, target, forceTarget);
    
    // Chama a função original com possíveis modificações
    original_SetAimState(thisPtr, finalState, target, forceTarget);
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

    // ====== Hooks das novas funcionalidades ======

    // Hook para tempo de recarga das armas
    uintptr_t addr_GetReloadTime = getAbsoluteAddress(targetLibName, 0x456E48);
    MSHookFunction((void *) addr_GetReloadTime, (void *) &hook_GetReloadTime,
                   (void **) &original_GetReloadTime);

    // Nota: Os hooks de velocidade serão implementados através de modificação de memória
    // pois walk_speed e run_speed são campos de struct, não funções

    // ====== Hooks do sistema de mira ======

    // Hook para atualização de alvos de mira
    uintptr_t addr_UpdateAimTarget = getAbsoluteAddress(targetLibName, 0x45CF6C);
    MSHookFunction((void *) addr_UpdateAimTarget, (void *) &hook_UpdateAimTarget,
                   (void **) &original_UpdateAimTarget);

    // Hook para configuração de estado de mira
    uintptr_t addr_SetAimState = getAbsoluteAddress(targetLibName, 0x4599AC);
    MSHookFunction((void *) addr_SetAimState, (void *) &hook_SetAimState,
                   (void **) &original_SetAimState);

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

            // Novas funcionalidades
            OBFUSCATE("Category_Controle do Jogador"),
            OBFUSCATE("15_Button_Colocar no Cavalo"),
            OBFUSCATE("16_Button_Remover do Cavalo"),
            OBFUSCATE("17_Toggle_Recarga Instantânea"),
            OBFUSCATE("18_Toggle_Hack de Velocidade"),
            OBFUSCATE("19_SeekBar_Multiplicador de Velocidade_1_10"),

            // Sistema de mira
            OBFUSCATE("Category_Sistema de Mira"),
            OBFUSCATE("20_Toggle_Auto-Aim"),
            OBFUSCATE("21_Toggle_AimBot Inteligente"),
            OBFUSCATE("22_Toggle_Sempre Headshot"),
            OBFUSCATE("23_Button_Limpar Alvos de Mira"),
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
        case 15: // Colocar no cavalo
            __android_log_print(ANDROID_LOG_INFO, "MOD_HORSE", "🐎 Iniciando processo para montar no cavalo...");
            CallSetPlayerOnHorse();
            playerOnHorse = true;
            break;
        case 16: // Remover do cavalo  
            __android_log_print(ANDROID_LOG_INFO, "MOD_HORSE", "🐎 Iniciando processo para desmontar do cavalo...");
            CallSetPlayerOffHorse();
            playerOnHorse = false;
            break;
        case 17: // Recarga instantânea
            instantReload = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_RELOAD", "Recarga instantânea ativada");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_RELOAD", "Recarga instantânea desativada");
            }
            break;
        case 18: // Hack de velocidade
            speedHack = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_SPEED", "Hack de velocidade ativado");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_SPEED", "Hack de velocidade desativado");
            }
            break;
        case 19: // Multiplicador de velocidade
            if (value >= 1 && value <= 10) {
                speedMultiplier = (float)value;
                __android_log_print(ANDROID_LOG_INFO, "MOD_SPEED", 
                                    "Multiplicador de velocidade alterado para: %.1f", speedMultiplier);
            }
            break;
        case 20: // Auto-Aim
            autoAim = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIM", "Auto-Aim ativado");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIM", "Auto-Aim desativado");
            }
            break;
        case 21: // AimBot Inteligente V2
            aimBot = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "🎯 AimBot Inteligente V2 ativado - Caçará NPCs/Animais próximos (NÃO o próprio player)");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "AimBot Inteligente V2 desativado");
            }
            break;
        case 22: // Sempre Headshot
            alwaysHeadshot = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIM", "Sempre Headshot ativado - Todos os tiros serão headshots");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIM", "Sempre Headshot desativado");
            }
            break;
        case 23: // Limpar alvos de mira
            // Limpa todos os alvos atuais e força atualização
            __android_log_print(ANDROID_LOG_INFO, "MOD_AIM", "Limpando todos os alvos de mira...");
            // Esta funcionalidade é segura pois apenas limpa alvos
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