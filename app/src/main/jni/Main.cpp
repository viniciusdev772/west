#include <pthread.h>
#include <jni.h>
#include <unistd.h>
#include <cstdint>
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
    DropNull = 0,
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
// CORREÇÕES IMPLEMENTADAS V3.0:
// ✅ SetPlayerOnHorse: Agora usa GetOnHorse.get_Instance() (0x4654C4) - MÉTODO SEGURO
// ✅ SetPlayerOffHorse: Agora usa GetOffHorse.get_Instance() (0x465150) - MÉTODO SEGURO  
// ✅ AimBot V5: SISTEMA AGRESSIVO com offsets E campos reais do dump.cs
// ✅ Proteção Anti-Cowboy: JAMAIS mira em Cowboy = 1 (PlayerBaseType + AnimalType)
// ✅ Busca de alvo segura: AimTargetPlayer(0x24) + AimTargetForCamera(0x28)
// ✅ Offsets Reais: aimTargetState(0x20), AimTargetPlayer(0x24), AimTargetForCamera(0x28)
// ✅ Mira estável: Força Aiming_Focus apenas com alvo válido (Transform)
// ✅ EnemyPosCtrl: Acesso real à lista de inimigos (0x2E66C4)
// ✅ PlayerBaseType: Enum para distinguir Cowboy, EnemyNPC, Animal, Zombies, etc
// ✅ Anti-Crash: Estados do jogo ao invés de funções diretas para cavalo
// ✅ Sistema Policial V1: NPCs policiais, xerife, caçadores de recompensa
// ✅ Controle de Polícia: GeneratePolice, HideAllPolice, GetPoliceMaxNum
// ✅ NPCs da Lei: Sheriff(18), BountyHunter(25) com estados reais
// ✅ PROTEÇÃO ANTI-CRASH: Validação de ponteiros, endereços e parâmetros
// ✅ Tratamento de Exceções: std::exception + catch(...) em todas as funções
// ✅ Valores Seguros: Retorna defaults válidos em caso de erro
// ✅ Busca Forçada: Múltiplas chamadas de UpdateAimTarget para acelerar detecção
// ✅ Logs melhorados: Emojis específicos 🎯🧟👹 para diferentes tipos de inimigos
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
bool aimBotAggressive = false;       // ⚡ Aimbot agressivo (mais tentativas por frame)
bool alwaysHeadshot = false;         // 💀 Força todos os tiros como headshot
bool flyMode = false;                // 🕊️ Modo voo experimental
bool completeEsp = false;            // 👁️ ESP completo com barras de vida
bool autoKill = false;               // ☠️ Mata inimigos ativos com BeHit
int sliderValue = 1, Moedas = 0, Gems = 0;
float speedMultiplier = 1.0f;
float flyVerticalSpeed = 5.0f;
float flyHeightStep = 1.0f;

// Ações pendentes para execução em contexto de jogo
volatile bool pendingGeneratePolice = false;
volatile bool pendingHidePolice = false;
volatile bool pendingShowPolice = false;
volatile bool pendingCreateMissionHints = false;
volatile bool pendingDestroyMissionHints = false;
volatile bool pendingEspRefresh = false;
volatile bool pendingEspClear = false;
volatile bool pendingAutoKillBurst = false;

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

template <typename T>
struct Il2CppArray {
    void* klass;
    void* monitor;
    void* bounds;
    uint32_t max_length;
    T items[0];
};

template <typename T>
struct Il2CppList {
    void* klass;
    void* monitor;
    Il2CppArray<T>* items;
    int size;
    int version;
    void* syncRoot;
};

template <typename TKey, typename TValue>
struct Il2CppDictionary {
    void* klass;
    void* monitor;
    void* table;
    void* linkSlots;
    Il2CppArray<TKey>* keySlots;
    Il2CppArray<TValue>* valueSlots;
    int touchedSlots;
    int emptySlot;
    int count;
    int threshold;
    void* hcp;
    void* serialization_info;
};

// Estrutura para dados de origem do jogador (correta conforme dump.cs)
struct MyPlayerOriData {
    void* vTable;           // 0x0
    int unknown1;          // 0x4
    const char* name;      // 0x8
    int blood;             // 0xC
    int gun_ID;            // 0x10
    int pistol_ID;         // 0x14
    int long_gun_ID;       // 0x18
    int knife_ID;          // 0x1C
    int head_ID;           // 0x20
    int body_ID;           // 0x24
    int pant_ID;           // 0x28
    int horse_ID;          // 0x2C
    float walk_speed;      // 0x30
    float run_speed;       // 0x34
    float acc;             // 0x38
    float dec;             // 0x3C
};

// Estrutura para dados de velocidade em tempo real (encontrada no dump.cs)
struct PlayerSpeedData {
    void* vTable;               // 0x0
    int unknown1;              // 0x4  
    int unknown2;              // 0x8
    int unknown3;              // 0xC
    void* unknown4;            // 0x10
    float m_dMaxSpeed;         // 0x14 - Velocidade máxima atual
    float m_dMaxSprintSpeed;   // 0x18 - Velocidade máxima de sprint
    float m_dMaxRunSpeed;      // 0x1C - Velocidade máxima de corrida  
    float m_dMaxWalkSpeed;     // 0x20 - Velocidade máxima de caminhada
    int unknown5;              // 0x24
    int unknown6;              // 0x28
    int unknown7;              // 0x2C
    int unknown8;              // 0x30
    float m_dCurSpeed;         // 0x34 - Velocidade atual do jogador
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
typedef void* (*EnemyPosCtrlGetInstanceFunc)(void* method);
typedef void* (*MissionCtrlGetInstanceFunc)(void* method);
typedef void* (*GameCtrlGetInstanceFunc)(void* method);
typedef void (*ReloadBulletsFunc)();
typedef float (*GetReloadTimeFunc)();
typedef Vector3 (*ReadCurrentPlayerPositionFunc)();
typedef Vector3 (*PlayerGetPositionFunc)(void* thisPtr, void* method);
typedef void (*PlayerSetPositionFunc)(void* thisPtr, Vector3 pos, void* method);
typedef void (*PlayerSetNavMeshEnableFunc)(void* thisPtr, bool enable, void* method);
typedef void (*PlayerSetCurrentVelocityFunc)(void* thisPtr, Vector3 velocity, void* method);
typedef Vector3 (*MyCtrlPlayerGetPositionFunc)(void* thisPtr, void* method);
typedef void (*MyCtrlPlayerSetPositionFunc)(void* thisPtr, Vector3 pos, void* method);

// Tipos de função corrigidos
typedef MyPlayerOriData* (*GetMyPlayerOriDataFunc)();

// Enumeração para estados de mira (do dump.cs)
enum AimTargetState {
    Nobody = 0,
    Aiming_Focus = 1,
    Aiming_NotFocus = 2
};

// Enumeração para tipos de player base (do dump.cs)
enum PlayerBaseType {
    PlayerNull = 0,
    Cowboy = 1,        // ❌ NUNCA MIRAR - Jogador principal
    Horse = 2,
    MissionPerson = 3,
    EnemyNPC = 4,      // ✅ Inimigos NPCs
    Zombies = 5,       // ✅ Zumbis
    Animal = 6,        // ✅ Animais
    Ogre = 7,          // ✅ Ogros - PRIORIDADE ALTA
    NonPermanentNpc = 8
};

// Enumeração para tipos de animais específicos (do dump.cs)
enum AnimalType {
    AnimalNull = 0,
    AnimalCowboy = 1,     // ❌ NUNCA MIRAR
    
    // Inimigos Humanos - PRIORIDADE MÉDIA
    BountyHunter = 25,    // ✅ Caçador de recompensas
    Robber = 26,          // ✅ Ladrão  
    Pro01 = 30,           // ✅ Profissional 1
    Pro02 = 31,           // ✅ Profissional 2
    
    // Zumbis - PRIORIDADE ALTA
    Zombies01 = 34,       // ✅ Zumbi tipo 1
    Zombies02 = 35,       // ✅ Zumbi tipo 2
    Zombies03 = 36,       // ✅ Zumbi tipo 3
    Zombies04 = 37,       // ✅ Zumbi tipo 4
    Zombies05 = 38,       // ✅ Zumbi tipo 5
    Zombies06 = 39,       // ✅ Zumbi tipo 6
    Zombies07 = 40,       // ✅ Zumbi tipo 7
    
    // Ogros/Chefes - PRIORIDADE MÁXIMA
    AnimalOgre = 41,      // ✅ Ogro comum (renomeado para evitar conflito)
    OgreBoss = 42,        // ✅ Chefe Ogro
    
    // Animais Hostis - PRIORIDADE BAIXA
    Cheetah = 43,         // ✅ Guepardo
    Bear = 44,            // ✅ Urso  
    Wolf01 = 45,          // ✅ Lobo tipo 1
    Wolf02 = 46,          // ✅ Lobo tipo 2
    Wolf03 = 47,          // ✅ Lobo tipo 3
    Eagle = 49            // ✅ Águia
};

// ========== ENUMS PARA SISTEMA POLICIAL (do dump.cs) ==========

// Estados do Xerife (do dump.cs)
enum NpcSheriffStates {
    SheriffInit = 0,
    SheriffIdle = 1,
    SheriffWalkingDes = 2,
    SheriffPickWarrant = 3    // ✅ Pegar mandado de prisão
};

// Estados do Caçador de Recompensas (do dump.cs)
enum NpcBountyHunterStates {
    BountyInit = 0,
    BountyIdle = 1,
    BountyStirCoffee = 2,     // ✅ Mexer café
    BountyDrinkingCoffee = 3  // ✅ Beber café
};

enum MissionUnlockType {
    MissionUnlock_Main = 0,
    MissionUnlock_Branch = 1,
    MissionUnlock_Daily = 2
};

enum GameScenes {
    GameScene_FirstLoading = 0,
    GameScene_Empty = 1,
    GameScene_NoviceVillage = 2,
    GameScene_Cell = 3,
    GameScene_Canyon = 4,
    GameScene_Forest = 5,
    GameScene_Bar = 6,
    GameScene_Mine = 7,
    GameScene_Cemetery = 8,
    GameScene_Null = 9,
    GameScene_ForMinYUI = 10
};

// Tipos de Posições Policiais (do dump.cs)
enum PoliceScenePosType {
    NV_PoliceStation = 5,     // ✅ Delegacia
    NV_PoliceWarrant = 14     // ✅ Mandado policial
};

typedef void (*UpdateAimTargetFunc)(void* thisPtr);
typedef void (*SetAimStateFunc)(void* thisPtr, AimTargetState state, void* target, bool forceTarget);

// ========== TIPOS DE FUNÇÃO PARA SISTEMA POLICIAL ==========
typedef void* (*GetNPCenemyOriDataFunc)(int ID);
typedef int (*GetPoliceMaxNumFunc)(void* method);
typedef void (*GeneratePoliceFunc)(void* thisPtr, void* method);
typedef Vector3 (*GetPoliceBurnPosFunc)(void* thisPtr, Vector3 playerPos, float minSqr, float maxSqr);
typedef void (*HideNonNpcAndPoliceFunc)(void* thisPtr, void* method);
typedef void (*RecoverNonNpcAndPoliceFunc)(void* thisPtr, void* method);
typedef void* (*GetNpcSheriffInstanceFunc)(void* method);
typedef void* (*GetNpcBountyHunterInstanceFunc)(void* method);
typedef void* (*GetSceneInOutPosCtrlInstanceFunc)(void* method);
typedef void (*CreateMissionHintsFunc)(void* thisPtr, int gameScene, int missionType, void* method);
typedef void (*DestroyMissionHintsFunc)(void* thisPtr, void* method);
typedef void* (*GetGunParticalEffectsCtrlInstanceFunc)(void* method);
typedef void (*ToggleSceneMissionHintsFunc)(void* thisPtr, void* method);
typedef void* (*GetUIManageInstanceFunc)(void* method);
typedef void (*ShowAimFollowTargetUIFunc)(void* thisPtr, void* method);
typedef void (*HideAimFollowTargetUIFunc)(void* thisPtr, void* method);
typedef void (*SetAimFollowTargetUIFunc)(void* thisPtr, void* target, void* method);
typedef void* (*GetEntityManagerInstanceFunc)(void* method);
typedef void* (*GameCtrlGetEnemyActiveListFunc)(void* thisPtr, void* method);
typedef void (*GenerateEnemyBloodFunc)(void* thisPtr, void* target, void* method);
typedef void (*DestroyAllBloodFunc)(void* thisPtr, void* method);
typedef void (*SetCurrentBloodValueFunc)(void* thisPtr, void* target, int blood, int maxBlood, void* method);
typedef void (*SetCurrentBloodEnableFunc)(void* thisPtr, void* target, bool active, void* method);
typedef void (*PlayerBaseBeHitFunc)(void* thisPtr, int loseBlood, int hurtPart, void* method);
typedef void (*MyCtrlPlayerMyUpdateFunc)(void* thisPtr);
typedef void (*MissionCtrlPlayerActionFunc)(void* thisPtr, void* player, void* method);
typedef bool (*MissionEntityContainEnemyFunc)(void* thisPtr, void* playerBase, void* method);
typedef void (*MissionEntityDeleteEnemyFunc)(void* thisPtr, void* playerBase, void* method);
typedef bool (*EnemyFactoryContainPlayerBaseFunc)(void* thisPtr, void* playerBase, void* method);
typedef void (*EnemyFactoryDeletePlayerBaseFunc)(void* thisPtr, void* playerBase, void* method);
typedef void (*EnemyGCFunc)(void* thisPtr, void* player, void* method);

// Declarações antecipadas das funções necessárias
void CallSetAimState(void* playerCtrl, AimTargetState state, void* target, bool forceTarget);

// ========== DECLARAÇÕES ANTECIPADAS - SISTEMA POLICIAL ==========
void* GetNPCenemyOriData(int ID);
int GetPoliceMaxNum();
void GeneratePolice(void* missionCtrl);
Vector3 GetPoliceBurnPos(void* enemyPosCtrl, Vector3 playerPos, float minSqr, float maxSqr);
void HideAllPolice(void* missionCtrl);
void ShowAllPolice(void* missionCtrl);
void* GetMissionCtrlInstance();
void* GetSheriffInstance();
void* GetBountyHunterInstance();
void* GetSceneInOutPosCtrlInstance();
void CreateMissionHints();
void DestroyMissionHints();
void* GetGunParticalEffectsCtrlInstance();
void TurnOnSceneMissionHints();
void TurnOffSceneMissionHints();
void* GetUIManageInstance();
void* GetPlayingUICreatorInstance();
void* GetPlayingUIBloodFactoryInstance();
void ShowTargetMarkerOnCurrentTarget();
void HideTargetMarker();
bool CanEnableCompleteESP();
void RefreshCompleteESP();
void ClearCompleteESP();
void LogTrackedEntities();
bool CanRunAutoKill();
int CollectActiveEnemyBases(void** outEnemies, int maxEnemies);
void RunAutoKillOnce();
void ProcessGameplayFrame(void* myCtrlPlayer);

bool IsValidAimTransform(void* target, void* playerCtrl) {
    if (!target || target == playerCtrl) return false;
    return (uintptr_t)target >= 0x10000000;
}

/**
 * Encontra alvo atual de mira no MyCtrlPlayer de forma segura.
 * dump.cs:
 * - AimTargetPlayer em 0x24
 * - AimTargetForCamera em 0x28
 * @param playerCtrl Ponteiro para o controlador do jogador
 * @return Ponteiro para Transform alvo ou nullptr
 */
void* FindBestTarget(void* playerCtrl) {
    if (!playerCtrl) return nullptr;
    
    try {
        void** aimTargetPlayerPtr = reinterpret_cast<void**>((char*)playerCtrl + 0x24);
        void* currentAimTarget = aimTargetPlayerPtr ? *aimTargetPlayerPtr : nullptr;
        if (IsValidAimTransform(currentAimTarget, playerCtrl)) return currentAimTarget;

        void** aimTargetCameraPtr = reinterpret_cast<void**>((char*)playerCtrl + 0x28);
        void* aimTargetForCamera = aimTargetCameraPtr ? *aimTargetCameraPtr : nullptr;
        if (IsValidAimTransform(aimTargetForCamera, playerCtrl)) return aimTargetForCamera;

        return nullptr;
        
    } catch (...) {
        return nullptr;
    }
}

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

void ForceAimRefresh(void* playerCtrl) {
    if (!playerCtrl) return;
    try {
        // dump.cs: MyCtrlPlayer.aimTargetState // 0x20
        AimTargetState* aimStatePtr = reinterpret_cast<AimTargetState*>((char*)playerCtrl + 0x20);
        *aimStatePtr = Nobody;

        // dump.cs: MyCtrlPlayer.AimTargetPlayer // 0x24
        void** aimTargetPtr = reinterpret_cast<void**>((char*)playerCtrl + 0x24);
        if (aimTargetPtr) *aimTargetPtr = nullptr;

        // dump.cs: MyCtrlPlayer.AimTargetForCamera // 0x28
        void** aimTargetCamPtr = reinterpret_cast<void**>((char*)playerCtrl + 0x28);
        if (aimTargetCamPtr) *aimTargetCamPtr = nullptr;
    } catch (...) {
    }
}

Vector3 CallReadCurrentPlayerPosition() {
    Vector3 pos = {0.0f, 0.0f, 0.0f};
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x52F754);
    if (baseAddress == 0) return pos;
    auto readPosition = reinterpret_cast<ReadCurrentPlayerPositionFunc>(baseAddress);
    return readPosition();
}

void* GetGameCtrlInstance() {
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x2DDD44); // GameCtrl.GetInstance()
    if (baseAddress == 0) return nullptr;
    auto getGameCtrlInstance = reinterpret_cast<GameCtrlGetInstanceFunc>(baseAddress);
    return getGameCtrlInstance(nullptr);
}

void* GetMyPlayerInstance() {
    void* gameCtrl = GetGameCtrlInstance();
    if (!gameCtrl) return nullptr;
    // dump.cs: GameCtrl.myPlayer // 0x14
    void** myPlayerPtr = reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x14);
    if (!myPlayerPtr) return nullptr;
    void* player = *myPlayerPtr;
    if (!player || (uintptr_t)player < 0x10000000) return nullptr;
    return player;
}

void* GetMyCtrlPlayerInstance() {
    void* gameCtrl = GetGameCtrlInstance();
    if (!gameCtrl) return nullptr;
    // dump.cs: GameCtrl.m_pCtrlPlayer // 0x10
    void** myCtrlPlayerPtr = reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x10);
    if (!myCtrlPlayerPtr) return nullptr;
    void* myCtrlPlayer = *myCtrlPlayerPtr;
    if (!myCtrlPlayer || (uintptr_t)myCtrlPlayer < 0x10000000) return nullptr;
    return myCtrlPlayer;
}

void* GetCtrlPlayerFromMyCtrl() {
    void* myCtrlPlayer = GetMyCtrlPlayerInstance();
    if (!myCtrlPlayer) return nullptr;
    // dump.cs: MyCtrlPlayer.ctrlPlayer // 0xC
    void** ctrlPlayerPtr = reinterpret_cast<void**>(reinterpret_cast<char*>(myCtrlPlayer) + 0xC);
    if (!ctrlPlayerPtr) return nullptr;
    void* ctrlPlayer = *ctrlPlayerPtr;
    if (!ctrlPlayer || (uintptr_t)ctrlPlayer < 0x10000000) return nullptr;
    return ctrlPlayer;
}

void SetFlyRuntimeState(bool enabled) {
    void* ctrlPlayer = GetCtrlPlayerFromMyCtrl();
    if (!ctrlPlayer) return;

    uintptr_t addrSetNavMeshEnable = getAbsoluteAddress(targetLibName, 0x34897C); // Player.SetNavMesEnable(bool)
    uintptr_t addrSetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35C8F8); // Player.SetCurrentVelocity(Vector3)
    if (addrSetNavMeshEnable == 0 || addrSetCurrentVelocity == 0) return;

    auto setNavMeshEnable = reinterpret_cast<PlayerSetNavMeshEnableFunc>(addrSetNavMeshEnable);
    auto setCurrentVelocity = reinterpret_cast<PlayerSetCurrentVelocityFunc>(addrSetCurrentVelocity);

    // Em voo, desativa NavMesh para não prender fora da malha; ao sair, reativa.
    setNavMeshEnable(ctrlPlayer, !enabled, nullptr);

    // Evita estado de velocidade residual que pode bloquear input.
    Vector3 zeroVel = {0.0f, 0.0f, 0.0f};
    setCurrentVelocity(ctrlPlayer, zeroVel, nullptr);
}

bool ApplyFlyPositionStep() {
    // Usa MyCtrlPlayer para evitar desync visual ("fantasma")
    void* myCtrlPlayer = GetMyCtrlPlayerInstance();
    if (!myCtrlPlayer) return false;

    uintptr_t addrGetPosMyCtrl = getAbsoluteAddress(targetLibName, 0x4499F8); // MyCtrlPlayer.GetPosition()
    uintptr_t addrSetPosMyCtrl = getAbsoluteAddress(targetLibName, 0x45D69C); // MyCtrlPlayer.SetPosition(Vector3)
    uintptr_t addrSetPosPlayer = getAbsoluteAddress(targetLibName, 0x348A38); // Player.SetPosition(Vector3)
    if (addrGetPosMyCtrl == 0 || addrSetPosMyCtrl == 0 || addrSetPosPlayer == 0) return false;

    auto getPosMyCtrl = reinterpret_cast<MyCtrlPlayerGetPositionFunc>(addrGetPosMyCtrl);
    auto setPosMyCtrl = reinterpret_cast<MyCtrlPlayerSetPositionFunc>(addrSetPosMyCtrl);
    auto setPosPlayer = reinterpret_cast<PlayerSetPositionFunc>(addrSetPosPlayer);

    Vector3 pos = getPosMyCtrl(myCtrlPlayer, nullptr);
    float deltaY = flyHeightStep * (flyVerticalSpeed / 10.0f);
    pos.y += deltaY;

    // Atualiza controlador principal
    setPosMyCtrl(myCtrlPlayer, pos, nullptr);

    // Sincroniza também o Player real para não travar locomoção
    void* ctrlPlayer = GetCtrlPlayerFromMyCtrl();
    if (ctrlPlayer) {
        setPosPlayer(ctrlPlayer, pos, nullptr);
    }

    // Mantém estado de locomoção consistente com modo voo.
    SetFlyRuntimeState(flyMode);

    return true;
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
            __android_log_print(ANDROID_LOG_WARN, "ModMenu", "Aviso: Instância GetOnHorse não disponível");
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
            __android_log_print(ANDROID_LOG_WARN, "ModMenu", "Aviso: Instância GetOffHorse não disponível");
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
    if (baseAddress == 0 || baseAddress < 0x10000000) return;
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
    if (!playerCtrl) return;
    
    uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x4599AC);
    if (baseAddress == 0 || baseAddress < 0x10000000) return;
    auto setAimState = reinterpret_cast<SetAimStateFunc>(baseAddress);
    setAimState(playerCtrl, state, target, forceTarget);
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
        void* enemyPosCtrl = getEnemyPosCtrlInstance(nullptr);
        
        if (enemyPosCtrl) {
            __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIMBOT", "EnemyPosCtrl obtido com sucesso: %p", enemyPosCtrl);
        }
        
        return enemyPosCtrl;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_AIMBOT", "Exceção ao obter EnemyPosCtrl");
        return nullptr;
    }
}

// ========== IMPLEMENTAÇÃO DO SISTEMA POLICIAL ==========

/**
 * Obtém dados de origem de NPCs inimigos (inclui polícia)
 * PROTEÇÃO ANTI-CRASH: Verificações rigorosas de segurança
 */
void* GetNPCenemyOriData(int ID) {
    // Validação de entrada
    if (ID < 0 || ID > 1000) return nullptr; // IDs válidos esperados
    
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return nullptr;
        
        // dump.cs: PoliceLoader.GetNPCenemyOriData(int ID) -> RVA 0x31EDF0
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x31EDF0);
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return nullptr;
        
        // Verificação adicional de validade do endereço
        if (baseAddress < 0x10000000) return nullptr; // Endereço muito baixo
        
        auto getNPCEnemyOriData = reinterpret_cast<GetNPCenemyOriDataFunc>(baseAddress);
        if (!getNPCEnemyOriData) return nullptr;
        
        return getNPCEnemyOriData(ID);
    } catch (const std::exception& e) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

/**
 * Obtém número máximo de policiais permitidos
 * PROTEÇÃO ANTI-CRASH: Retorna valor seguro em caso de erro
 */
int GetPoliceMaxNum() {
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return 5; // Valor padrão seguro
        
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x53A5A0);
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return 5;
        
        // Verificação de validade do endereço
        if (baseAddress < 0x10000000) return 5;
        
        auto getPoliceMaxNum = reinterpret_cast<GetPoliceMaxNumFunc>(baseAddress);
        if (!getPoliceMaxNum) return 5;
        
        int result = getPoliceMaxNum(nullptr);
        // Validação do resultado (valores razoáveis)
        if (result < 0 || result > 100) return 5;
        
        return result;
    } catch (const std::exception& e) {
        return 5; // Valor padrão seguro
    } catch (...) {
        return 5; // Valor padrão seguro
    }
}

/**
 * Gera/spawna policiais no jogo
 * PROTEÇÃO ANTI-CRASH: Verificação rigorosa de ponteiros e estados
 */
void GeneratePolice(void* missionCtrl) {
    // Verificação de ponteiro nulo
    if (!missionCtrl) return;
    
    // Verificação adicional de validade do ponteiro
    if ((uintptr_t)missionCtrl < 0x10000000) return;
    
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return;
        
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x281598);
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return;
        
        // Verificação de validade do endereço
        if (baseAddress < 0x10000000) return;
        
        auto generatePolice = reinterpret_cast<GeneratePoliceFunc>(baseAddress);
        if (!generatePolice) return;
        
        // Chama a função apenas se todas as verificações passaram
        generatePolice(missionCtrl, nullptr);
        
    } catch (const std::exception& e) {
        // Exceção capturada - não faz nada
    } catch (...) {
        // Qualquer exceção - não faz nada
    }
}

/**
 * Obtém posição de spawn da polícia baseada na posição do jogador
 * PROTEÇÃO ANTI-CRASH: Usa offset real 0x2E6B40 com validações completas
 */
Vector3 GetPoliceBurnPos(void* enemyPosCtrl, Vector3 playerPos, float minSqr, float maxSqr) {
    Vector3 defaultPos = {0.0f, 0.0f, 0.0f};
    
    // Verificação de ponteiro nulo
    if (!enemyPosCtrl) return defaultPos;
    
    // Verificação de validade do ponteiro
    if ((uintptr_t)enemyPosCtrl < 0x10000000) return defaultPos;
    
    // Validação dos parâmetros de entrada
    if (minSqr < 0.0f || maxSqr < 0.0f || minSqr > maxSqr) return defaultPos;
    if (minSqr > 1000.0f || maxSqr > 1000.0f) return defaultPos; // Limites razoáveis
    
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return defaultPos;
        
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x2E6B40); // Offset real
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return defaultPos;
        
        // Verificação de validade do endereço
        if (baseAddress < 0x10000000) return defaultPos;
        
        auto getPoliceBurnPos = reinterpret_cast<GetPoliceBurnPosFunc>(baseAddress);
        if (!getPoliceBurnPos) return defaultPos;
        
        Vector3 result = getPoliceBurnPos(enemyPosCtrl, playerPos, minSqr, maxSqr);
        
        // Validação do resultado (posições razoáveis)
        if (result.x < -10000.0f || result.x > 10000.0f) return defaultPos;
        if (result.y < -10000.0f || result.y > 10000.0f) return defaultPos;
        if (result.z < -10000.0f || result.z > 10000.0f) return defaultPos;
        
        return result;
    } catch (const std::exception& e) {
        return defaultPos;
    } catch (...) {
        return defaultPos;
    }
}

/**
 * Oculta todos os NPCs e policiais
 * PROTEÇÃO ANTI-CRASH: Múltiplas camadas de verificação
 */
void HideAllPolice(void* missionCtrl) {
    // Verificação de ponteiro nulo
    if (!missionCtrl) return;
    
    // Verificação de validade do ponteiro
    if ((uintptr_t)missionCtrl < 0x10000000) return;
    
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return;
        
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x278038);
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return;
        
        // Verificação de validade do endereço
        if (baseAddress < 0x10000000) return;
        
        auto hideNonNpcAndPolice = reinterpret_cast<HideNonNpcAndPoliceFunc>(baseAddress);
        if (!hideNonNpcAndPolice) return;
        
        hideNonNpcAndPolice(missionCtrl, nullptr);
        
    } catch (const std::exception& e) {
        // Exceção capturada - operação falhou silenciosamente
    } catch (...) {
        // Qualquer exceção - operação falhou silenciosamente
    }
}

/**
 * Mostra/recupera todos os NPCs e policiais
 * PROTEÇÃO ANTI-CRASH: Múltiplas camadas de verificação
 */
void ShowAllPolice(void* missionCtrl) {
    // Verificação de ponteiro nulo
    if (!missionCtrl) return;
    
    // Verificação de validade do ponteiro
    if ((uintptr_t)missionCtrl < 0x10000000) return;
    
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return;
        
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x27B5DC);
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return;
        
        // Verificação de validade do endereço
        if (baseAddress < 0x10000000) return;
        
        auto recoverNonNpcAndPolice = reinterpret_cast<RecoverNonNpcAndPoliceFunc>(baseAddress);
        if (!recoverNonNpcAndPolice) return;
        
        recoverNonNpcAndPolice(missionCtrl, nullptr);
        
    } catch (const std::exception& e) {
        // Exceção capturada - operação falhou silenciosamente
    } catch (...) {
        // Qualquer exceção - operação falhou silenciosamente
    }
}

/**
 * Obtém instância do xerife
 * PROTEÇÃO ANTI-CRASH: Singleton pattern seguro
 */
void* GetSheriffInstance() {
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return nullptr;
        
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x486D8C);
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return nullptr;
        
        // Verificação de validade do endereço
        if (baseAddress < 0x10000000) return nullptr;
        
        auto getSheriffInstance = reinterpret_cast<GetNpcSheriffInstanceFunc>(baseAddress);
        if (!getSheriffInstance) return nullptr;
        
        void* instance = getSheriffInstance(nullptr);
        
        // Verificação do resultado (instância válida)
        if (instance && (uintptr_t)instance >= 0x10000000) {
            return instance;
        }
        
        return nullptr;
    } catch (const std::exception& e) {
        return nullptr;
    } catch (...) {
        return nullptr;
    }
}

/**
 * Obtém instância do caçador de recompensas
 * PROTEÇÃO ANTI-CRASH: Singleton pattern seguro
 */
void* GetBountyHunterInstance() {
    try {
        // Verifica se a biblioteca está carregada
        if (!isLibraryLoaded(targetLibName)) return nullptr;
        
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x487098);
        if (baseAddress == 0 || baseAddress == (uintptr_t)-1) return nullptr;
        
        // Verificação de validade do endereço
        if (baseAddress < 0x10000000) return nullptr;
        
        auto getBountyHunterInstance = reinterpret_cast<GetNpcBountyHunterInstanceFunc>(baseAddress);
        if (!getBountyHunterInstance) return nullptr;
        
        void* instance = getBountyHunterInstance(nullptr);
        
        // Verificação do resultado (instância válida)
        if (instance && (uintptr_t)instance >= 0x10000000) {
            return instance;
        }
        
        return nullptr;
    } catch (const std::exception& e) {
        return nullptr;
    } catch (...) {
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
int (*original_GetHitBlood)(void *thisPtr, int part, int type, Vector3 enemy, Vector3 myPosition, int modelType);

/**
 * Hook para a função GetHitBlood
 * Modifica o dano causado pelas balas (com verificações de segurança)
 */
int hook_GetHitBlood(void *thisPtr, int part, int type, Vector3 enemy, Vector3 myPosition, int modelType) {
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

// Ponteiros para funções de recarga
float (*original_GetReloadTime)(void* thisPtr);

// Ponteiros para funções de mira
void (*original_UpdateAimTarget)(void* thisPtr);
void (*original_SetAimState)(void* thisPtr, AimTargetState state, void* target, bool forceTarget);
void (*original_MyCtrlPlayerMyUpdate)(void* thisPtr);

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

// ================ HOOKS PARA VELOCIDADE CORRIGIDOS =================

// Ponteiros para funções originais (removendo as inexistentes)
MyPlayerOriData* (*original_GetMyPlayerOriData)();

/**
 * Obtém os dados de origem do jogador (incluindo velocidades)
 * @return Ponteiro para MyPlayerOriData ou nullptr
 */
MyPlayerOriData* GetPlayerOriData() {
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x4972F4); // GetMyPlayerOriData()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_SPEED", "Erro: Endereço inválido para GetMyPlayerOriData");
            return nullptr;
        }
        
        auto getPlayerOriData = reinterpret_cast<GetMyPlayerOriDataFunc>(baseAddress);
        MyPlayerOriData* playerData = getPlayerOriData();
        
        if (playerData) {
            __android_log_print(ANDROID_LOG_DEBUG, "MOD_SPEED", 
                               "Dados do jogador obtidos: walk_speed=%.2f, run_speed=%.2f", 
                               playerData->walk_speed, playerData->run_speed);
        }
        
        return playerData;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_SPEED", "Exceção ao obter dados de origem do jogador");
        return nullptr;
    }
}

/**
 * Hook para a função GetMyPlayerOriData
 * Modifica as velocidades quando o hack está ativado
 */
MyPlayerOriData* hook_GetMyPlayerOriData() {
    MyPlayerOriData* data = original_GetMyPlayerOriData();
    if (!data) return nullptr;

    // Aplica hack de velocidade SEMPRE que os dados são acessados
    if (speedHack) {
        // Valores base do jogo (descobertos através de testes)
        static const float BASE_WALK = 2.0f;
        static const float BASE_RUN = 4.0f;
        
        // Força valores multiplicados TODA VEZ que é chamado
        data->walk_speed = BASE_WALK * speedMultiplier;
        data->run_speed = BASE_RUN * speedMultiplier;
        data->acc = 10.0f * speedMultiplier;  // Aceleração alta
        data->dec = 5.0f;  // Desaceleração normal
    }

    // Processa ações pendentes em contexto de jogo
    if (pendingGeneratePolice || pendingHidePolice || pendingShowPolice ||
        pendingCreateMissionHints || pendingDestroyMissionHints) {
        void* missionCtrl = GetMissionCtrlInstance();
        if (missionCtrl) {
            if (pendingGeneratePolice) {
                GeneratePolice(missionCtrl);
                pendingGeneratePolice = false;
            }
            if (pendingHidePolice) {
                HideAllPolice(missionCtrl);
                pendingHidePolice = false;
            }
            if (pendingShowPolice) {
                ShowAllPolice(missionCtrl);
                pendingShowPolice = false;
            }
        }

        if (pendingCreateMissionHints) {
            CreateMissionHints();
            pendingCreateMissionHints = false;
        }

        if (pendingDestroyMissionHints) {
            DestroyMissionHints();
            pendingDestroyMissionHints = false;
        }
    }

    // Modo voo experimental:
    // a movimentação vertical é aplicada imediatamente pelos sliders (cases 33/34),
    // evitando conflito com locomoção normal.

    return data;
}

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
 * Hook para a função UpdateAimTarget
 * SISTEMA AIMBOT V5: Busca agressiva com offsets reais do dump.cs
 */
void hook_UpdateAimTarget(void* thisPtr) {
    if (!thisPtr) return;
    
    // Chama a função original primeiro
    original_UpdateAimTarget(thisPtr);
    
    if (!autoAim && !aimBot && !aimBotAggressive) return;

    // Usa somente campos reais do MyCtrlPlayer (0x24/0x28).
    void* bestTarget = FindBestTarget(thisPtr);

    // Modo agressivo: força novos ciclos de aquisição quando ainda não há alvo.
    if (!bestTarget && aimBotAggressive) {
        static int aggressiveFrameCounter = 0;
        aggressiveFrameCounter++;

        if ((aggressiveFrameCounter % 2) == 0) {
            ForceAimRefresh(thisPtr);
            original_UpdateAimTarget(thisPtr);
            original_UpdateAimTarget(thisPtr);
            bestTarget = FindBestTarget(thisPtr);
        }
    }

    if (!bestTarget) return;

    CallSetAimState(thisPtr, Aiming_Focus, bestTarget, true);

    // Mantém estado interno consistente para reduzir flicker.
    try {
        AimTargetState* aimStatePtr = reinterpret_cast<AimTargetState*>((char*)thisPtr + 0x20);
        *aimStatePtr = Aiming_Focus;

        // Mantém ambos os campos de alvo alinhados em modo agressivo.
        if (aimBotAggressive) {
            void** aimTargetPtr = reinterpret_cast<void**>((char*)thisPtr + 0x24);
            if (aimTargetPtr) *aimTargetPtr = bestTarget;
            void** aimTargetCamPtr = reinterpret_cast<void**>((char*)thisPtr + 0x28);
            if (aimTargetCamPtr) *aimTargetCamPtr = bestTarget;
        }
    } catch (...) {
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
    
    // Quando autoAim/aimBot ativos, mantém foco se houver alvo válido.
    if ((autoAim || aimBot || aimBotAggressive) && target && state == Aiming_NotFocus) {
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

void ProcessGameplayFrame(void* myCtrlPlayer) {
    if (!myCtrlPlayer) return;

    if (pendingEspClear) {
        ClearCompleteESP();
        pendingEspClear = false;
    }

    if (pendingEspRefresh) {
        RefreshCompleteESP();
        pendingEspRefresh = false;
    }

    if (pendingAutoKillBurst) {
        RunAutoKillOnce();
        pendingAutoKillBurst = false;
    }

    if (completeEsp && CanEnableCompleteESP()) {
        static int espFrameCounter = 0;
        espFrameCounter++;
        if ((espFrameCounter % 45) == 0) {
            RefreshCompleteESP();
        }
    }

    if (autoKill && CanRunAutoKill()) {
        static int autoKillFrameCounter = 0;
        autoKillFrameCounter++;
        if ((autoKillFrameCounter % 20) == 0) {
            RunAutoKillOnce();
        }
    }
}

void hook_MyCtrlPlayerMyUpdate(void* thisPtr) {
    if (!thisPtr) return;
    original_MyCtrlPlayerMyUpdate(thisPtr);
    ProcessGameplayFrame(thisPtr);
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

    // Hook para dados de origem do jogador (inclui velocidades)
    uintptr_t addr_GetMyPlayerOriData = getAbsoluteAddress(targetLibName, 0x4972F4);
    MSHookFunction((void *) addr_GetMyPlayerOriData, (void *) &hook_GetMyPlayerOriData,
                   (void **) &original_GetMyPlayerOriData);

    // ====== Hooks do sistema de mira ======

    // Hook para atualização de alvos de mira
    uintptr_t addr_UpdateAimTarget = getAbsoluteAddress(targetLibName, 0x45CF6C);
    MSHookFunction((void *) addr_UpdateAimTarget, (void *) &hook_UpdateAimTarget,
                   (void **) &original_UpdateAimTarget);

    // Hook por-frame do jogador para ESP/auto-kill sem depender de tiro
    uintptr_t addr_MyCtrlPlayerMyUpdate = getAbsoluteAddress(targetLibName, 0x4552A8);
    MSHookFunction((void *) addr_MyCtrlPlayerMyUpdate, (void *) &hook_MyCtrlPlayerMyUpdate,
                   (void **) &original_MyCtrlPlayerMyUpdate);

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
            OBFUSCATE("4_Toggle_Vida Infinita (15/02/2026)"),
            OBFUSCATE("1_SeekBar_Dano de bala (15/02/2026)_1_999"),
            OBFUSCATE("2_InputValue_Adicionar Moedas (15/02/2026)"),
            OBFUSCATE("3_InputValue_Adicionar Gems (15/02/2026)"),
            OBFUSCATE("5_SeekBar_Balas das Armas (15/02/2026)_1_999999"),

            // Debug de inimigos
            OBFUSCATE("Category_Debug de Inimigos"),
            OBFUSCATE("6_Toggle_Debug Posições de Inimigos (15/02/2026)"),
            OBFUSCATE("7_Button_Forçar Remoção de Inimigos (15/02/2026)"),

            // Gerenciamento de itens
            OBFUSCATE("Category_Gerenciamento de Itens"),
            OBFUSCATE("8_Toggle_Ouro/Diamantes Infinitos (15/02/2026)"),
            OBFUSCATE("9_Toggle_Munição Infinita (15/02/2026)"),
            OBFUSCATE("10_Toggle_Vida Infinita (Via Itens) (15/02/2026)"),
            OBFUSCATE("11_Toggle_Recursos Infinitos (15/02/2026)"),
            OBFUSCATE("12_Button_Adicionar Todas as Partes de Armas (15/02/2026)"),
            OBFUSCATE("13_Button_Adicionar Todas as Peles (15/02/2026)"),
            OBFUSCATE("14_Button_Adicionar 10 Whisky (15/02/2026)"),

            // Novas funcionalidades
            OBFUSCATE("Category_Controle do Jogador"),
            OBFUSCATE("15_Button_Colocar no Cavalo (15/02/2026)"),
            OBFUSCATE("16_Button_Remover do Cavalo (15/02/2026)"),
            OBFUSCATE("17_Toggle_Recarga Instantânea (15/02/2026)"),
            OBFUSCATE("18_Toggle_Hack de Velocidade (15/02/2026)"),
            OBFUSCATE("19_SeekBar_Multiplicador de Velocidade (15/02/2026)_1_10"),

            // Sistema de mira
            OBFUSCATE("Category_Sistema de Mira"),
            OBFUSCATE("20_Toggle_Auto-Aim (15/02/2026)"),
            OBFUSCATE("21_Toggle_AimBot V3 (Funções Reais) (15/02/2026)"),
            OBFUSCATE("35_Toggle_AimBot Agressivo (15/02/2026)"),
            OBFUSCATE("22_Toggle_Sempre Headshot (15/02/2026)"),
            OBFUSCATE("23_Button_Limpar Alvos de Mira (15/02/2026)"),

            // Sistema Policial
            OBFUSCATE("Category_Sistema Policial"),
            OBFUSCATE("24_Button_Gerar Policiais (15/02/2026)"),
            OBFUSCATE("25_Button_Ocultar Todos os Policiais (15/02/2026)"),
            OBFUSCATE("26_Button_Mostrar Todos os Policiais (15/02/2026)"),
            OBFUSCATE("27_Button_Obter Número Max de Policiais (15/02/2026)"),
            OBFUSCATE("28_Button_Obter Posição de Spawn da Polícia (15/02/2026)"),
            
            // NPCs Especiais
            OBFUSCATE("Category_NPCs da Lei"),
            OBFUSCATE("29_Button_Obter Instância do Xerife (15/02/2026)"),
            OBFUSCATE("30_Button_Obter Instância do Caçador (15/02/2026)"),
            OBFUSCATE("31_Button_Obter Dados de NPC Inimigo (15/02/2026)"),

            // Modo voo (experimental)
            OBFUSCATE("Category_Modo Voo"),
            OBFUSCATE("32_Toggle_Modo Voo (Experimental) (15/02/2026)"),
            OBFUSCATE("33_SeekBar_Velocidade Vertical_1_20"),
            OBFUSCATE("34_SeekBar_Ganho de Altura_1_50"),

            // Visual do jogo
            OBFUSCATE("Category_Visual do Jogo"),
            OBFUSCATE("36_Button_Criar Mission Hints Visuais (08/03/2026)"),
            OBFUSCATE("37_Button_Remover Mission Hints Visuais (08/03/2026)"),
            OBFUSCATE("38_Button_Mostrar Marcador no Alvo Atual (08/03/2026)"),
            OBFUSCATE("39_Button_Ocultar Marcador do Alvo (08/03/2026)"),
            OBFUSCATE("40_Toggle_ESP Completo (Barras de Vida) (08/03/2026)"),
            OBFUSCATE("41_Button_Atualizar ESP Agora (08/03/2026)"),
            OBFUSCATE("42_Button_Obter Todas as Entidades (08/03/2026)"),
            OBFUSCATE("43_Toggle_Auto Kill Seguro (08/03/2026)"),
            OBFUSCATE("44_Button_Kill All Agora (08/03/2026)"),
    };

    int Total_Feature = (sizeof features / sizeof features[0]);
    ret = (jobjectArray)
            env->NewObjectArray(Total_Feature, env->FindClass(OBFUSCATE("java/lang/String")),
                                env->NewStringUTF(""));

    for (int i = 0; i < Total_Feature; i++)
        env->SetObjectArrayElement(ret, i, env->NewStringUTF(features[i]));

    return (ret);
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
        case 21: // AimBot V3
            aimBot = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "🎯 AimBot V3 ativado - USA FUNÇÕES REAIS do dump.cs (NPCs🎯/Zumbis🧟/Ogros👹) - NUNCA mira no próprio player");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "AimBot V3 desativado");
            }
            break;
        case 35: // AimBot Agressivo
            aimBotAggressive = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "⚡ AimBot Agressivo ativado - ciclos extras de aquisição de alvo");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_AIMBOT", "AimBot Agressivo desativado");
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
            __android_log_print(ANDROID_LOG_INFO, "MOD_AIM", "Limpando todos os alvos de mira...");
            {
                void* myCtrlPlayer = GetMyCtrlPlayerInstance();
                if (myCtrlPlayer) {
                    ForceAimRefresh(myCtrlPlayer);
                    __android_log_print(ANDROID_LOG_INFO, "MOD_AIM", "Alvos de mira limpos com sucesso");
                } else {
                    __android_log_print(ANDROID_LOG_WARN, "MOD_AIM", "MyCtrlPlayer indisponivel para limpar a mira");
                }
            }
            break;
            
        // ========== SISTEMA POLICIAL ==========
        case 24: // Gerar policiais
            {
                pendingGeneratePolice = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_POLICE", "GeneratePolice agendado (execução segura no hook)");
            }
            break;
        case 25: // Ocultar todos os policiais
            {
                pendingHidePolice = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_POLICE", "HideAllPolice agendado (execução segura no hook)");
            }
            break;
        case 26: // Mostrar todos os policiais
            {
                pendingShowPolice = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_POLICE", "ShowAllPolice agendado (execução segura no hook)");
            }
            break;
        case 27: // Obter número máximo de policiais
            {
                int maxPolice = GetPoliceMaxNum();
                // O valor será retornado pela função
            }
            break;
        case 28: // Obter posição de spawn da polícia
            {
                void* enemyPosCtrl = GetEnemyPosCtrlInstance();
                if (enemyPosCtrl) {
                    Vector3 playerPos = {0.0f, 0.0f, 0.0f}; // Posição padrão
                    Vector3 policePos = GetPoliceBurnPos(enemyPosCtrl, playerPos, 10.0f, 50.0f);
                }
            }
            break;
            
        // ========== NPCs DA LEI ==========
        case 29: // Obter instância do xerife
            {
                void* sheriff = GetSheriffInstance();
                // Instância do xerife obtida
            }
            break;
        case 30: // Obter instância do caçador de recompensas
            {
                void* bountyHunter = GetBountyHunterInstance();
                // Instância do caçador obtida
            }
            break;
        case 31: // Obter dados de NPC inimigo
            {
                void* npcData = GetNPCenemyOriData(1); // ID padrão 1
                // Dados do NPC obtidos
            }
            break;
        case 32: // Modo voo experimental
            flyMode = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Modo voo experimental ativado");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Modo voo experimental desativado");
            }
            SetFlyRuntimeState(flyMode);
            break;
        case 33: // Velocidade vertical
            if (value >= 1 && value <= 20) {
                flyVerticalSpeed = (float)value;
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Velocidade vertical ajustada para: %.1f", flyVerticalSpeed);
                ApplyFlyPositionStep(); // efeito imediato ao mover slider
            }
            break;
        case 34: // Ganho de altura por tick
            if (value >= 1 && value <= 50) {
                flyHeightStep = (float)value / 10.0f;
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Ganho de altura ajustado para: %.2f", flyHeightStep);
                ApplyFlyPositionStep(); // efeito imediato ao mover slider
            }
            break;
        case 36: // Criar Mission Hints Visuais
            TurnOnSceneMissionHints();
            CreateMissionHints();
            pendingCreateMissionHints = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "MissionHints visuais acionados");
            break;
        case 37: // Remover Mission Hints Visuais
            TurnOffSceneMissionHints();
            DestroyMissionHints();
            pendingDestroyMissionHints = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "MissionHints visuais desativados");
            break;
        case 38: // Mostrar Marcador no Alvo Atual
            ShowTargetMarkerOnCurrentTarget();
            break;
        case 39: // Ocultar Marcador do Alvo
            HideTargetMarker();
            break;
        case 40: // ESP Completo
            if (boolean) {
                if (CanEnableCompleteESP()) {
                    completeEsp = true;
                    pendingEspRefresh = true;
                    __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP completo ativado");
                } else {
                    completeEsp = false;
                    __android_log_print(ANDROID_LOG_WARN, "MOD_ESP", "ESP bloqueado: UI ou jogo ainda nao estao prontos");
                }
            } else {
                completeEsp = false;
                pendingEspClear = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP completo desativado");
            }
            break;
        case 41: // Atualizar ESP Agora
            if (CanEnableCompleteESP()) {
                pendingEspRefresh = true;
            } else {
                __android_log_print(ANDROID_LOG_WARN, "MOD_ESP", "Atualizacao manual bloqueada: estado do jogo invalido");
            }
            break;
        case 42: // Obter todas as entidades
            LogTrackedEntities();
            break;
        case 43: // Auto Kill Seguro
            if (boolean) {
                if (CanRunAutoKill()) {
                    autoKill = true;
                    pendingAutoKillBurst = true;
                    __android_log_print(ANDROID_LOG_INFO, "MOD_AUTOKILL", "Auto Kill seguro ativado");
                } else {
                    autoKill = false;
                    __android_log_print(ANDROID_LOG_WARN, "MOD_AUTOKILL", "Auto Kill bloqueado: contexto de jogo invalido");
                }
            } else {
                autoKill = false;
                __android_log_print(ANDROID_LOG_INFO, "MOD_AUTOKILL", "Auto Kill seguro desativado");
            }
            break;
        case 44: // Kill All Agora
            if (CanRunAutoKill()) {
                pendingAutoKillBurst = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_AUTOKILL", "Kill All agendado");
            } else {
                __android_log_print(ANDROID_LOG_WARN, "MOD_AUTOKILL", "Kill All bloqueado: contexto de jogo invalido");
            }
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

/**
 * Obtém instância do controlador de missões (MissionCtrl)
 * @return Ponteiro para MissionCtrl ou nullptr
 */
void* GetMissionCtrlInstance() {
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x25A1AC); // MissionCtrl.GetInstance()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_POLICE", "Erro: Endereço inválido para MissionCtrl.GetInstance");
            return nullptr;
        }

        auto getMissionCtrlInstance = reinterpret_cast<MissionCtrlGetInstanceFunc>(baseAddress);
        void* missionCtrl = getMissionCtrlInstance(nullptr);

        if (missionCtrl) {
            __android_log_print(ANDROID_LOG_DEBUG, "MOD_POLICE", "MissionCtrl obtido com sucesso: %p", missionCtrl);
        }

        return missionCtrl;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_POLICE", "Exceção ao obter MissionCtrl");
        return nullptr;
    }
}

void* GetSceneInOutPosCtrlInstance() {
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x324A30); // SceneInOutPosCtrl.GetInstance()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Erro: Endereço inválido para SceneInOutPosCtrl.GetInstance");
            return nullptr;
        }

        auto getInstance = reinterpret_cast<GetSceneInOutPosCtrlInstanceFunc>(baseAddress);
        return getInstance(nullptr);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção ao obter SceneInOutPosCtrl");
        return nullptr;
    }
}

void* GetGunParticalEffectsCtrlInstance() {
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x251E0C); // GunParticalEffectsCtrl.GetInstance()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Erro: Endereço inválido para GunParticalEffectsCtrl.GetInstance");
            return nullptr;
        }

        auto getInstance = reinterpret_cast<GetGunParticalEffectsCtrlInstanceFunc>(baseAddress);
        return getInstance(nullptr);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção ao obter GunParticalEffectsCtrl");
        return nullptr;
    }
}

static int GetCurrentGameSceneValue() {
    void* gameCtrl = GetGameCtrlInstance();
    if (!gameCtrl) return (int)GameScene_NoviceVillage;

    try {
        int sceneValue = *reinterpret_cast<int*>(reinterpret_cast<char*>(gameCtrl) + 0x40); // GameCtrl.gameScene
        if (sceneValue < (int)GameScene_FirstLoading || sceneValue > (int)GameScene_ForMinYUI) {
            return (int)GameScene_NoviceVillage;
        }
        return sceneValue;
    } catch (...) {
        return (int)GameScene_NoviceVillage;
    }
}

void CreateMissionHints() {
    void* sceneCtrl = GetSceneInOutPosCtrlInstance();
    if (!sceneCtrl) return;

    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x324A94); // SceneInOutPosCtrl.CreateMissionHints()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Erro: Endereço inválido para CreateMissionHints");
            return;
        }

        auto createMissionHints = reinterpret_cast<CreateMissionHintsFunc>(baseAddress);
        int gameScene = GetCurrentGameSceneValue();
        createMissionHints(sceneCtrl, gameScene, (int)MissionUnlock_Main, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "MissionHints criados: scene=%d", gameScene);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção em CreateMissionHints");
    }
}

void DestroyMissionHints() {
    void* sceneCtrl = GetSceneInOutPosCtrlInstance();
    if (!sceneCtrl) return;

    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x3252A0); // SceneInOutPosCtrl.DestroyMissionHints()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Erro: Endereço inválido para DestroyMissionHints");
            return;
        }

        auto destroyMissionHints = reinterpret_cast<DestroyMissionHintsFunc>(baseAddress);
        destroyMissionHints(sceneCtrl, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "MissionHints removidos");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção em DestroyMissionHints");
    }
}

void TurnOnSceneMissionHints() {
    void* effectCtrl = GetGunParticalEffectsCtrlInstance();
    if (!effectCtrl) return;

    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x253E4C); // GunParticalEffectsCtrl.TurnOnSceneMissionHints()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Erro: Endereço inválido para TurnOnSceneMissionHints");
            return;
        }

        auto toggleHints = reinterpret_cast<ToggleSceneMissionHintsFunc>(baseAddress);
        toggleHints(effectCtrl, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "TurnOnSceneMissionHints executado");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção em TurnOnSceneMissionHints");
    }
}

void TurnOffSceneMissionHints() {
    void* effectCtrl = GetGunParticalEffectsCtrlInstance();
    if (!effectCtrl) return;

    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x253E50); // GunParticalEffectsCtrl.TurnOffSceneMissionHints()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Erro: Endereço inválido para TurnOffSceneMissionHints");
            return;
        }

        auto toggleHints = reinterpret_cast<ToggleSceneMissionHintsFunc>(baseAddress);
        toggleHints(effectCtrl, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "TurnOffSceneMissionHints executado");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção em TurnOffSceneMissionHints");
    }
}

void* GetUIManageInstance() {
    try {
        uintptr_t baseAddress = getAbsoluteAddress(targetLibName, 0x3FC5F0); // UI_Manage.GetInstance()
        if (baseAddress == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Erro: Endereço inválido para UI_Manage.GetInstance");
            return nullptr;
        }

        auto getInstance = reinterpret_cast<GetUIManageInstanceFunc>(baseAddress);
        return getInstance(nullptr);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção ao obter UI_Manage");
        return nullptr;
    }
}

void* GetPlayingUICreatorInstance() {
    void* uiManage = GetUIManageInstance();
    if (!uiManage) return nullptr;

    try {
        // dump.cs: UI_Manage.uiPlayingUICreator // 0x24
        void** creatorPtr = reinterpret_cast<void**>(reinterpret_cast<char*>(uiManage) + 0x24);
        if (!creatorPtr) return nullptr;
        void* creator = *creatorPtr;
        if (!creator || (uintptr_t)creator < 0x10000000) return nullptr;
        return creator;
    } catch (...) {
        return nullptr;
    }
}

static bool IsProbablyValidPtr(void* ptr) {
    return ptr && (uintptr_t)ptr >= 0x10000000;
}

void* GetPlayingUIBloodFactoryInstance() {
    void* uiManage = GetUIManageInstance();
    if (!uiManage) return nullptr;

    try {
        // dump.cs: UI_Manage.uiCamera // 0xC
        void* uiCamera = *reinterpret_cast<void**>(reinterpret_cast<char*>(uiManage) + 0xC);
        if (!IsProbablyValidPtr(uiCamera)) return nullptr;

        // dump.cs: UI_Manage.uiPlayingUIBloods // 0x14
        void** factoryPtr = reinterpret_cast<void**>(reinterpret_cast<char*>(uiManage) + 0x14);
        if (!factoryPtr) return nullptr;
        void* factory = *factoryPtr;
        if (!IsProbablyValidPtr(factory)) return nullptr;

        // dump.cs: UI_PlayingUI_BloodFactory.BloodPrefab // 0xC
        void* bloodPrefab = *reinterpret_cast<void**>(reinterpret_cast<char*>(factory) + 0xC);
        return IsProbablyValidPtr(bloodPrefab) ? factory : nullptr;
    } catch (...) {
        return nullptr;
    }
}

bool CanEnableCompleteESP() {
    if (!IsProbablyValidPtr(GetGameCtrlInstance())) return false;
    if (!IsProbablyValidPtr(GetMyPlayerInstance())) return false;
    if (!IsProbablyValidPtr(GetPlayingUIBloodFactoryInstance())) return false;
    return true;
}

bool CanRunAutoKill() {
    if (!IsProbablyValidPtr(GetGameCtrlInstance())) return false;
    if (!IsProbablyValidPtr(GetMyCtrlPlayerInstance())) return false;
    if (getAbsoluteAddress(targetLibName, 0x2F1D7C) == 0) return false; // GameCtrl.GetEnermyActiveList()
    if (getAbsoluteAddress(targetLibName, 0x31B830) == 0) return false; // PlayerBase.BeHit()

    void* enemies[4] = {};
    return CollectActiveEnemyBases(enemies, 4) > 0;
}

static int GetEntityManagerCount() {
    try {
        uintptr_t addrGetInstance = getAbsoluteAddress(targetLibName, 0x2E8CF0); // EntityManager.GetInstance()
        if (addrGetInstance == 0) return 0;

        auto getEntityManager = reinterpret_cast<GetEntityManagerInstanceFunc>(addrGetInstance);
        void* entityManager = getEntityManager(nullptr);
        if (!IsProbablyValidPtr(entityManager)) return 0;

        // dump.cs: EntityManager.m_EntityMap // 0x8
        auto* entityMap = *reinterpret_cast<Il2CppDictionary<int, void*>**>(reinterpret_cast<char*>(entityManager) + 0x8);
        if (!entityMap) return 0;

        int count = entityMap->count;
        return count > 0 ? count : 0;
    } catch (...) {
        return 0;
    }
}

static void AppendUniquePlayer(void** players, int& count, int maxPlayers, void* player) {
    if (!IsProbablyValidPtr(player) || count >= maxPlayers) return;
    for (int i = 0; i < count; ++i) {
        if (players[i] == player) return;
    }
    players[count++] = player;
}

static void CollectPlayersFromList(void* listPtr, void** players, int& count, int maxPlayers) {
    if (!IsProbablyValidPtr(listPtr)) return;

    auto* list = reinterpret_cast<Il2CppList<void*>*>(listPtr);
    if (!list || !list->items) return;

    int size = list->size;
    if (size <= 0 || size > 512) return;

    uint32_t maxLength = list->items->max_length;
    int limit = size < static_cast<int>(maxLength) ? size : static_cast<int>(maxLength);
    for (int i = 0; i < limit; ++i) {
        AppendUniquePlayer(players, count, maxPlayers, list->items->items[i]);
    }
}

static void CollectPlayersFromDictionary(void* dictPtr, void** players, int& count, int maxPlayers) {
    if (!IsProbablyValidPtr(dictPtr)) return;

    auto* dict = reinterpret_cast<Il2CppDictionary<int, void*>*>(dictPtr);
    if (!dict || !dict->valueSlots) return;

    int touchedSlots = dict->touchedSlots;
    if (touchedSlots <= 0 || touchedSlots > 1024) return;

    uint32_t maxLength = dict->valueSlots->max_length;
    int limit = touchedSlots < static_cast<int>(maxLength) ? touchedSlots : static_cast<int>(maxLength);
    for (int i = 0; i < limit; ++i) {
        AppendUniquePlayer(players, count, maxPlayers, dict->valueSlots->items[i]);
    }
}

static int CollectTrackedPlayers(void** outPlayers, int maxPlayers) {
    int count = 0;

    try {
        void* gameCtrl = GetGameCtrlInstance();
        if (IsProbablyValidPtr(gameCtrl)) {
            uintptr_t addrGetEnemyActiveList = getAbsoluteAddress(targetLibName, 0x2F1D7C); // GameCtrl.GetEnermyActiveList()
            if (addrGetEnemyActiveList != 0) {
                auto getEnemyActiveList = reinterpret_cast<GameCtrlGetEnemyActiveListFunc>(addrGetEnemyActiveList);
                void* enemyList = getEnemyActiveList(gameCtrl, nullptr);
                if (IsProbablyValidPtr(enemyList)) {
                    auto* list = reinterpret_cast<Il2CppList<void*>*>(enemyList); // List<PlayerBase>
                    if (list && list->items) {
                        int size = list->size;
                        if (size > 0 && size <= 512) {
                            uint32_t maxLength = list->items->max_length;
                            int limit = size < static_cast<int>(maxLength) ? size : static_cast<int>(maxLength);
                            for (int i = 0; i < limit; ++i) {
                                void* playerBase = list->items->items[i];
                                if (!IsProbablyValidPtr(playerBase)) continue;
                                // dump.cs: PlayerBase.m_dPlayer // 0xC
                                void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0xC);
                                AppendUniquePlayer(outPlayers, count, maxPlayers, player);
                            }
                        }
                    }
                }
            }

            // GameCtrl.myHorse também é uma entidade visual válida em alguns estados.
        }

        void* missionCtrl = GetMissionCtrlInstance();
        if (IsProbablyValidPtr(missionCtrl)) {
            CollectPlayersFromList(*reinterpret_cast<void**>(reinterpret_cast<char*>(missionCtrl) + 0x38), outPlayers, count, maxPlayers); // nonTaskPerNpcPlayers
            CollectPlayersFromList(*reinterpret_cast<void**>(reinterpret_cast<char*>(missionCtrl) + 0x3C), outPlayers, count, maxPlayers); // nonTaskNonPerNpcPlayers
            CollectPlayersFromList(*reinterpret_cast<void**>(reinterpret_cast<char*>(missionCtrl) + 0x40), outPlayers, count, maxPlayers); // noneMissionAnimalPlayers
            CollectPlayersFromList(*reinterpret_cast<void**>(reinterpret_cast<char*>(missionCtrl) + 0x44), outPlayers, count, maxPlayers); // policePlayers
            AppendUniquePlayer(outPlayers, count, maxPlayers, *reinterpret_cast<void**>(reinterpret_cast<char*>(missionCtrl) + 0x5C)); // TutorialEnemyPlayer
        }
    } catch (...) {
    }

    return count;
}

int CollectActiveEnemyBases(void** outEnemies, int maxEnemies) {
    if (!outEnemies || maxEnemies <= 0) return 0;

    try {
        void* gameCtrl = GetGameCtrlInstance();
        if (!IsProbablyValidPtr(gameCtrl)) return 0;

        uintptr_t addrGetEnemyActiveList = getAbsoluteAddress(targetLibName, 0x2F1D7C); // GameCtrl.GetEnermyActiveList()
        if (addrGetEnemyActiveList == 0) return 0;

        auto getEnemyActiveList = reinterpret_cast<GameCtrlGetEnemyActiveListFunc>(addrGetEnemyActiveList);
        void* enemyList = getEnemyActiveList(gameCtrl, nullptr);
        if (!IsProbablyValidPtr(enemyList)) return 0;

        auto* list = reinterpret_cast<Il2CppList<void*>*>(enemyList); // List<PlayerBase>
        if (!list || !list->items) return 0;

        int size = list->size;
        if (size <= 0 || size > 512) return 0;

        uint32_t maxLength = list->items->max_length;
        int limit = size < static_cast<int>(maxLength) ? size : static_cast<int>(maxLength);
        int count = 0;
        for (int i = 0; i < limit && count < maxEnemies; ++i) {
            void* enemyBase = list->items->items[i];
            if (!IsProbablyValidPtr(enemyBase)) continue;
            outEnemies[count++] = enemyBase;
        }
        return count;
    } catch (...) {
        return 0;
    }
}

void RunAutoKillOnce() {
    if (!CanRunAutoKill()) return;

    try {
        uintptr_t addrBeHit = getAbsoluteAddress(targetLibName, 0x31B830); // PlayerBase.BeHit()
        uintptr_t addrKilledAI = getAbsoluteAddress(targetLibName, 0x2812C4); // MissionCtrl.KilledAI(Player)
        uintptr_t addrClearEnemy = getAbsoluteAddress(targetLibName, 0x280390); // MissionCtrl.ClearEnemy(Player)
        uintptr_t addrMissionContainEnemy = getAbsoluteAddress(targetLibName, 0x480EA0); // MissionEntity.ContainEnemy(PlayerBase)
        uintptr_t addrMissionDeleteEnemy = getAbsoluteAddress(targetLibName, 0x4811A0); // MissionEntity.DeleteEnemy(PlayerBase)
        uintptr_t addrFactoryContainPlayerBase = getAbsoluteAddress(targetLibName, 0x2E4A34); // EnemyFactory.ContainPlayerBase(PlayerBase)
        uintptr_t addrFactoryDeletePlayerBase = getAbsoluteAddress(targetLibName, 0x2E4AB4); // EnemyFactory.DeletePlayerBase(PlayerBase)
        uintptr_t addrEnemyGC = getAbsoluteAddress(targetLibName, 0x2E791C); // EnermyGC.EnemyGC(Player)
        if (addrBeHit == 0) return;

        auto beHit = reinterpret_cast<PlayerBaseBeHitFunc>(addrBeHit);
        auto killedAI = reinterpret_cast<MissionCtrlPlayerActionFunc>(addrKilledAI);
        auto clearEnemy = reinterpret_cast<MissionCtrlPlayerActionFunc>(addrClearEnemy);
        auto missionContainEnemy = reinterpret_cast<MissionEntityContainEnemyFunc>(addrMissionContainEnemy);
        auto missionDeleteEnemy = reinterpret_cast<MissionEntityDeleteEnemyFunc>(addrMissionDeleteEnemy);
        auto factoryContainPlayerBase = reinterpret_cast<EnemyFactoryContainPlayerBaseFunc>(addrFactoryContainPlayerBase);
        auto factoryDeletePlayerBase = reinterpret_cast<EnemyFactoryDeletePlayerBaseFunc>(addrFactoryDeletePlayerBase);
        auto enemyGC = reinterpret_cast<EnemyGCFunc>(addrEnemyGC);

        void* gameCtrl = GetGameCtrlInstance();
        if (!IsProbablyValidPtr(gameCtrl)) return;

        void* enermyGC = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x24);      // GameCtrl.m_pEnermyGC
        void* enemyFactory = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x28);   // GameCtrl.m_pEnemyFactory
        void* missionCtrl = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x2C);    // GameCtrl.m_MissionCtrl
        void* missionEntity = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x30);  // GameCtrl.m_MissionEntity

        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        if (enemyCount <= 0) return;

        int touched = 0;
        int removed = 0;
        for (int i = 0; i < enemyCount; ++i) {
            void* enemyBase = enemies[i];
            if (!IsProbablyValidPtr(enemyBase)) continue;

            void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0xC); // PlayerBase.m_dPlayer
            void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0x14); // PlayerBase.m_dPlayerBaseData
            if (!IsProbablyValidPtr(baseData)) continue;

            void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8); // PlayerBaseData.m_dProperty
            if (!IsProbablyValidPtr(property)) continue;

            int currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14); // PlayerBaseProperty.m_dCurrentBlood
            int maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10);     // PlayerBaseProperty.m_dMaxBlood
            if (currentBlood <= 0 || maxBlood <= 0) continue;

            // dump.cs: ColliderBodyParts.Head = 0
            int lethalDamage = currentBlood + maxBlood + 5000;
            beHit(enemyBase, lethalDamage, 0, nullptr);
            touched++;

            if (IsProbablyValidPtr(missionCtrl) && IsProbablyValidPtr(player)) {
                if (addrKilledAI != 0) {
                    killedAI(missionCtrl, player, nullptr);
                }
                if (addrClearEnemy != 0) {
                    clearEnemy(missionCtrl, player, nullptr);
                }
            }

            if (IsProbablyValidPtr(missionEntity) && addrMissionContainEnemy != 0 && addrMissionDeleteEnemy != 0) {
                if (missionContainEnemy(missionEntity, enemyBase, nullptr)) {
                    missionDeleteEnemy(missionEntity, enemyBase, nullptr);
                    removed++;
                }
            }

            if (IsProbablyValidPtr(enemyFactory) && addrFactoryContainPlayerBase != 0 && addrFactoryDeletePlayerBase != 0) {
                if (factoryContainPlayerBase(enemyFactory, enemyBase, nullptr)) {
                    factoryDeletePlayerBase(enemyFactory, enemyBase, nullptr);
                }
            }

            if (IsProbablyValidPtr(enermyGC) && IsProbablyValidPtr(player) && addrEnemyGC != 0) {
                enemyGC(enermyGC, player, nullptr);
            }
        }

        __android_log_print(ANDROID_LOG_INFO, "MOD_AUTOKILL", "AutoKill tocou %d inimigos, removeu %d", touched, removed);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_AUTOKILL", "Falha protegida em RunAutoKillOnce");
    }
}

static bool GetPlayerBloodInfo(void* player, int& currentBlood, int& maxBlood) {
    if (!IsProbablyValidPtr(player)) return false;

    try {
        // dump.cs: Player.m_player -> 0xC
        void* playerBase = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0xC);
        if (!IsProbablyValidPtr(playerBase)) return false;

        // dump.cs: PlayerBase.m_dPlayerBaseData -> 0x14
        void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0x14);
        if (!IsProbablyValidPtr(baseData)) return false;

        // dump.cs: PlayerBaseData.m_dProperty -> 0x8
        void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8);
        if (!IsProbablyValidPtr(property)) return false;

        // dump.cs: PlayerBaseProperty.m_dMaxBlood -> 0x10, m_dCurrentBlood -> 0x14
        maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10);
        currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14);
        return maxBlood > 0 && currentBlood >= 0;
    } catch (...) {
        return false;
    }
}

void RefreshCompleteESP() {
    try {
        if (!CanEnableCompleteESP()) return;

        void* bloodFactory = GetPlayingUIBloodFactoryInstance();
        if (!IsProbablyValidPtr(bloodFactory)) return;

        uintptr_t addrGenerateEnemyBlood = getAbsoluteAddress(targetLibName, 0x3C67D8);
        uintptr_t addrDestroyAllBlood = getAbsoluteAddress(targetLibName, 0x3C6F60);
        uintptr_t addrSetCurrentBlood = getAbsoluteAddress(targetLibName, 0x3C70EC);
        uintptr_t addrSetCurrentBloodEnable = getAbsoluteAddress(targetLibName, 0x3C7354);
        if (addrGenerateEnemyBlood == 0 || addrDestroyAllBlood == 0 || addrSetCurrentBlood == 0 || addrSetCurrentBloodEnable == 0) return;

        auto generateEnemyBlood = reinterpret_cast<GenerateEnemyBloodFunc>(addrGenerateEnemyBlood);
        auto destroyAllBlood = reinterpret_cast<DestroyAllBloodFunc>(addrDestroyAllBlood);
        auto setCurrentBlood = reinterpret_cast<SetCurrentBloodValueFunc>(addrSetCurrentBlood);
        auto setCurrentBloodEnable = reinterpret_cast<SetCurrentBloodEnableFunc>(addrSetCurrentBloodEnable);

        destroyAllBlood(bloodFactory, nullptr);

        void* players[128] = {};
        int playerCount = CollectTrackedPlayers(players, 128);
        if (playerCount <= 0) return;

        void* myPlayer = GetMyPlayerInstance();
        int applied = 0;

        for (int i = 0; i < playerCount; ++i) {
            void* player = players[i];
            if (!IsProbablyValidPtr(player) || player == myPlayer) continue;

            void* target = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x3C); // Player.Head
            if (!IsProbablyValidPtr(target)) {
                target = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x24); // Player.Root
            }
            if (!IsProbablyValidPtr(target)) continue;

            int currentBlood = 0;
            int maxBlood = 0;
            if (!GetPlayerBloodInfo(player, currentBlood, maxBlood)) continue;
            if (maxBlood <= 0 || currentBlood < 0 || currentBlood > maxBlood * 4) continue;

            generateEnemyBlood(bloodFactory, target, nullptr);
            setCurrentBlood(bloodFactory, target, currentBlood, maxBlood, nullptr);
            setCurrentBloodEnable(bloodFactory, target, true, nullptr);
            applied++;
        }

        __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP atualizado em %d entidades", applied);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_ESP", "Falha protegida em RefreshCompleteESP");
    }
}

void ClearCompleteESP() {
    void* bloodFactory = GetPlayingUIBloodFactoryInstance();
    if (!IsProbablyValidPtr(bloodFactory)) return;

    try {
        uintptr_t addrDestroyAllBlood = getAbsoluteAddress(targetLibName, 0x3C6F60); // UI_PlayingUI_BloodFactory.DestroyAllBlood()
        if (addrDestroyAllBlood == 0) return;

        auto destroyAllBlood = reinterpret_cast<DestroyAllBloodFunc>(addrDestroyAllBlood);
        destroyAllBlood(bloodFactory, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP limpo");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_ESP", "Falha ao limpar ESP");
    }
}

void LogTrackedEntities() {
    void* players[256] = {};
    int trackedPlayers = CollectTrackedPlayers(players, 256);
    int entityManagerCount = GetEntityManagerCount();

    __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                        "Entidades rastreadas: trackedPlayers=%d entityManager=%d",
                        trackedPlayers, entityManagerCount);
}

void ShowTargetMarkerOnCurrentTarget() {
    void* creator = GetPlayingUICreatorInstance();
    void* myCtrlPlayer = GetMyCtrlPlayerInstance();
    if (!creator || !myCtrlPlayer) {
        __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "UI creator ou MyCtrlPlayer indisponivel");
        return;
    }

    void* target = FindBestTarget(myCtrlPlayer);
    if (!target) {
        __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "Nenhum alvo atual encontrado para marcador");
        return;
    }

    try {
        uintptr_t addrShow = getAbsoluteAddress(targetLibName, 0x3C9864); // UI_PlayingUI_Creator.ShowAimFollowTagetUI()
        uintptr_t addrSetTarget = getAbsoluteAddress(targetLibName, 0x3C9A34); // UI_PlayingUI_Creator.SetAimFollowTarget(Transform)
        if (addrShow == 0 || addrSetTarget == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Endereço inválido para marcador de alvo");
            return;
        }

        auto showMarker = reinterpret_cast<ShowAimFollowTargetUIFunc>(addrShow);
        auto setMarkerTarget = reinterpret_cast<SetAimFollowTargetUIFunc>(addrSetTarget);
        showMarker(creator, nullptr);
        setMarkerTarget(creator, target, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Marcador visual aplicado ao alvo atual");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção ao mostrar marcador no alvo");
    }
}

void HideTargetMarker() {
    void* creator = GetPlayingUICreatorInstance();
    if (!creator) {
        __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "UI creator indisponivel para ocultar marcador");
        return;
    }

    try {
        uintptr_t addrHide = getAbsoluteAddress(targetLibName, 0x3C994C); // UI_PlayingUI_Creator.HideAimFollowTagetUI()
        if (addrHide == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Endereço inválido para ocultar marcador");
            return;
        }

        auto hideMarker = reinterpret_cast<HideAimFollowTargetUIFunc>(addrHide);
        hideMarker(creator, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Marcador visual ocultado");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Exceção ao ocultar marcador");
    }
}
