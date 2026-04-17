#include <pthread.h>
#include <jni.h>
#include <unistd.h>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <android/log.h>
#include <dlfcn.h>
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include "Includes/Utils.h"
#include "KittyMemory/MemoryPatch.h"
#include "DialogOnLoad.h"
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
bool aimBotAggressive = false;       // ⚡ Aimbot agressivo (mais tentativas por frame)
bool alwaysHeadshot = false;         // 💀 Força todos os tiros como headshot
bool killNearbyOnWeaponSwap = false; // 🔄 Mata NPCs proximos ao trocar de arma
bool flyMode = false;                // 🕊️ Modo voo experimental
bool npcFlyMode = false;             // ☁️ Faz NPCs ativos levitarem sem depender de filtro hostil
bool npcWarMode = false;             // ⚔️ Faz NPCs hostis entrarem em conflito entre si
bool npcFlyHoverWave = false;        // 🌊 Oscilação leve na altura do hover dos NPCs
bool espCanvas = false;              // 🖼️ ESP desenhada via ESPView Java
bool completeEsp = false;            // 👁️ ESP completo com barras de vida
bool autoKill = false;               // ☠️ Mata inimigos ativos com BeHit
bool bulletTailEsp = false;          // 🔫 Desenha trilhas de tiro em NPCs armados
bool minimapEnemyEsp = false;        // 🗺️ ESP de inimigos no minimapa
bool autoClearPolice = false;        // 🚔 Limpa policiais continuamente
bool espShowBox = true;              // ⬛ Caixa 2D do ESP canvas
bool espShowSnapline = false;        // 📐 Linha da base da tela ate o alvo
bool espShowLabel = false;           // 🏷️ Texto com tag/distancia
bool espShowSkeleton = false;        // 🦴 Esqueleto 2D usando os bones expostos no Player
bool espShowOffscreen360 = false;    // 🧭 Indicador de borda para inimigos fora da tela
bool espShowOffscreenLabel = false;  // 🏷️ Texto do indicador 360 fora da tela
bool espShowOffscreenCount = false;  // 🔢 Quantidade de inimigos na mesma direcao do ESP 360
int sliderValue = 1, Moedas = 0, Gems = 0;
int espSnaplineOriginMode = 0;       // 0 = topo, 1 = centro, 2 = base
float flyVerticalSpeed = 5.0f;
float flyHeightStep = 1.0f;
float weaponSwapKillRadius = 8.0f;
float npcFlyLift = 1.2f;
float npcFlyHoverHeight = 1.6f;
float npcFlyDriftScale = 1.0f;
float npcFlyHoverResponsiveness = 0.8f;
float npcFlyHoverWaveAmplitude = 0.15f;
float espBoxThickness = 2.0f;
float espLineThickness = 1.4f;
float espTextSize = 18.0f;
float espSnaplineOffsetX = 0.0f;
float espSnaplineOffsetY = 30.0f;
int espColorRed = 255;
int espColorGreen = 90;
int espColorBlue = 90;
static constexpr int kMaxNpcFlightPlayers = 64;
void* npcFlightPlayers[kMaxNpcFlightPlayers] = {};
float npcFlightBaseY[kMaxNpcFlightPlayers] = {};
float npcFlightTargetY[kMaxNpcFlightPlayers] = {};
int npcFlightPlayerCount = 0;
jmethodID gEspDrawLineMethod = nullptr;
jmethodID gEspDrawRectMethod = nullptr;
jmethodID gEspDrawTextMethod = nullptr;
jmethodID gEspDrawCircleMethod = nullptr;
jmethodID gEspViewGetWidthMethod = nullptr;
jmethodID gEspViewGetHeightMethod = nullptr;

// Ações pendentes para execução em contexto de jogo
volatile bool pendingCreateMissionHints = false;
volatile bool pendingDestroyMissionHints = false;
volatile bool pendingEspRefresh = false;
volatile bool pendingEspClear = false;
volatile bool pendingAutoKillBurst = false;
volatile bool pendingBulletTailShot = false;
volatile bool pendingBulletTailClear = false;
volatile bool pendingMiniMapEspRefresh = false;
volatile bool pendingMiniMapEspClear = false;
volatile bool pendingShowWordsTest = false;
volatile bool pendingShowWordsCustom = false;
volatile bool pendingLoginSuccessHintSequence = false;
volatile bool pendingChaosTime = false;
volatile bool pendingChaosSpawn = false;
volatile bool pendingChaosUI = false;
volatile bool pendingChaosWeapon = false;
volatile bool pendingChaosFx = false;
volatile bool pendingKillPoliceOnly = false;
char pendingCustomWordsText[256] = {0};
char pendingLoginSuccessHints[12][128] = {0};

enum TeleportRequestMode {
    TeleportNone = 0,
    TeleportCurrentTarget = 1,
    TeleportNearestHostile = 2,
    TeleportNearestMissionNpc = 3,
    TeleportNearestMapNpc = 4
};

volatile int pendingTeleportRequest = TeleportNone;

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
typedef void (*PlayerSetNavMeshSpeedFunc)(void* thisPtr, float speed, void* method);
typedef void (*PlayerSetCurrentVelocityFunc)(void* thisPtr, Vector3 velocity, void* method);
typedef void (*PlayerTranslateWithSamplePosFunc)(void* thisPtr, Vector3 velocity, void* method);
typedef void (*PlayerDisableMoveColliderFunc)(void* thisPtr, void* method);
typedef void (*PlayerEnableMoveColliderFunc)(void* thisPtr, void* method);
typedef int (*PlayerGetCurrentGunTypeFunc)(void* thisPtr, void* method);
typedef Vector3 (*PlayerGetCurrentVelocityFunc)(void* thisPtr, void* method);
typedef bool (*PlayerIsMovingFunc)(void* thisPtr, void* method);
typedef Vector3 (*PlayerHeadingFunc)(void* thisPtr, void* method);
typedef void (*PlayerRotateToCameraDirFunc)(void* thisPtr, void* method);
typedef void (*PlayerSetRotationCameraDirFunc)(void* thisPtr, void* method);
typedef void (*PlayerSetCurrentRotationFunc)(void* thisPtr, Vector3 velocity, void* method);
typedef void (*PlayerRotateToDirFunc)(void* thisPtr, Vector3 dir, void* method);
typedef void (*PlayerStopRotateFunc)(void* thisPtr, void* method);
typedef Vector3 (*MyCtrlPlayerGetPositionFunc)(void* thisPtr, void* method);
typedef void (*MyCtrlPlayerSetPositionFunc)(void* thisPtr, Vector3 pos, void* method);
typedef Vector3 (*TransformGetPositionFunc)(void* thisPtr, void* method);
typedef Vector3 (*CameraWorldToScreenPointFunc)(void* thisPtr, Vector3 pos, void* method);
typedef int (*ScreenGetWidthFunc)();
typedef int (*ScreenGetHeightFunc)();
typedef void (*GameCtrlSetTimeScaleFunc)(float time, void* method);
typedef void (*GameCtrlRecoverTimeScaleFunc)(void* method);
typedef void (*GameCtrlChangedWeaponFunc)(void* thisPtr, int kindID, bool isGun, void* method);
typedef void (*GameCtrlSetHorseParamFunc)(void* thisPtr, int horseID, void* method);
typedef void (*GameCtrlChangeHorseFunc)(void* thisPtr, int horseID, void* method);
typedef void (*MissionCtrlGenerateTutorialEnemyFunc)(void* thisPtr, void* method);
typedef void (*MissionCtrlSimpleActionFunc)(void* thisPtr, void* method);
typedef void (*ShowHeadshootUIFunc)(void* thisPtr, void* method);
typedef void (*ShowMiniMapUIFunc)(void* thisPtr, void* method);
typedef void (*EnableAimCenterUIFunc)(void* thisPtr, bool active, void* method);
typedef void (*SetMissionTimeRegulatorFunc)(void* thisPtr, float time, void* method);
typedef void (*DisableMissionTimeRegulatorFunc)(void* thisPtr, void* method);
typedef void (*SetHorseSprintEnableFunc)(void* thisPtr, bool enable, void* method);
typedef void (*SetCurrentHorseSprintFunc)(void* thisPtr, float length, void* method);

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
    Sheriff = 18,         // ✅ Xerife
    BountyHunter = 25,    // ✅ Caçador de recompensas
    Robber = 26,          // ✅ Ladrão  
    Pro01 = 30,           // ✅ Profissional 1
    Pro02 = 31,           // ✅ Profissional 2
    Bid01 = 32,           // ✅ NPC hostil tipo Bid 1
    Bid02 = 33,           // ✅ NPC hostil tipo Bid 2
    
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

enum ScenePosType {
    ScenePosType_Null = 0,
    ScenePosType_NV_Square = 1,
    ScenePosType_NV_PoliceStation = 5
};

enum Effects {
    Effects_Null = 0,
    Effects_GunHitWood = 1,
    Effects_GunHitBlood = 2,
    Effects_MissionMainEffect = 3,
    Effects_MissionBranchEffect = 4,
    Effects_ShootPracticeEffect = 5,
    Effects_QuickDrawEffect = 6,
    Effects_HorseRideEffect = 7,
    Effects_DartEffect = 8,
    Effects_TwentyOneEffect = 9,
    Effects_DestinationEffect = 10,
    Effects_Gunshoot_Pistol = 11,
    Effects_Gunshoot_Rifle = 12,
    Effects_Gunshoot_Shotgun = 13
};

typedef void (*UpdateAimTargetFunc)(void* thisPtr);
typedef void (*SetAimStateFunc)(void* thisPtr, AimTargetState state, void* target, bool forceTarget);

// ========== TIPOS DE FUNÇÃO PARA SISTEMA POLICIAL ==========
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
typedef void (*EffectParamCtorFunc)(void* thisPtr, Vector3 pos, Vector3 rot, float duration, void* parent, int scenePosType, bool enableBoxCollider, void* method);
typedef void (*SceneInOutToggleEffectFunc)(void* thisPtr, void* method);
typedef void* (*GetUIManageInstanceFunc)(void* method);
typedef void (*ShowAimFollowTargetUIFunc)(void* thisPtr, void* method);
typedef void (*HideAimFollowTargetUIFunc)(void* thisPtr, void* method);
typedef void (*SetAimFollowTargetUIFunc)(void* thisPtr, void* target, void* method);
typedef void* (*GetEntityManagerInstanceFunc)(void* method);
typedef void* (*GameCtrlGetEnemyActiveListFunc)(void* thisPtr, void* method);
typedef void* (*GameCtrlGetHeadLookTargetFunc)(void* thisPtr, void* me, void* method);
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
typedef void* (*GetBulletTailFactoryInstanceFunc)(void* method);
typedef void (*GenerateEnemyBulletTailFunc)(void* thisPtr, Vector3 startPos, void* player, bool isHit, void* method);
typedef void (*DestroyAllBulletTailFunc)(void* thisPtr, void* method);
typedef void (*GenerateMiniMapHintsFunc)(void* thisPtr, void* param, void* method);
typedef void (*DestroyMiniMapHintByTargetFunc)(void* thisPtr, int type, void* target, bool isClearAll, void* method);
typedef void (*DestroyAllMiniMapHintsFunc)(void* thisPtr, void* method);
typedef void* (*MiniMapHintsCtorByTargetFunc)(void* thisPtr, int hintType, void* target, void* method);
typedef void (*GunParticalPlayEffectFunc)(void* thisPtr, int effect, void* param, void* colliderParam, void* method);
typedef void (*GunParticalDeleteEffectFunc)(void* thisPtr, int effect, int id, void* method);
typedef void (*GunParticalToggleEffectFunc)(void* thisPtr, int effect, void* method);
typedef void (*PlayWordsHintsWithTimeFunc)(void* thisPtr, void* str, float showTime, void* method);
typedef void* (*Il2CppStringNewFunc)(const char* str);
typedef int (*MissionCtrlGetStateFunc)(void* thisPtr, void* method);
typedef void* (*ComponentGetGameObjectFunc)(void* thisPtr, void* method);
typedef bool (*PropertyAddCanAttackLayerFunc)(void* thisPtr, void* targetGameObject, void* method);

// Declarações antecipadas das funções necessárias
void CallSetAimState(void* playerCtrl, AimTargetState state, void* target, bool forceTarget);

// ========== DECLARAÇÕES ANTECIPADAS - SISTEMA POLICIAL ==========
void GeneratePolice(void* missionCtrl);
void* GetMissionCtrlInstance();
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
bool CanRunAutoKill();
int CollectActiveEnemyBases(void** outEnemies, int maxEnemies);
void RunAutoKillOnce();
void RunWeaponSwapKillNearbyOnce(float radiusMeters);
void UpdateWeaponSwapKillMonitor();
void ProcessGameplayFrame(void* myCtrlPlayer);
void* GetBulletTailFactoryInstance();
static bool IsProbablyValidPtr(void* ptr);
static bool ResolvePlayerStartPos(void* myPlayer, Vector3& outStartPos);
static bool IsBulletTailCompatibleEnemyBase(void* enemyBase, void** outPlayer);
static bool PlayerContainsTransform(void* player, void* targetTransform);
static bool GetPlayerBloodInfo(void* player, int& currentBlood, int& maxBlood);
bool GenerateBulletTailForPlayer(void* factory, const Vector3& startPos, void* targetPlayer, bool isHit);
int GenerateBulletTailForAllActiveEnemies();
bool CanUseBulletTail();
void TriggerBulletTailNow();
void ClearBulletTailNow();
void* GetMiniMapIconCtrlInstance();
bool CanUseMiniMapEnemyEsp();
void RefreshMiniMapEnemyEsp();
void ClearMiniMapEnemyEsp();
void ShowWordsHintText(const char* text, float showTime);
void QueueLoginSuccessHints(const char* displayName, int remainingDays);
extern "C" jstring SubmitNativeLogin(JNIEnv* env, jclass clazz, jobject context, jstring email, jstring password);
extern "C" jstring GetNativeLoginSummary(JNIEnv* env, jclass clazz);

static long long GetMonotonicTimeMs() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<long long>(ts.tv_sec) * 1000LL + static_cast<long long>(ts.tv_nsec / 1000000LL);
}
void ProcessGameplayHints();
void RunChaosTimeOnce();
void RunChaosSpawnOnce();
void RunChaosUIOnce();
void RunChaosWeaponOnce();
void RunChaosFxOnce();
void RunKillPoliceOnlyOnce();
void RunNpcWarFrame();
bool RunTeleportToRequest(int requestMode);
void* ResolveBestAggressiveAimTarget(void* myCtrlPlayer);
void* GetGameCtrlInstance();
void* GetMyPlayerInstance();
bool GetPlayerWorldPosition(void* player, Vector3& outPos);
static bool CanAttackTargetTransform(void* myCtrlPlayer, void* targetTransform);
void* GetMainCameraInstance();
bool GetTransformWorldPosition(void* transform, Vector3& outPos);
bool WorldToScreen(const Vector3& worldPos, Vector3& outScreenPos);
bool ProjectPlayerScreenAnchors(void* player, Vector3& outHeadScreen, Vector3& outRootScreen);
void LogWorldToScreenSamples();
void RefreshTargetMarkerESP();
void DrawOn(JNIEnv* env, jobject espView, jobject canvas);
void ApplyFlyMovementFrame(void* myCtrlPlayer);
void ApplyEnemyFlyMovementFrame();
void SetEnemyFlightState(bool enabled);

struct ScreenBox {
    float left;
    float top;
    float right;
    float bottom;
};

static bool TryBuildScreenBoxForPlayer(void* player, ScreenBox& outBox);
static float Vector3LengthXZ(const Vector3& value);
static Vector3 NormalizeXZ(const Vector3& value);
static float ClampMagnitudeXZ(float value, float minValue, float maxValue);
static bool IsFlyableNpcPlayer(void* player);
static void ClearTrackedNpcFlightPlayers();
static bool IsTrackedNpcFlightPlayer(void* player);
static void TrackNpcFlightPlayer(void* player);
static void RestoreTrackedNpcFlightPlayers();
static void* GetPlayerBaseFromPlayer(void* player);
static void* ResolveTargetPlayerForBulletTail();
static void* ResolveNpcFlightTargetPlayer();
static void RestoreSingleEnemyFlightState(void* enemyPlayer);
static bool ResolvePlayerMetaFromPlayer(void* player, int& outBaseType, int& outAnimalType, int& outGunID, Vector3* outPos);
static void AppendUniquePlayer(void** outPlayers, int& count, int maxPlayers, void* player);
static int CollectLivingPlayers(void** outPlayers, int maxPlayers);
static bool IsTeleportHostileType(int baseType, int animalType, int gunID);
static void* ResolvePlayerFromTrackedTransform(void* targetTransform);
static void* ResolveTeleportTargetPlayer(int requestMode);
static bool TeleportMyPlayerNearTarget(void* targetPlayer, const char* reasonText);
static bool EnsureEspJniMethods(JNIEnv* env, jobject espView);
static bool IsHostileEspType(int baseType, int animalType, int gunID);
static void DrawEspLine(JNIEnv* env, jobject espView, jobject canvas, int a, int r, int g, int b, float stroke, float fromX, float fromY, float toX, float toY);
static void DrawEspRect(JNIEnv* env, jobject espView, jobject canvas, int a, int r, int g, int b, float stroke, float x, float y, float width, float height);
static void DrawEspText(JNIEnv* env, jobject espView, jobject canvas, int a, int r, int g, int b, const char* text, float x, float y, float size);

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

static void* GetPreferredAimTransformFromPlayer(void* player) {
    if (!IsProbablyValidPtr(player)) return nullptr;

    try {
        void* head = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x3C); // Player.Head
        if (IsProbablyValidPtr(head)) return head;

        void* neck = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x38); // Player.Neck
        if (IsProbablyValidPtr(neck)) return neck;

        void* body = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x28); // Player.Body
        if (IsProbablyValidPtr(body)) return body;

        void* root = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x24); // Player.Root
        if (IsProbablyValidPtr(root)) return root;
    } catch (...) {
    }

    return nullptr;
}

bool GetPlayerWorldPosition(void* player, Vector3& outPos) {
    if (!IsProbablyValidPtr(player)) return false;

    try {
        uintptr_t addrPlayerGetPosition = getAbsoluteAddress(targetLibName, 0x341378); // Player.GetPosition()
        if (addrPlayerGetPosition == 0) return false;
        auto getPlayerPosition = reinterpret_cast<PlayerGetPositionFunc>(addrPlayerGetPosition);
        outPos = getPlayerPosition(player, nullptr);
        return true;
    } catch (...) {
        return false;
    }
}

static bool CanAttackTargetTransform(void* myCtrlPlayer, void* targetTransform) {
    if (!IsProbablyValidPtr(myCtrlPlayer) || !IsProbablyValidPtr(targetTransform)) return false;

    try {
        void* myCtrlPlayerData = *reinterpret_cast<void**>(reinterpret_cast<char*>(myCtrlPlayer) + 0x14); // MyCtrlPlayer.m_dMyCtrlPlayerData
        if (!IsProbablyValidPtr(myCtrlPlayerData)) return false;

        void* propertyAdd = *reinterpret_cast<void**>(reinterpret_cast<char*>(myCtrlPlayerData) + 0x8); // MyCtrlPlayerData.m_dPropertyAdd
        if (!IsProbablyValidPtr(propertyAdd)) return false;

        uintptr_t addrGetGameObject = getAbsoluteAddress(targetLibName, 0x846AE0); // Component.get_gameObject()
        uintptr_t addrPlayerCanAttackLayer = getAbsoluteAddress(targetLibName, 0x457DFC); // PropertyAdd.PlayerCanAttackLayer(GameObject)
        if (addrGetGameObject == 0 || addrPlayerCanAttackLayer == 0) return false;

        auto componentGetGameObject = reinterpret_cast<ComponentGetGameObjectFunc>(addrGetGameObject);
        auto playerCanAttackLayer = reinterpret_cast<PropertyAddCanAttackLayerFunc>(addrPlayerCanAttackLayer);

        void* targetGameObject = componentGetGameObject(targetTransform, nullptr);
        if (!IsProbablyValidPtr(targetGameObject)) return false;

        return playerCanAttackLayer(propertyAdd, targetGameObject, nullptr);
    } catch (...) {
        return false;
    }
}

static int GetAggressivePriorityScore(int baseType, int animalType, int gunID) {
    switch (baseType) {
        case Ogre:
            return 1000 + ((animalType == OgreBoss) ? 200 : 0);
        case Zombies:
            return 800;
        case EnemyNPC:
            return 600 + ((gunID > 0) ? 120 : 0);
        case Animal:
            if (animalType == Bear || animalType == Wolf01 || animalType == Wolf02 || animalType == Wolf03 || animalType == Cheetah) {
                return 350;
            }
            return 150;
        case MissionPerson:
            return 120;
        case NonPermanentNpc:
            return 80;
        default:
            return -1;
    }
}

static bool IsSameCombatTarget(void* lhsPlayer, void* rhsPlayer) {
    return IsProbablyValidPtr(lhsPlayer) && IsProbablyValidPtr(rhsPlayer) && lhsPlayer == rhsPlayer;
}

static bool ResolveCombatData(void* enemyBase, void** outPlayer, int& outBaseType, int& outAnimalType, int& outGunID, Vector3& outPos) {
    if (outPlayer) *outPlayer = nullptr;
    outBaseType = -1;
    outAnimalType = -1;
    outGunID = 0;
    if (!IsProbablyValidPtr(enemyBase)) return false;

    try {
        void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0xC); // PlayerBase.m_dPlayer
        void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0x14); // PlayerBase.m_dPlayerBaseData
        if (!IsProbablyValidPtr(player) || !IsProbablyValidPtr(baseData)) return false;

        void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8); // PlayerBaseData.m_dProperty
        if (!IsProbablyValidPtr(property)) return false;

        outBaseType = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0xC);     // PlayerBaseProperty.baseType
        int maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10);    // PlayerBaseProperty.m_dMaxBlood
        int currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14);// PlayerBaseProperty.m_dCurrentBlood
        if (currentBlood <= 0 || maxBlood <= 0) return false;

        void* aiData = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0xC); // PlayerBaseData.m_dAIdata
        if (IsProbablyValidPtr(aiData)) {
            outAnimalType = *reinterpret_cast<int*>(reinterpret_cast<char*>(aiData) + 0x8); // AIdata.animalType
            outGunID = *reinterpret_cast<int*>(reinterpret_cast<char*>(aiData) + 0x10);      // AIdata.gunID
        }

        if (!GetPlayerWorldPosition(player, outPos)) return false;
        if (outPlayer) *outPlayer = player;
        return true;
    } catch (...) {
        return false;
    }
}

static bool ResolvePlayerMetaFromPlayer(void* player, int& outBaseType, int& outAnimalType, int& outGunID, Vector3* outPos) {
    outBaseType = -1;
    outAnimalType = -1;
    outGunID = 0;
    if (outPos) *outPos = {0.0f, 0.0f, 0.0f};
    if (!IsProbablyValidPtr(player)) return false;

    try {
        void* playerBase = GetPlayerBaseFromPlayer(player);
        if (!IsProbablyValidPtr(playerBase)) return false;

        void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0x14); // PlayerBase.m_dPlayerBaseData
        if (!IsProbablyValidPtr(baseData)) return false;

        void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8); // PlayerBaseData.m_dProperty
        if (!IsProbablyValidPtr(property)) return false;

        outBaseType = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0xC); // PlayerBaseProperty.baseType
        int maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10); // PlayerBaseProperty.m_dMaxBlood
        int currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14); // PlayerBaseProperty.m_dCurrentBlood
        if (currentBlood <= 0 || maxBlood <= 0) return false;

        void* aiData = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0xC); // PlayerBaseData.m_dAIdata
        if (IsProbablyValidPtr(aiData)) {
            outAnimalType = *reinterpret_cast<int*>(reinterpret_cast<char*>(aiData) + 0x8); // AIdata.animalType
            outGunID = *reinterpret_cast<int*>(reinterpret_cast<char*>(aiData) + 0x10); // AIdata.gunID
        }

        if (outPos && !GetPlayerWorldPosition(player, *outPos)) return false;
        return true;
    } catch (...) {
        return false;
    }
}

void* ResolveBestAggressiveAimTarget(void* myCtrlPlayer) {
    if (!IsProbablyValidPtr(myCtrlPlayer)) return nullptr;

    void* bestTarget = FindBestTarget(myCtrlPlayer);
    if (IsProbablyValidPtr(bestTarget) && CanAttackTargetTransform(myCtrlPlayer, bestTarget)) return bestTarget;

    try {
        void* myPlayer = GetMyPlayerInstance();
        if (!IsProbablyValidPtr(myPlayer)) return nullptr;

        static void* lockedTargetPlayer = nullptr;
        static int lockedFramesLeft = 0;
        Vector3 myPos = {0.0f, 0.0f, 0.0f};
        if (!GetPlayerWorldPosition(myPlayer, myPos)) return nullptr;

        if (IsProbablyValidPtr(lockedTargetPlayer) && lockedFramesLeft > 0) {
            int lockedBaseType = -1;
            int lockedAnimalType = -1;
            int lockedGunID = 0;
            Vector3 lockedPos = {0.0f, 0.0f, 0.0f};
            const bool lockedStillAlive = ResolvePlayerMetaFromPlayer(
                lockedTargetPlayer,
                lockedBaseType,
                lockedAnimalType,
                lockedGunID,
                &lockedPos
            );
            const int lockedScore = lockedStillAlive
                ? GetAggressivePriorityScore(lockedBaseType, lockedAnimalType, lockedGunID)
                : -1;

            void* lockedTransform = GetPreferredAimTransformFromPlayer(lockedTargetPlayer);
            if (
                lockedStillAlive &&
                lockedScore >= 0 &&
                IsProbablyValidPtr(lockedTransform) &&
                CanAttackTargetTransform(myCtrlPlayer, lockedTransform)
            ) {
                lockedFramesLeft--;
                return lockedTransform;
            }

            lockedTargetPlayer = nullptr;
            lockedFramesLeft = 0;
        }

        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        if (enemyCount <= 0) return nullptr;

        void* bestPlayer = nullptr;
        void* bestTransform = nullptr;
        int bestScore = -100000;
        float bestDistanceSq = 1.0e30f;

        for (int i = 0; i < enemyCount; ++i) {
            void* enemyPlayer = nullptr;
            int baseType = -1;
            int animalType = -1;
            int gunID = 0;
            Vector3 enemyPos = {0.0f, 0.0f, 0.0f};
            if (!ResolveCombatData(enemies[i], &enemyPlayer, baseType, animalType, gunID, enemyPos)) continue;
            if (!IsProbablyValidPtr(enemyPlayer) || enemyPlayer == myPlayer) continue;

            void* targetTransform = GetPreferredAimTransformFromPlayer(enemyPlayer);
            if (!IsProbablyValidPtr(targetTransform)) continue;
            if (!CanAttackTargetTransform(myCtrlPlayer, targetTransform)) continue;

            int score = GetAggressivePriorityScore(baseType, animalType, gunID);
            if (score < 0) continue;

            float dx = enemyPos.x - myPos.x;
            float dy = enemyPos.y - myPos.y;
            float dz = enemyPos.z - myPos.z;
            float distanceSq = dx * dx + dy * dy + dz * dz;

            if (score > bestScore || (score == bestScore && distanceSq < bestDistanceSq)) {
                bestScore = score;
                bestDistanceSq = distanceSq;
                bestPlayer = enemyPlayer;
                bestTransform = targetTransform;
            }
        }

        if (IsProbablyValidPtr(bestPlayer) && IsProbablyValidPtr(bestTransform)) {
            if (!IsSameCombatTarget(lockedTargetPlayer, bestPlayer)) {
                lockedTargetPlayer = bestPlayer;
                lockedFramesLeft = 18;
            }
            return bestTransform;
        }

        void* gameCtrl = GetGameCtrlInstance();
        if (!IsProbablyValidPtr(gameCtrl)) return nullptr;
        uintptr_t addrGetHeadLookTarget = getAbsoluteAddress(targetLibName, 0x2F19FC); // GameCtrl.GetHeadLookTarget(Player me)
        if (addrGetHeadLookTarget == 0) return nullptr;
        auto getHeadLookTarget = reinterpret_cast<GameCtrlGetHeadLookTargetFunc>(addrGetHeadLookTarget);
        void* targetPlayer = getHeadLookTarget(gameCtrl, myPlayer, nullptr);
        if (!IsProbablyValidPtr(targetPlayer) || targetPlayer == myPlayer) return nullptr;
        void* fallbackTransform = GetPreferredAimTransformFromPlayer(targetPlayer);
        if (!IsProbablyValidPtr(fallbackTransform)) return nullptr;
        return CanAttackTargetTransform(myCtrlPlayer, fallbackTransform) ? fallbackTransform : nullptr;
    } catch (...) {
        return nullptr;
    }
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

void* GetMainCameraInstance() {
    try {
        void* gameCtrl = GetGameCtrlInstance();
        if (IsProbablyValidPtr(gameCtrl)) {
            void* cameraCtrl = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x1C); // GameCtrl.cameraCtrl
            if (IsProbablyValidPtr(cameraCtrl)) {
                void* mainCamera = *reinterpret_cast<void**>(reinterpret_cast<char*>(cameraCtrl) + 0x14); // CameraCtrl.MainCamera
                if (IsProbablyValidPtr(mainCamera)) {
                    return mainCamera;
                }
            }
        }

        void* uiManage = GetUIManageInstance();
        if (IsProbablyValidPtr(uiManage)) {
            void* uiCamera = *reinterpret_cast<void**>(reinterpret_cast<char*>(uiManage) + 0xC); // UI_Manage.uiCamera
            if (IsProbablyValidPtr(uiCamera)) {
                return uiCamera;
            }
        }
    } catch (...) {
    }

    return nullptr;
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

bool GetTransformWorldPosition(void* transform, Vector3& outPos) {
    if (!IsProbablyValidPtr(transform)) return false;

    try {
        uintptr_t addrTransformGetPosition = getAbsoluteAddress(targetLibName, 0x6EDF68); // Transform.get_position()
        if (addrTransformGetPosition == 0) return false;

        auto transformGetPosition = reinterpret_cast<TransformGetPositionFunc>(addrTransformGetPosition);
        outPos = transformGetPosition(transform, nullptr);
        return true;
    } catch (...) {
        return false;
    }
}

bool WorldToScreen(const Vector3& worldPos, Vector3& outScreenPos) {
    outScreenPos = {0.0f, 0.0f, 0.0f};

    try {
        void* mainCamera = GetMainCameraInstance();
        if (!IsProbablyValidPtr(mainCamera)) return false;

        uintptr_t addrWorldToScreen = getAbsoluteAddress(targetLibName, 0x8448C0); // Camera.WorldToScreenPoint(Vector3)
        uintptr_t addrScreenWidth = getAbsoluteAddress(targetLibName, 0x6E7564); // Screen.get_width()
        uintptr_t addrScreenHeight = getAbsoluteAddress(targetLibName, 0x6E75E8); // Screen.get_height()
        if (addrWorldToScreen == 0 || addrScreenWidth == 0 || addrScreenHeight == 0) return false;

        auto worldToScreen = reinterpret_cast<CameraWorldToScreenPointFunc>(addrWorldToScreen);
        auto screenGetWidth = reinterpret_cast<ScreenGetWidthFunc>(addrScreenWidth);
        auto screenGetHeight = reinterpret_cast<ScreenGetHeightFunc>(addrScreenHeight);

        const int screenWidth = screenGetWidth();
        const int screenHeight = screenGetHeight();
        if (screenWidth <= 0 || screenHeight <= 0) return false;

        Vector3 unityScreen = worldToScreen(mainCamera, worldPos, nullptr);
        if (unityScreen.z <= 0.01f) return false;

        outScreenPos.x = unityScreen.x;
        outScreenPos.y = static_cast<float>(screenHeight) - unityScreen.y; // converte de origem bottom-left do Unity para top-left
        outScreenPos.z = unityScreen.z;
        return outScreenPos.x >= -200.0f && outScreenPos.x <= (screenWidth + 200.0f) &&
               outScreenPos.y >= -200.0f && outScreenPos.y <= (screenHeight + 200.0f);
    } catch (...) {
        return false;
    }
}

bool ProjectPlayerScreenAnchors(void* player, Vector3& outHeadScreen, Vector3& outRootScreen) {
    outHeadScreen = {0.0f, 0.0f, 0.0f};
    outRootScreen = {0.0f, 0.0f, 0.0f};
    if (!IsProbablyValidPtr(player)) return false;

    try {
        void* head = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x3C); // Player.Head
        void* root = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x24); // Player.Root
        if (!IsProbablyValidPtr(head) || !IsProbablyValidPtr(root)) return false;

        Vector3 headWorld = {0.0f, 0.0f, 0.0f};
        Vector3 rootWorld = {0.0f, 0.0f, 0.0f};
        if (!GetTransformWorldPosition(head, headWorld) || !GetTransformWorldPosition(root, rootWorld)) return false;
        if (!WorldToScreen(headWorld, outHeadScreen) || !WorldToScreen(rootWorld, outRootScreen)) return false;
        return true;
    } catch (...) {
        return false;
    }
}

static bool TryBuildScreenBoxForPlayer(void* player, ScreenBox& outBox) {
    Vector3 headScreen = {0.0f, 0.0f, 0.0f};
    Vector3 rootScreen = {0.0f, 0.0f, 0.0f};
    if (!ProjectPlayerScreenAnchors(player, headScreen, rootScreen)) return false;

    const float height = rootScreen.y - headScreen.y;
    if (height < 8.0f) return false;

    const float width = height * 0.45f;
    outBox.left = headScreen.x - (width * 0.5f);
    outBox.right = headScreen.x + (width * 0.5f);
    outBox.top = headScreen.y;
    outBox.bottom = rootScreen.y;
    return true;
}

static bool EnsureEspJniMethods(JNIEnv* env, jobject espView) {
    if (!env || !espView) return false;
    if (gEspDrawLineMethod && gEspDrawRectMethod && gEspDrawTextMethod &&
        gEspDrawCircleMethod &&
        gEspViewGetWidthMethod && gEspViewGetHeightMethod) {
        return true;
    }

    jclass espClass = env->GetObjectClass(espView);
    if (!espClass) return false;

    gEspDrawLineMethod = env->GetMethodID(espClass, "drawLine", "(Landroid/graphics/Canvas;IIIIFFFFF)V");
    gEspDrawRectMethod = env->GetMethodID(espClass, "drawRect", "(Landroid/graphics/Canvas;IIIIFFFFF)V");
    gEspDrawTextMethod = env->GetMethodID(espClass, "drawText", "(Landroid/graphics/Canvas;IIIILjava/lang/String;FFF)V");
    gEspDrawCircleMethod = env->GetMethodID(espClass, "drawCircle", "(Landroid/graphics/Canvas;IIIIFFFF)V");
    gEspViewGetWidthMethod = env->GetMethodID(espClass, "getWidth", "()I");
    gEspViewGetHeightMethod = env->GetMethodID(espClass, "getHeight", "()I");
    env->DeleteLocalRef(espClass);

    return gEspDrawLineMethod && gEspDrawRectMethod && gEspDrawTextMethod &&
           gEspDrawCircleMethod &&
           gEspViewGetWidthMethod && gEspViewGetHeightMethod;
}

static bool IsHostileEspType(int baseType, int animalType, int gunID) {
    if (baseType == EnemyNPC) return true;
    if (baseType == Zombies || baseType == Ogre) return true;
    if (baseType == MissionPerson && (animalType == Sheriff || animalType == BountyHunter) && gunID > 0) return true;
    if (baseType == Animal) {
        return animalType == Cheetah || animalType == Bear ||
               animalType == Wolf01 || animalType == Wolf02 || animalType == Wolf03;
    }
    return false;
}

static void DrawEspLine(JNIEnv* env, jobject espView, jobject canvas, int a, int r, int g, int b, float stroke, float fromX, float fromY, float toX, float toY) {
    if (!EnsureEspJniMethods(env, espView)) return;
    env->CallVoidMethod(espView, gEspDrawLineMethod, canvas, a, r, g, b, stroke, fromX, fromY, toX, toY);
}

static void DrawEspRect(JNIEnv* env, jobject espView, jobject canvas, int a, int r, int g, int b, float stroke, float x, float y, float width, float height) {
    if (!EnsureEspJniMethods(env, espView)) return;
    env->CallVoidMethod(espView, gEspDrawRectMethod, canvas, a, r, g, b, stroke, x, y, width, height);
}

static void DrawEspText(JNIEnv* env, jobject espView, jobject canvas, int a, int r, int g, int b, const char* text, float x, float y, float size) {
    if (!EnsureEspJniMethods(env, espView) || !text) return;

    jstring jText = env->NewStringUTF(text);
    if (!jText) return;
    env->CallVoidMethod(espView, gEspDrawTextMethod, canvas, a, r, g, b, jText, x, y, size);
    env->DeleteLocalRef(jText);
}

static void DrawEspCircle(JNIEnv* env, jobject espView, jobject canvas, int a, int r, int g, int b, float stroke, float x, float y, float radius) {
    if (!EnsureEspJniMethods(env, espView)) return;
    env->CallVoidMethod(espView, gEspDrawCircleMethod, canvas, a, r, g, b, stroke, x, y, radius);
}

static bool TryGetPlayerBoneScreen(void* player, uintptr_t boneOffset, Vector3& outScreen) {
    outScreen = {0.0f, 0.0f, 0.0f};
    if (!IsProbablyValidPtr(player)) return false;

    try {
        void* bone = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + boneOffset);
        if (!IsProbablyValidPtr(bone)) return false;

        Vector3 world = {0.0f, 0.0f, 0.0f};
        if (!GetTransformWorldPosition(bone, world)) return false;
        return WorldToScreen(world, outScreen);
    } catch (...) {
        return false;
    }
}

static void DrawEspSkeleton(JNIEnv* env, jobject espView, jobject canvas,
                            void* player, int red, int green, int blue, float stroke) {
    if (!IsProbablyValidPtr(player)) return;

    constexpr uintptr_t kBoneRoot = 0x24;
    constexpr uintptr_t kBoneBody = 0x28;
    constexpr uintptr_t kBoneUpHalfBody = 0x30;
    constexpr uintptr_t kBoneNeck = 0x38;
    constexpr uintptr_t kBoneHead = 0x3C;
    constexpr uintptr_t kBoneRightUpperArm = 0x44;
    constexpr uintptr_t kBoneRightHand = 0x48;
    constexpr uintptr_t kBoneLeftUpperArm = 0x50;
    constexpr uintptr_t kBoneLeftHand = 0x54;
    constexpr uintptr_t kBoneLeftLeg = 0x58;
    constexpr uintptr_t kBoneRightLeg = 0x5C;

    Vector3 root = {0.0f, 0.0f, 0.0f};
    Vector3 body = {0.0f, 0.0f, 0.0f};
    Vector3 upHalfBody = {0.0f, 0.0f, 0.0f};
    Vector3 neck = {0.0f, 0.0f, 0.0f};
    Vector3 head = {0.0f, 0.0f, 0.0f};
    Vector3 rUpperArm = {0.0f, 0.0f, 0.0f};
    Vector3 rHand = {0.0f, 0.0f, 0.0f};
    Vector3 lUpperArm = {0.0f, 0.0f, 0.0f};
    Vector3 lHand = {0.0f, 0.0f, 0.0f};
    Vector3 lLeg = {0.0f, 0.0f, 0.0f};
    Vector3 rLeg = {0.0f, 0.0f, 0.0f};

    const bool hasRoot = TryGetPlayerBoneScreen(player, kBoneRoot, root);
    const bool hasBody = TryGetPlayerBoneScreen(player, kBoneBody, body);
    const bool hasUpHalfBody = TryGetPlayerBoneScreen(player, kBoneUpHalfBody, upHalfBody);
    const bool hasNeck = TryGetPlayerBoneScreen(player, kBoneNeck, neck);
    const bool hasHead = TryGetPlayerBoneScreen(player, kBoneHead, head);
    const bool hasRightUpperArm = TryGetPlayerBoneScreen(player, kBoneRightUpperArm, rUpperArm);
    const bool hasRightHand = TryGetPlayerBoneScreen(player, kBoneRightHand, rHand);
    const bool hasLeftUpperArm = TryGetPlayerBoneScreen(player, kBoneLeftUpperArm, lUpperArm);
    const bool hasLeftHand = TryGetPlayerBoneScreen(player, kBoneLeftHand, lHand);
    const bool hasLeftLeg = TryGetPlayerBoneScreen(player, kBoneLeftLeg, lLeg);
    const bool hasRightLeg = TryGetPlayerBoneScreen(player, kBoneRightLeg, rLeg);

    if (hasHead && hasNeck) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, head.x, head.y, neck.x, neck.y);
    }
    if (hasNeck && hasUpHalfBody) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, neck.x, neck.y, upHalfBody.x, upHalfBody.y);
    } else if (hasHead && hasUpHalfBody) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, head.x, head.y, upHalfBody.x, upHalfBody.y);
    }
    if (hasUpHalfBody && hasBody) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, upHalfBody.x, upHalfBody.y, body.x, body.y);
    }
    if (hasBody && hasRoot) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, body.x, body.y, root.x, root.y);
    } else if (hasUpHalfBody && hasRoot) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, upHalfBody.x, upHalfBody.y, root.x, root.y);
    }
    if (hasUpHalfBody && hasRightUpperArm) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, upHalfBody.x, upHalfBody.y, rUpperArm.x, rUpperArm.y);
    }
    if (hasRightUpperArm && hasRightHand) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, rUpperArm.x, rUpperArm.y, rHand.x, rHand.y);
    }
    if (hasUpHalfBody && hasLeftUpperArm) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, upHalfBody.x, upHalfBody.y, lUpperArm.x, lUpperArm.y);
    }
    if (hasLeftUpperArm && hasLeftHand) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, lUpperArm.x, lUpperArm.y, lHand.x, lHand.y);
    }
    if (hasRoot && hasLeftLeg) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, root.x, root.y, lLeg.x, lLeg.y);
    }
    if (hasRoot && hasRightLeg) {
        DrawEspLine(env, espView, canvas, 210, red, green, blue, stroke, root.x, root.y, rLeg.x, rLeg.y);
    }
}

static void ResolveEspTypeColor(int baseType, int animalType, int gunID, int& outRed, int& outGreen, int& outBlue) {
    outRed = espColorRed;
    outGreen = espColorGreen;
    outBlue = espColorBlue;

    if (baseType == Zombies) {
        outRed = 130;
        outGreen = 255;
        outBlue = 110;
        return;
    }

    if (baseType == Ogre) {
        outRed = 255;
        outGreen = 140;
        outBlue = 60;
        return;
    }

    if (baseType == Animal) {
        outRed = 255;
        outGreen = 220;
        outBlue = 90;
        return;
    }

    if (baseType == MissionPerson && (animalType == Sheriff || animalType == BountyHunter) && gunID > 0) {
        outRed = 110;
        outGreen = 170;
        outBlue = 255;
    }
}

static void DrawEspTriangle(JNIEnv* env, jobject espView, jobject canvas,
                            int a, int r, int g, int b, float stroke,
                            float tipX, float tipY, float dirX, float dirY,
                            float length, float width) {
    const float baseCenterX = tipX - (dirX * length);
    const float baseCenterY = tipY - (dirY * length);
    const float perpX = -dirY;
    const float perpY = dirX;
    const float leftX = baseCenterX + (perpX * width);
    const float leftY = baseCenterY + (perpY * width);
    const float rightX = baseCenterX - (perpX * width);
    const float rightY = baseCenterY - (perpY * width);

    DrawEspLine(env, espView, canvas, a, r, g, b, stroke, tipX, tipY, leftX, leftY);
    DrawEspLine(env, espView, canvas, a, r, g, b, stroke, tipX, tipY, rightX, rightY);
    DrawEspLine(env, espView, canvas, a, r, g, b, stroke, leftX, leftY, rightX, rightY);
}

struct OffscreenEspGroup {
    bool active = false;
    float dirX = 0.0f;
    float dirY = 0.0f;
    float nearestDistance = 99999.0f;
    int count = 0;
    int red = 255;
    int green = 90;
    int blue = 90;
    const char* tag = "ENEMY";
};

static int FindMatchingOffscreenGroup(OffscreenEspGroup* groups, int groupCount, float dirX, float dirY) {
    int bestIndex = -1;
    float bestSimilarity = 0.94f;

    for (int i = 0; i < groupCount; ++i) {
        if (!groups[i].active) continue;

        const float similarity = (groups[i].dirX * dirX) + (groups[i].dirY * dirY);
        if (similarity > bestSimilarity) {
            bestSimilarity = similarity;
            bestIndex = i;
        }
    }

    return bestIndex;
}

static void AddOrUpdateOffscreenGroup(OffscreenEspGroup* groups, int& groupCount, int maxGroups,
                                      float dirX, float dirY, float distance,
                                      int red, int green, int blue, const char* tag) {
    if (!groups || maxGroups <= 0 || !tag) return;

    int groupIndex = FindMatchingOffscreenGroup(groups, groupCount, dirX, dirY);
    if (groupIndex < 0) {
        if (groupCount >= maxGroups) {
            int farthestIndex = 0;
            for (int i = 1; i < groupCount; ++i) {
                if (groups[i].nearestDistance > groups[farthestIndex].nearestDistance) {
                    farthestIndex = i;
                }
            }
            groupIndex = farthestIndex;
        } else {
            groupIndex = groupCount++;
        }

        groups[groupIndex].active = true;
        groups[groupIndex].dirX = dirX;
        groups[groupIndex].dirY = dirY;
        groups[groupIndex].nearestDistance = distance;
        groups[groupIndex].count = 1;
        groups[groupIndex].red = red;
        groups[groupIndex].green = green;
        groups[groupIndex].blue = blue;
        groups[groupIndex].tag = tag;
        return;
    }

    OffscreenEspGroup& group = groups[groupIndex];
    group.count += 1;

    const float blendedDirX = (group.dirX * static_cast<float>(group.count - 1)) + dirX;
    const float blendedDirY = (group.dirY * static_cast<float>(group.count - 1)) + dirY;
    const float blendedLength = std::sqrt((blendedDirX * blendedDirX) + (blendedDirY * blendedDirY));
    if (blendedLength > 0.001f) {
        group.dirX = blendedDirX / blendedLength;
        group.dirY = blendedDirY / blendedLength;
    }

    if (distance < group.nearestDistance) {
        group.nearestDistance = distance;
        group.red = red;
        group.green = green;
        group.blue = blue;
        group.tag = tag;
    }
}

static bool ProjectWorldToScreenRaw(const Vector3& worldPos, Vector3& outScreenPos) {
    outScreenPos = {0.0f, 0.0f, 0.0f};

    try {
        void* mainCamera = GetMainCameraInstance();
        if (!IsProbablyValidPtr(mainCamera)) return false;

        uintptr_t addrWorldToScreen = getAbsoluteAddress(targetLibName, 0x8448C0); // Camera.WorldToScreenPoint(Vector3)
        uintptr_t addrScreenWidth = getAbsoluteAddress(targetLibName, 0x6E7564); // Screen.get_width()
        uintptr_t addrScreenHeight = getAbsoluteAddress(targetLibName, 0x6E75E8); // Screen.get_height()
        if (addrWorldToScreen == 0 || addrScreenWidth == 0 || addrScreenHeight == 0) return false;

        auto worldToScreen = reinterpret_cast<CameraWorldToScreenPointFunc>(addrWorldToScreen);
        auto screenGetWidth = reinterpret_cast<ScreenGetWidthFunc>(addrScreenWidth);
        auto screenGetHeight = reinterpret_cast<ScreenGetHeightFunc>(addrScreenHeight);

        const int rawScreenWidth = screenGetWidth();
        const int rawScreenHeight = screenGetHeight();
        if (rawScreenWidth <= 0 || rawScreenHeight <= 0) return false;

        Vector3 unityScreen = worldToScreen(mainCamera, worldPos, nullptr);
        outScreenPos.x = unityScreen.x;
        outScreenPos.y = static_cast<float>(rawScreenHeight) - unityScreen.y;
        outScreenPos.z = unityScreen.z;
        return true;
    } catch (...) {
        return false;
    }
}

void DrawOn(JNIEnv* env, jobject espView, jobject canvas) {
    if (!env || !espView || !canvas) return;
    if (!isLibraryLoaded(targetLibName) || !IsDialogLoginValidated()) return;
    if (!espCanvas) return;
    if (!EnsureEspJniMethods(env, espView)) return;

    const bool wantsOnScreenBox = espShowBox || espShowSkeleton || espShowSnapline || espShowLabel;
    const bool wantsOffscreen360 = espShowOffscreen360;
    if (!wantsOnScreenBox && !wantsOffscreen360) return;

    void* myPlayer = GetMyPlayerInstance();
    if (!IsProbablyValidPtr(myPlayer)) return;

    int screenWidth = env->CallIntMethod(espView, gEspViewGetWidthMethod);
    int screenHeight = env->CallIntMethod(espView, gEspViewGetHeightMethod);
    if (screenWidth <= 0 || screenHeight <= 0) return;

    const float screenCenterX = static_cast<float>(screenWidth) * 0.5f;
    const float screenCenterY = static_cast<float>(screenHeight) * 0.5f;
    const float edgeMargin = 42.0f;
    const float offscreenRadius = 14.0f;
    OffscreenEspGroup offscreenGroups[16] = {};
    int offscreenGroupCount = 0;

    Vector3 myPos = {0.0f, 0.0f, 0.0f};
    if (!GetPlayerWorldPosition(myPlayer, myPos)) return;

    void* enemies[128] = {};
    int enemyCount = CollectActiveEnemyBases(enemies, 128);
    for (int i = 0; i < enemyCount; ++i) {
        void* enemyBase = enemies[i];
        if (!IsProbablyValidPtr(enemyBase)) continue;

        void* player = nullptr;
        int baseType = -1;
        int animalType = -1;
        int gunID = 0;
        Vector3 enemyPos = {0.0f, 0.0f, 0.0f};
        if (!ResolveCombatData(enemyBase, &player, baseType, animalType, gunID, enemyPos)) continue;
        if (!IsProbablyValidPtr(player) || player == myPlayer) continue;
        if (!IsHostileEspType(baseType, animalType, gunID)) continue;

        int red = espColorRed;
        int green = espColorGreen;
        int blue = espColorBlue;
        const char* tag = "ENEMY";

        if (baseType == Zombies) {
            tag = "ZOMBIE";
        } else if (baseType == Ogre) {
            tag = "OGRE";
        } else if (baseType == Animal) {
            tag = "ANIMAL";
        } else if (baseType == MissionPerson) {
            tag = "LAW";
        }

        ResolveEspTypeColor(baseType, animalType, gunID, red, green, blue);

        float distance = 0.0f;
        if (espShowLabel || espShowOffscreen360) {
            const float dx = enemyPos.x - myPos.x;
            const float dy = enemyPos.y - myPos.y;
            const float dz = enemyPos.z - myPos.z;
            distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
        }

        char label[64] = {0};
        if (espShowLabel) {
            std::snprintf(label, sizeof(label), "%s %.0fm", tag, distance);
        }

        bool drewOnScreenEsp = false;
        bool targetIsOnScreen = false;
        ScreenBox box{};
        float boxWidth = 0.0f;
        float boxHeight = 0.0f;
        float centerX = 0.0f;
        float targetY = 0.0f;
        Vector3 rawScreen = {0.0f, 0.0f, 0.0f};
        bool hasRawScreen = false;

        if (wantsOnScreenBox) {
            if (TryBuildScreenBoxForPlayer(player, box)) {
                boxWidth = box.right - box.left;
                boxHeight = box.bottom - box.top;
                centerX = box.left + (boxWidth * 0.5f);
                targetY = box.top + (boxHeight * 0.5f);

                targetIsOnScreen =
                        box.right >= 0.0f &&
                        box.left <= static_cast<float>(screenWidth) &&
                        box.bottom >= 0.0f &&
                        box.top <= static_cast<float>(screenHeight);

                if (targetIsOnScreen) {
                    if (espShowBox) {
                        DrawEspRect(env, espView, canvas, 220, red, green, blue, espBoxThickness, box.left, box.top, boxWidth, boxHeight);
                    }
                    if (espShowSkeleton) {
                        DrawEspSkeleton(env, espView, canvas, player, red, green, blue, std::max(1.2f, espLineThickness));
                    }
                    drewOnScreenEsp = true;
                }
            }
        } else if (wantsOffscreen360) {
            if (!ProjectWorldToScreenRaw(enemyPos, rawScreen)) continue;
            hasRawScreen = true;
            targetIsOnScreen =
                    rawScreen.z > 0.01f &&
                    rawScreen.x >= 0.0f &&
                    rawScreen.x <= static_cast<float>(screenWidth) &&
                    rawScreen.y >= 0.0f &&
                    rawScreen.y <= static_cast<float>(screenHeight);
        }

        if (espShowSnapline && drewOnScreenEsp) {
            float startX = (static_cast<float>(screenWidth) * 0.5f) + espSnaplineOffsetX;
            float startY = espSnaplineOffsetY;

            if (espSnaplineOriginMode == 1) {
                startY = (static_cast<float>(screenHeight) * 0.5f) + espSnaplineOffsetY;
            } else if (espSnaplineOriginMode == 2) {
                startY = static_cast<float>(screenHeight) - espSnaplineOffsetY;
            }

            DrawEspLine(env, espView, canvas, 150, red, green, blue, espLineThickness,
                        startX,
                        startY,
                        centerX,
                        targetY);
        }
        if (espShowLabel && drewOnScreenEsp) {
            DrawEspText(env, espView, canvas, 235, 255, 255, 255, label, centerX, box.top - 10.0f, espTextSize);
        }

        if (!targetIsOnScreen && espShowOffscreen360) {
            if (!hasRawScreen && !ProjectWorldToScreenRaw(enemyPos, rawScreen)) {
                continue;
            }

            float dirX = rawScreen.x - screenCenterX;
            float dirY = rawScreen.y - screenCenterY;

            if (rawScreen.z <= 0.01f) {
                dirX = -dirX;
                dirY = -dirY;
            }

            const float dirLength = std::sqrt((dirX * dirX) + (dirY * dirY));
            if (dirLength <= 0.001f) continue;

            dirX /= dirLength;
            dirY /= dirLength;

            const float usableHalfWidth = screenCenterX - edgeMargin;
            const float usableHalfHeight = screenCenterY - edgeMargin;
            if (usableHalfWidth <= 0.0f || usableHalfHeight <= 0.0f) continue;

            AddOrUpdateOffscreenGroup(offscreenGroups, offscreenGroupCount,
                                      static_cast<int>(sizeof(offscreenGroups) / sizeof(offscreenGroups[0])),
                                      dirX, dirY, distance, red, green, blue, tag);
        }
    }

    for (int groupIndex = 0; groupIndex < offscreenGroupCount; ++groupIndex) {
        OffscreenEspGroup& group = offscreenGroups[groupIndex];
        if (!group.active) continue;

        const float scaleX = (screenCenterX - edgeMargin) / std::max(std::fabs(group.dirX), 0.001f);
        const float scaleY = (screenCenterY - edgeMargin) / std::max(std::fabs(group.dirY), 0.001f);
        const float edgeScale = std::min(scaleX, scaleY);
        const float indicatorX = screenCenterX + (group.dirX * edgeScale);
        const float indicatorY = screenCenterY + (group.dirY * edgeScale);
        const float dangerFactor = std::max(0.0f, std::min(1.0f, (40.0f - group.nearestDistance) / 40.0f));
        const float triangleLength = 15.0f + (dangerFactor * 10.0f);
        const float triangleWidth = 9.0f + (dangerFactor * 4.0f);
        const float ringRadius = offscreenRadius + (dangerFactor * 5.0f);
        const int indicatorAlpha = 195 + static_cast<int>(dangerFactor * 50.0f);

        DrawEspTriangle(env, espView, canvas, indicatorAlpha, group.red, group.green, group.blue,
                        std::max(2.0f, espLineThickness), indicatorX, indicatorY,
                        group.dirX, group.dirY, triangleLength, triangleWidth);
        DrawEspCircle(env, espView, canvas, 165, group.red, group.green, group.blue,
                      std::max(1.5f, espLineThickness - 0.2f), indicatorX, indicatorY, ringRadius);

        if (espShowOffscreenCount && group.count > 1) {
            char countText[16] = {0};
            std::snprintf(countText, sizeof(countText), "x%d", group.count);
            DrawEspText(env, espView, canvas, 245, group.red, group.green, group.blue, countText,
                        indicatorX, indicatorY + ringRadius + 16.0f, std::max(11.0f, espTextSize - 3.0f));
        }

        if (espShowOffscreenLabel) {
            char groupLabel[64] = {0};
            std::snprintf(groupLabel, sizeof(groupLabel), "%s %.0fm", group.tag, group.nearestDistance);
            DrawEspText(env, espView, canvas, 235, 255, 255, 255, groupLabel,
                        indicatorX, indicatorY - 24.0f, std::max(10.0f, espTextSize - 2.0f));
        }
    }

    if (env->ExceptionCheck()) {
        env->ExceptionClear();
    }
}

static float Vector3LengthXZ(const Vector3& value) {
    return std::sqrt((value.x * value.x) + (value.z * value.z));
}

static Vector3 NormalizeXZ(const Vector3& value) {
    const float len = Vector3LengthXZ(value);
    if (len <= 0.0001f) return {0.0f, 0.0f, 0.0f};
    return {value.x / len, 0.0f, value.z / len};
}

static float ClampMagnitudeXZ(float value, float minValue, float maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

static bool HasTrailCompatiblePlayerAnchors(void* player) {
    if (!IsProbablyValidPtr(player)) return false;

    try {
        void* gunRightHand = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x74); // Player.GunRightHand
        void* gunMiddleFront = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x80); // Player.GunMiddleFront
        void* head = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x3C); // Player.Head
        return IsProbablyValidPtr(gunRightHand) || IsProbablyValidPtr(gunMiddleFront) || IsProbablyValidPtr(head);
    } catch (...) {
        return false;
    }
}

static bool IsNpcWarEligibleEnemyBase(void* enemyBase, void** outPlayer, void** outPlayerBase, Vector3* outPos) {
    if (outPlayer) *outPlayer = nullptr;
    if (outPlayerBase) *outPlayerBase = nullptr;
    if (outPos) *outPos = {0.0f, 0.0f, 0.0f};

    void* player = nullptr;
    int baseType = -1;
    int animalType = -1;
    int gunID = 0;
    Vector3 pos = {0.0f, 0.0f, 0.0f};
    if (!ResolveCombatData(enemyBase, &player, baseType, animalType, gunID, pos)) return false;
    if (!IsProbablyValidPtr(player)) return false;

    const bool hostileHuman =
            (baseType == EnemyNPC || (baseType == MissionPerson && (animalType == Sheriff || animalType == BountyHunter))) &&
            gunID > 0 &&
            HasTrailCompatiblePlayerAnchors(player) &&
            (animalType == Robber || animalType == Pro01 || animalType == Pro02 ||
             animalType == Bid01 || animalType == Bid02 ||
             animalType == Sheriff || animalType == BountyHunter);
    const bool hostileMonster = (baseType == Zombies || baseType == Ogre);
    const bool hostileAnimal =
            (baseType == Animal) &&
            (animalType == Cheetah || animalType == Bear || animalType == Wolf01 ||
             animalType == Wolf02 || animalType == Wolf03);

    if (!(hostileHuman || hostileMonster || hostileAnimal)) return false;

    void* playerBase = GetPlayerBaseFromPlayer(player);
    if (!IsProbablyValidPtr(playerBase)) return false;

    if (outPlayer) *outPlayer = player;
    if (outPlayerBase) *outPlayerBase = playerBase;
    if (outPos) *outPos = pos;
    return true;
}

static bool IsPoliceWarEligiblePlayer(void* player, void** outPlayerBase, Vector3* outPos) {
    if (outPlayerBase) *outPlayerBase = nullptr;
    if (outPos) *outPos = {0.0f, 0.0f, 0.0f};
    if (!IsProbablyValidPtr(player) || player == GetMyPlayerInstance()) return false;

    void* playerBase = GetPlayerBaseFromPlayer(player);
    if (!IsProbablyValidPtr(playerBase)) return false;

    try {
        void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0x14); // PlayerBase.m_dPlayerBaseData
        if (!IsProbablyValidPtr(baseData)) return false;

        void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8); // PlayerBaseData.m_dProperty
        void* aiData = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0xC); // PlayerBaseData.m_dAIdata
        if (!IsProbablyValidPtr(property) || !IsProbablyValidPtr(aiData)) return false;

        int baseType = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0xC); // PlayerBaseProperty.baseType
        int animalType = *reinterpret_cast<int*>(reinterpret_cast<char*>(aiData) + 0x8); // AIdata.animalType
        int gunID = *reinterpret_cast<int*>(reinterpret_cast<char*>(aiData) + 0x10); // AIdata.gunID
        if (baseType != MissionPerson) return false;
        if (!(animalType == Sheriff || animalType == BountyHunter)) return false;
        if (gunID <= 0) return false;
        if (!HasTrailCompatiblePlayerAnchors(player)) return false;

        Vector3 pos = {0.0f, 0.0f, 0.0f};
        if (!GetPlayerWorldPosition(player, pos)) return false;

        if (outPlayerBase) *outPlayerBase = playerBase;
        if (outPos) *outPos = pos;
        return true;
    } catch (...) {
        return false;
    }
}

static bool IsFlyableNpcPlayer(void* player) {
    int baseType = -1;
    int animalType = -1;
    int gunID = 0;
    if (!ResolvePlayerMetaFromPlayer(player, baseType, animalType, gunID, nullptr)) return false;
    if (!IsProbablyValidPtr(player) || player == GetMyPlayerInstance()) return false;
    if (baseType == Cowboy || baseType == Horse || animalType == AnimalCowboy) return false;
    return true;
}

static void ClearTrackedNpcFlightPlayers() {
    npcFlightPlayerCount = 0;
    for (int i = 0; i < kMaxNpcFlightPlayers; ++i) {
        npcFlightPlayers[i] = nullptr;
        npcFlightBaseY[i] = 0.0f;
        npcFlightTargetY[i] = 0.0f;
    }
}

static bool IsTrackedNpcFlightPlayer(void* player) {
    if (!IsProbablyValidPtr(player)) return false;
    for (int i = 0; i < npcFlightPlayerCount; ++i) {
        if (npcFlightPlayers[i] == player) return true;
    }
    return false;
}

static void TrackNpcFlightPlayer(void* player) {
    if (!IsProbablyValidPtr(player) || IsTrackedNpcFlightPlayer(player) || npcFlightPlayerCount >= kMaxNpcFlightPlayers) return;
    npcFlightPlayers[npcFlightPlayerCount++] = player;
}

static void RestoreTrackedNpcFlightPlayers() {
    for (int i = 0; i < npcFlightPlayerCount; ++i) {
        RestoreSingleEnemyFlightState(npcFlightPlayers[i]);
    }
    ClearTrackedNpcFlightPlayers();
}

static void RestoreSingleEnemyFlightState(void* enemyPlayer) {
    if (!IsProbablyValidPtr(enemyPlayer)) return;

    uintptr_t addrSetNavMeshEnable = getAbsoluteAddress(targetLibName, 0x34897C); // Player.SetNavMesEnable(bool)
    uintptr_t addrSetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35C8F8); // Player.SetCurrentVelocity(Vector3)
    uintptr_t addrSetNavMeshSpeed = getAbsoluteAddress(targetLibName, 0x35C038); // Player.SetNavMesSpeed(float)
    uintptr_t addrEnableMoveCollider = getAbsoluteAddress(targetLibName, 0x3603BC); // Player.EnableMoveCollider()
    if (addrSetNavMeshEnable == 0 || addrSetCurrentVelocity == 0) return;

    auto setNavMeshEnable = reinterpret_cast<PlayerSetNavMeshEnableFunc>(addrSetNavMeshEnable);
    auto setCurrentVelocity = reinterpret_cast<PlayerSetCurrentVelocityFunc>(addrSetCurrentVelocity);
    auto setNavMeshSpeed = reinterpret_cast<PlayerSetNavMeshSpeedFunc>(addrSetNavMeshSpeed);
    auto enableMoveCollider = reinterpret_cast<PlayerEnableMoveColliderFunc>(addrEnableMoveCollider);

    Vector3 zeroVel = {0.0f, 0.0f, 0.0f};
    setNavMeshEnable(enemyPlayer, true, nullptr);
    if (addrSetNavMeshSpeed != 0) {
        setNavMeshSpeed(enemyPlayer, 4.0f, nullptr);
    }
    if (addrEnableMoveCollider != 0) {
        enableMoveCollider(enemyPlayer, nullptr);
    }
    setCurrentVelocity(enemyPlayer, zeroVel, nullptr);
}

void LogWorldToScreenSamples() {
    try {
        void* enemies[32] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 32);
        if (enemyCount <= 0) {
            __android_log_print(ANDROID_LOG_INFO, "MOD_W2S", "Nenhum inimigo ativo para projetar na tela");
            return;
        }

        int logged = 0;
        for (int i = 0; i < enemyCount && logged < 3; ++i) {
            void* enemyBase = enemies[i];
            if (!IsProbablyValidPtr(enemyBase)) continue;

            void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0xC); // PlayerBase.m_dPlayer
            if (!IsProbablyValidPtr(player) || player == GetMyPlayerInstance()) continue;

            Vector3 headScreen = {0.0f, 0.0f, 0.0f};
            Vector3 rootScreen = {0.0f, 0.0f, 0.0f};
            ScreenBox box = {};
            const bool projected = ProjectPlayerScreenAnchors(player, headScreen, rootScreen);
            const bool boxReady = projected && TryBuildScreenBoxForPlayer(player, box);

            __android_log_print(
                    ANDROID_LOG_INFO,
                    "MOD_W2S",
                    "Enemy[%d] projected=%d head=(%.1f, %.1f, %.2f) root=(%.1f, %.1f, %.2f) box=%d [L%.1f T%.1f R%.1f B%.1f]",
                    i,
                    projected ? 1 : 0,
                    headScreen.x, headScreen.y, headScreen.z,
                    rootScreen.x, rootScreen.y, rootScreen.z,
                    boxReady ? 1 : 0,
                    box.left, box.top, box.right, box.bottom);
            logged++;
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_W2S", "Falha protegida ao registrar amostras WorldToScreen");
    }
}

void RefreshTargetMarkerESP() {
    void* creator = GetPlayingUICreatorInstance();
    void* myCtrlPlayer = GetMyCtrlPlayerInstance();
    if (!IsProbablyValidPtr(creator) || !IsProbablyValidPtr(myCtrlPlayer)) return;

    try {
        uintptr_t addrShow = getAbsoluteAddress(targetLibName, 0x3C9864); // UI_PlayingUI_Creator.ShowAimFollowTagetUI()
        uintptr_t addrHide = getAbsoluteAddress(targetLibName, 0x3C994C); // UI_PlayingUI_Creator.HideAimFollowTagetUI()
        uintptr_t addrSetTarget = getAbsoluteAddress(targetLibName, 0x3C9A34); // UI_PlayingUI_Creator.SetAimFollowTarget(Transform)
        if (addrShow == 0 || addrHide == 0 || addrSetTarget == 0) return;

        auto showMarker = reinterpret_cast<ShowAimFollowTargetUIFunc>(addrShow);
        auto hideMarker = reinterpret_cast<HideAimFollowTargetUIFunc>(addrHide);
        auto setMarkerTarget = reinterpret_cast<SetAimFollowTargetUIFunc>(addrSetTarget);

        void* target = ResolveBestAggressiveAimTarget(myCtrlPlayer);
        if (!IsProbablyValidPtr(target)) {
            target = FindBestTarget(myCtrlPlayer);
        }

        if (IsProbablyValidPtr(target)) {
            showMarker(creator, nullptr);
            setMarkerTarget(creator, target, nullptr);
        } else {
            hideMarker(creator, nullptr);
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Falha protegida ao atualizar marcador ESP");
    }
}

void SetFlyRuntimeState(bool enabled) {
    void* ctrlPlayer = GetCtrlPlayerFromMyCtrl();
    if (!ctrlPlayer) return;

    uintptr_t addrSetNavMeshEnable = getAbsoluteAddress(targetLibName, 0x34897C); // Player.SetNavMesEnable(bool)
    uintptr_t addrSetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35C8F8); // Player.SetCurrentVelocity(Vector3)
    uintptr_t addrSetNavMeshSpeed = getAbsoluteAddress(targetLibName, 0x35C038); // Player.SetNavMesSpeed(float)
    if (addrSetNavMeshEnable == 0 || addrSetCurrentVelocity == 0) return;

    auto setNavMeshEnable = reinterpret_cast<PlayerSetNavMeshEnableFunc>(addrSetNavMeshEnable);
    auto setCurrentVelocity = reinterpret_cast<PlayerSetCurrentVelocityFunc>(addrSetCurrentVelocity);
    auto setNavMeshSpeed = reinterpret_cast<PlayerSetNavMeshSpeedFunc>(addrSetNavMeshSpeed);

    // Em voo, desativa NavMesh para não prender fora da malha; ao sair, reativa.
    setNavMeshEnable(ctrlPlayer, !enabled, nullptr);
    if (addrSetNavMeshSpeed != 0) {
        setNavMeshSpeed(ctrlPlayer, enabled ? 0.0f : 4.0f, nullptr);
    }

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

void ApplyFlyMovementFrame(void* myCtrlPlayer) {
    if (!flyMode || !IsProbablyValidPtr(myCtrlPlayer)) return;

    void* ctrlPlayer = GetCtrlPlayerFromMyCtrl();
    if (!IsProbablyValidPtr(ctrlPlayer)) return;

    uintptr_t addrTranslateWithSamplePos = getAbsoluteAddress(targetLibName, 0x35CB9C); // Player.TranslateWithSamplePos(Vector3)
    uintptr_t addrGetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35CD58); // Player.GetCurrentVelocity()
    uintptr_t addrIsMoving = getAbsoluteAddress(targetLibName, 0x35C648); // Player.IsMoving()
    uintptr_t addrHeading = getAbsoluteAddress(targetLibName, 0x35C0F4); // Player.Heading()
    uintptr_t addrRotateToCameraDir = getAbsoluteAddress(targetLibName, 0x35C3B8); // Player.RotateToCameraDir()
    uintptr_t addrSetRotationCameraDir = getAbsoluteAddress(targetLibName, 0x35C2C4); // Player.SetRotationCameraDir()
    uintptr_t addrSetCurrentRotation = getAbsoluteAddress(targetLibName, 0x35C994); // Player.SetCurrentRotation(Vector3)
    uintptr_t addrStopRotate = getAbsoluteAddress(targetLibName, 0x35C720); // Player.StopRotate()
    uintptr_t addrSetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35C8F8); // Player.SetCurrentVelocity(Vector3)
    uintptr_t addrPlayerGetPosition = getAbsoluteAddress(targetLibName, 0x341378); // Player.GetPosition()
    uintptr_t addrSetPosMyCtrl = getAbsoluteAddress(targetLibName, 0x45D69C); // MyCtrlPlayer.SetPosition(Vector3)
    if (addrTranslateWithSamplePos == 0 || addrGetCurrentVelocity == 0 || addrIsMoving == 0 ||
        addrHeading == 0 || addrRotateToCameraDir == 0 || addrSetCurrentVelocity == 0 ||
        addrPlayerGetPosition == 0 || addrSetPosMyCtrl == 0) {
        return;
    }

    auto translateWithSamplePos = reinterpret_cast<PlayerTranslateWithSamplePosFunc>(addrTranslateWithSamplePos);
    auto getCurrentVelocity = reinterpret_cast<PlayerGetCurrentVelocityFunc>(addrGetCurrentVelocity);
    auto isMoving = reinterpret_cast<PlayerIsMovingFunc>(addrIsMoving);
    auto getHeading = reinterpret_cast<PlayerHeadingFunc>(addrHeading);
    auto rotateToCameraDir = reinterpret_cast<PlayerRotateToCameraDirFunc>(addrRotateToCameraDir);
    auto setRotationCameraDir = reinterpret_cast<PlayerSetRotationCameraDirFunc>(addrSetRotationCameraDir);
    auto setCurrentRotation = reinterpret_cast<PlayerSetCurrentRotationFunc>(addrSetCurrentRotation);
    auto stopRotate = reinterpret_cast<PlayerStopRotateFunc>(addrStopRotate);
    auto setCurrentVelocity = reinterpret_cast<PlayerSetCurrentVelocityFunc>(addrSetCurrentVelocity);
    auto getPlayerPosition = reinterpret_cast<PlayerGetPositionFunc>(addrPlayerGetPosition);
    auto setPosMyCtrl = reinterpret_cast<MyCtrlPlayerSetPositionFunc>(addrSetPosMyCtrl);

    try {
        static Vector3 lastStableHeading = {0.0f, 0.0f, 1.0f};

        if (addrSetRotationCameraDir != 0) {
            setRotationCameraDir(ctrlPlayer, nullptr);
        } else {
            rotateToCameraDir(ctrlPlayer, nullptr);
        }

        Vector3 currentVelocity = getCurrentVelocity(ctrlPlayer, nullptr);
        Vector3 heading = getHeading(ctrlPlayer, nullptr);
        bool moving = isMoving(ctrlPlayer, nullptr);

        Vector3 normalizedHeading = NormalizeXZ(heading);
        if (Vector3LengthXZ(normalizedHeading) <= 0.001f) {
            normalizedHeading = NormalizeXZ(currentVelocity);
        }
        if (Vector3LengthXZ(normalizedHeading) <= 0.001f) {
            normalizedHeading = lastStableHeading;
        } else {
            lastStableHeading = normalizedHeading;
        }

        const float inputSpeed = 1.2f + (flyVerticalSpeed * 0.65f);
        const float horizontalSpeed = ClampMagnitudeXZ(inputSpeed, 1.0f, 12.0f);
        const float hoverLift = 0.02f + (flyHeightStep * 0.035f);
        const float movingLift = moving ? ((flyHeightStep * 0.07f) + (flyVerticalSpeed * 0.01f)) : 0.0f;
        const float damping = moving ? 0.18f : 0.10f;

        Vector3 desiredVelocity = {0.0f, hoverLift + movingLift, 0.0f};
        if (Vector3LengthXZ(normalizedHeading) > 0.001f && (moving || Vector3LengthXZ(currentVelocity) > 0.05f)) {
            desiredVelocity.x = normalizedHeading.x * horizontalSpeed;
            desiredVelocity.z = normalizedHeading.z * horizontalSpeed;
        } else {
            desiredVelocity.x = currentVelocity.x * damping;
            desiredVelocity.z = currentVelocity.z * damping;
        }

        if (addrSetCurrentRotation != 0) {
            setCurrentRotation(ctrlPlayer, desiredVelocity, nullptr);
        }
        if (!moving && addrStopRotate != 0) {
            stopRotate(ctrlPlayer, nullptr);
        }

        setCurrentVelocity(ctrlPlayer, desiredVelocity, nullptr);
        translateWithSamplePos(ctrlPlayer, desiredVelocity, nullptr);

        Vector3 syncedPos = getPlayerPosition(ctrlPlayer, nullptr);
        setPosMyCtrl(myCtrlPlayer, syncedPos, nullptr);
    } catch (...) {
    }
}

void SetEnemyFlightState(bool enabled) {
    if (!enabled) {
        RestoreTrackedNpcFlightPlayers();
        return;
    }

    ClearTrackedNpcFlightPlayers();
}

void ApplyEnemyFlyMovementFrame() {
    if (!npcFlyMode) return;

    uintptr_t addrSetNavMeshEnable = getAbsoluteAddress(targetLibName, 0x34897C); // Player.SetNavMesEnable(bool)
    uintptr_t addrSetNavMeshSpeed = getAbsoluteAddress(targetLibName, 0x35C038); // Player.SetNavMesSpeed(float)
    uintptr_t addrGetPosition = getAbsoluteAddress(targetLibName, 0x341378); // Player.GetPosition()
    uintptr_t addrSetPosition = getAbsoluteAddress(targetLibName, 0x348A38); // Player.SetPosition(Vector3)
    uintptr_t addrGetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35CD58); // Player.GetCurrentVelocity()
    uintptr_t addrHeading = getAbsoluteAddress(targetLibName, 0x35C0F4); // Player.Heading()
    uintptr_t addrSetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35C8F8); // Player.SetCurrentVelocity(Vector3)
    uintptr_t addrTranslateWithSamplePos = getAbsoluteAddress(targetLibName, 0x35CB9C); // Player.TranslateWithSamplePos(Vector3)
    uintptr_t addrRotateToDir = getAbsoluteAddress(targetLibName, 0x344E34); // Player.RotateToDir(Vector3)
    uintptr_t addrDisableMoveCollider = getAbsoluteAddress(targetLibName, 0x360304); // Player.DisableMoveCollider()
    if (addrSetNavMeshEnable == 0 || addrGetPosition == 0 || addrSetPosition == 0 ||
        addrGetCurrentVelocity == 0 || addrSetCurrentVelocity == 0 || addrTranslateWithSamplePos == 0) {
        return;
    }

    auto setNavMeshEnable = reinterpret_cast<PlayerSetNavMeshEnableFunc>(addrSetNavMeshEnable);
    auto setNavMeshSpeed = reinterpret_cast<PlayerSetNavMeshSpeedFunc>(addrSetNavMeshSpeed);
    auto getPosition = reinterpret_cast<PlayerGetPositionFunc>(addrGetPosition);
    auto setPosition = reinterpret_cast<PlayerSetPositionFunc>(addrSetPosition);
    auto getCurrentVelocity = reinterpret_cast<PlayerGetCurrentVelocityFunc>(addrGetCurrentVelocity);
    auto getHeading = reinterpret_cast<PlayerHeadingFunc>(addrHeading);
    auto setCurrentVelocity = reinterpret_cast<PlayerSetCurrentVelocityFunc>(addrSetCurrentVelocity);
    auto translateWithSamplePos = reinterpret_cast<PlayerTranslateWithSamplePosFunc>(addrTranslateWithSamplePos);
    auto rotateToDir = reinterpret_cast<PlayerRotateToDirFunc>(addrRotateToDir);
    auto disableMoveCollider = reinterpret_cast<PlayerDisableMoveColliderFunc>(addrDisableMoveCollider);

    try {
        static int npcHoverTick = 0;
        npcHoverTick++;

        void* livingPlayers[kMaxNpcFlightPlayers] = {};
        int livingCount = CollectLivingPlayers(livingPlayers, kMaxNpcFlightPlayers);
        if (livingCount <= 0) {
            RestoreTrackedNpcFlightPlayers();
            return;
        }

        void* framePlayers[kMaxNpcFlightPlayers] = {};
        int framePlayerCount = 0;
        void* previousPlayers[kMaxNpcFlightPlayers] = {};
        float previousBaseY[kMaxNpcFlightPlayers] = {};
        float previousTargetY[kMaxNpcFlightPlayers] = {};
        int previousCount = npcFlightPlayerCount;
        for (int i = 0; i < previousCount; ++i) {
            previousPlayers[i] = npcFlightPlayers[i];
            previousBaseY[i] = npcFlightBaseY[i];
            previousTargetY[i] = npcFlightTargetY[i];
        }

        for (int i = 0; i < livingCount && framePlayerCount < kMaxNpcFlightPlayers; ++i) {
            void* enemyPlayer = livingPlayers[i];
            if (!IsFlyableNpcPlayer(enemyPlayer)) continue;
            framePlayers[framePlayerCount++] = enemyPlayer;
        }

        for (int i = 0; i < previousCount; ++i) {
            void* trackedPlayer = previousPlayers[i];
            bool stillTracked = false;
            for (int j = 0; j < framePlayerCount; ++j) {
                if (framePlayers[j] == trackedPlayer) {
                    stillTracked = true;
                    break;
                }
            }
            if (!stillTracked) {
                RestoreSingleEnemyFlightState(trackedPlayer);
            }
        }

        ClearTrackedNpcFlightPlayers();

        for (int i = 0; i < framePlayerCount; ++i) {
            void* enemyPlayer = framePlayers[i];
            if (!IsProbablyValidPtr(enemyPlayer)) continue;

            TrackNpcFlightPlayer(enemyPlayer);

            setNavMeshEnable(enemyPlayer, false, nullptr);
            if (addrSetNavMeshSpeed != 0) {
                setNavMeshSpeed(enemyPlayer, 0.0f, nullptr);
            }
            if (addrDisableMoveCollider != 0) {
                disableMoveCollider(enemyPlayer, nullptr);
            }

            Vector3 currentPos = getPosition(enemyPlayer, nullptr);
            Vector3 currentVelocity = getCurrentVelocity(enemyPlayer, nullptr);
            int previousIndex = -1;
            for (int j = 0; j < previousCount; ++j) {
                if (previousPlayers[j] == enemyPlayer) {
                    previousIndex = j;
                    break;
                }
            }

            float baseY = (previousIndex >= 0) ? previousBaseY[previousIndex] : currentPos.y;
            float targetY = baseY + npcFlyHoverHeight;
            if (npcFlyHoverWave) {
                float phase = (npcHoverTick * 0.08f) + (i * 0.65f);
                targetY += sinf(phase) * npcFlyHoverWaveAmplitude;
            }

            Vector3 heading = addrHeading != 0 ? getHeading(enemyPlayer, nullptr) : Vector3{0.0f, 0.0f, 0.0f};
            Vector3 drift = NormalizeXZ(heading);
            if (Vector3LengthXZ(drift) <= 0.001f) {
                drift = NormalizeXZ(currentVelocity);
            }

            const float hoverResponse = 0.04f + (npcFlyHoverResponsiveness * 0.08f);
            const float maxVerticalStep = 0.04f + (npcFlyLift * 0.05f) + (flyHeightStep * 0.01f);
            float verticalStep = (targetY - currentPos.y) * hoverResponse;
            if (verticalStep > maxVerticalStep) verticalStep = maxVerticalStep;
            if (verticalStep < -maxVerticalStep) verticalStep = -maxVerticalStep;
            if (fabsf(targetY - currentPos.y) < 0.015f) verticalStep = 0.0f;

            float driftSpeed = npcFlyDriftScale * (0.08f + (Vector3LengthXZ(currentVelocity) * 0.10f));
            driftSpeed = ClampMagnitudeXZ(driftSpeed, 0.02f, 0.75f);

            Vector3 desiredVelocity = {0.0f, verticalStep, 0.0f};
            Vector3 nextPos = currentPos;
            nextPos.y += verticalStep;

            if (Vector3LengthXZ(drift) > 0.001f) {
                desiredVelocity.x = drift.x * driftSpeed;
                desiredVelocity.z = drift.z * driftSpeed;
                nextPos.x += desiredVelocity.x;
                nextPos.z += desiredVelocity.z;
                if (addrRotateToDir != 0) {
                    rotateToDir(enemyPlayer, drift, nullptr);
                }
            } else {
                desiredVelocity.x = currentVelocity.x * 0.04f;
                desiredVelocity.z = currentVelocity.z * 0.04f;
                nextPos.x += desiredVelocity.x;
                nextPos.z += desiredVelocity.z;
            }

            setCurrentVelocity(enemyPlayer, desiredVelocity, nullptr);
            Vector3 translateVelocity = {desiredVelocity.x, 0.0f, desiredVelocity.z};
            translateWithSamplePos(enemyPlayer, translateVelocity, nullptr);
            setPosition(enemyPlayer, nextPos, nullptr);

            int trackedIndex = npcFlightPlayerCount - 1;
            if (trackedIndex >= 0 && trackedIndex < kMaxNpcFlightPlayers) {
                npcFlightBaseY[trackedIndex] = baseY;
                npcFlightTargetY[trackedIndex] = targetY;
            }
        }
    } catch (...) {
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

// ========== IMPLEMENTAÇÃO DO SISTEMA POLICIAL ==========

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
 * Força o cálculo como headshot quando a feature estiver ativa.
 */
int hook_GetHitBlood(void *thisPtr, int part, int type, Vector3 enemy, Vector3 myPosition, int modelType) {
    // Verificação de segurança para evitar crashes
    if (!thisPtr) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD", "Erro: thisPtr é nulo em GetHitBlood");
        return 1; // Retorna dano mínimo
    }

    constexpr int kColliderBodyPartHead = 0; // dump.cs: ColliderBodyParts.Head = 0
    int resolvedPart = alwaysHeadshot ? kColliderBodyPartHead : part;

    if (alwaysHeadshot && part != kColliderBodyPartHead) {
        __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Redirecionando hit para Head (part=%d -> %d)", part,
                            resolvedPart);
    }

    int result = original_GetHitBlood(thisPtr, resolvedPart, type, enemy, myPosition, modelType);

    if (sliderValue > 1) {
        result = alwaysHeadshot ? (sliderValue * 2) : sliderValue;
    }

    __android_log_print(ANDROID_LOG_DEBUG, "MOD", "Dano final: %d", result);
    return result;
}

// Ponteiros para funções de mira
void (*original_UpdateAimTarget)(void* thisPtr);
void (*original_SetAimState)(void* thisPtr, AimTargetState state, void* target, bool forceTarget);
void (*original_MyCtrlPlayerMyUpdate)(void* thisPtr);

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
 * Hook para a função UpdateAimTarget
 * SISTEMA AIMBOT V5: Busca agressiva com offsets reais do dump.cs
 */
void hook_UpdateAimTarget(void* thisPtr) {
    if (!thisPtr) return;
    
    // Chama a função original primeiro
    original_UpdateAimTarget(thisPtr);

    if (!aimBotAggressive) return;

    // Primeiro tenta o alvo já resolvido pelo jogo.
    void* bestTarget = FindBestTarget(thisPtr);
    if (IsProbablyValidPtr(bestTarget) && !CanAttackTargetTransform(thisPtr, bestTarget)) {
        bestTarget = nullptr;
    }

    static int aggressiveFrameCounter = 0;
    aggressiveFrameCounter++;

    if (!bestTarget || (aggressiveFrameCounter % 2) == 0) {
        ForceAimRefresh(thisPtr);
        original_UpdateAimTarget(thisPtr);
        original_UpdateAimTarget(thisPtr);
        bestTarget = FindBestTarget(thisPtr);
        if (IsProbablyValidPtr(bestTarget) && !CanAttackTargetTransform(thisPtr, bestTarget)) {
            bestTarget = nullptr;
        }
    }

    if (!bestTarget) {
        bestTarget = ResolveBestAggressiveAimTarget(thisPtr);
    }

    if (!bestTarget) return;
    if (!CanAttackTargetTransform(thisPtr, bestTarget)) return;

    CallSetAimState(thisPtr, Aiming_Focus, bestTarget, true);

    // Mantém estado interno consistente para reduzir flicker.
    try {
        AimTargetState* aimStatePtr = reinterpret_cast<AimTargetState*>((char*)thisPtr + 0x20);
        *aimStatePtr = Aiming_Focus;

        void** aimTargetPtr = reinterpret_cast<void**>((char*)thisPtr + 0x24);
        if (aimTargetPtr) *aimTargetPtr = bestTarget;
        void** aimTargetCamPtr = reinterpret_cast<void**>((char*)thisPtr + 0x28);
        if (aimTargetCamPtr) *aimTargetCamPtr = bestTarget;
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
    
    // Quando o aimbot agressivo está ativo, mantém foco se houver alvo válido.
    if (aimBotAggressive && target && state == Aiming_NotFocus) {
        finalState = Aiming_Focus;
        forceTarget = true;
        __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIM", "AimBot agressivo: Convertendo NotFocus para Focus");
    }
    
    __android_log_print(ANDROID_LOG_DEBUG, "MOD_AIM", 
                        "SetAimState: Estado=%d, Alvo=%p, Forçar=%d", 
                        finalState, target, forceTarget);
    
    // Chama a função original com possíveis modificações
    original_SetAimState(thisPtr, finalState, target, forceTarget);
}

void ProcessGameplayFrame(void* myCtrlPlayer) {
    if (!myCtrlPlayer) return;
    static bool authLockActive = false;

    if (!IsDialogLoginValidated()) {
        if (!authLockActive) {
            Health = false;
            aimBotAggressive = false;
            alwaysHeadshot = false;
            killNearbyOnWeaponSwap = false;
            flyMode = false;
            npcFlyMode = false;
            npcWarMode = false;
            espCanvas = false;
            completeEsp = false;
            autoKill = false;
            bulletTailEsp = false;
            minimapEnemyEsp = false;
            autoClearPolice = false;

            pendingCreateMissionHints = false;
            pendingDestroyMissionHints = false;
            pendingEspRefresh = false;
            pendingEspClear = false;
            pendingAutoKillBurst = false;
            pendingBulletTailShot = false;
            pendingBulletTailClear = false;
            pendingMiniMapEspRefresh = false;
            pendingMiniMapEspClear = false;
            pendingShowWordsTest = false;
            pendingShowWordsCustom = false;
            pendingChaosTime = false;
            pendingChaosSpawn = false;
            pendingChaosUI = false;
            pendingChaosWeapon = false;
            pendingChaosFx = false;
            pendingKillPoliceOnly = false;
            pendingTeleportRequest = TeleportNone;

            pendingCustomWordsText[0] = '\0';
            SetFlyRuntimeState(false);
            SetEnemyFlightState(false);
            HideTargetMarker();
            ClearCompleteESP();
            ClearBulletTailNow();
            ClearMiniMapEnemyEsp();

            __android_log_print(ANDROID_LOG_WARN, "MOD_DIALOG",
                                "Auth lock ativo: todas as funcoes do mod foram bloqueadas");
            authLockActive = true;
        }
        return;
    }

    authLockActive = false;

    if (pendingBulletTailClear) {
        ClearBulletTailNow();
        pendingBulletTailClear = false;
    }

    if (pendingBulletTailShot) {
        TriggerBulletTailNow();
        pendingBulletTailShot = false;
    }

    if (pendingMiniMapEspClear) {
        ClearMiniMapEnemyEsp();
        pendingMiniMapEspClear = false;
    }

    if (pendingMiniMapEspRefresh) {
        RefreshMiniMapEnemyEsp();
        pendingMiniMapEspRefresh = false;
    }

    if (pendingShowWordsTest) {
        ShowWordsHintText("Teste de texto custom via UI_WordsHints", 3.5f);
        pendingShowWordsTest = false;
    }

    if (pendingShowWordsCustom) {
        ShowWordsHintText(pendingCustomWordsText, 4.5f);
        pendingShowWordsCustom = false;
    }

    if (pendingChaosTime) {
        RunChaosTimeOnce();
        pendingChaosTime = false;
    }

    if (pendingChaosSpawn) {
        RunChaosSpawnOnce();
        pendingChaosSpawn = false;
    }

    if (pendingChaosUI) {
        RunChaosUIOnce();
        pendingChaosUI = false;
    }

    if (pendingChaosWeapon) {
        RunChaosWeaponOnce();
        pendingChaosWeapon = false;
    }

    if (pendingChaosFx) {
        RunChaosFxOnce();
        pendingChaosFx = false;
    }

    if (pendingKillPoliceOnly) {
        RunKillPoliceOnlyOnce();
        pendingKillPoliceOnly = false;
    }

    if (pendingTeleportRequest != TeleportNone) {
        const int requestMode = pendingTeleportRequest;
        pendingTeleportRequest = TeleportNone;
        RunTeleportToRequest(requestMode);
    }

    if (flyMode) {
        ApplyFlyMovementFrame(myCtrlPlayer);
    }
    if (npcFlyMode) {
        ApplyEnemyFlyMovementFrame();
    }
    if (npcWarMode) {
        RunNpcWarFrame();
    }
    if (autoClearPolice) {
        static int autoClearPoliceFrameCounter = 0;
        autoClearPoliceFrameCounter++;
        if ((autoClearPoliceFrameCounter % 45) == 0) {
            RunKillPoliceOnlyOnce();
        }
    }

    ProcessGameplayHints();

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
        if ((espFrameCounter % 12) == 0) {
            RefreshTargetMarkerESP();
        }
        if ((espFrameCounter % 45) == 0) {
            RefreshCompleteESP();
        }
    }

    if (aimBotAggressive && IsProbablyValidPtr(GetPlayingUICreatorInstance())) {
        static int markerFrameCounter = 0;
        markerFrameCounter++;
        if ((markerFrameCounter % 10) == 0) {
            RefreshTargetMarkerESP();
        }
    }

    if (bulletTailEsp && CanUseBulletTail()) {
        GenerateBulletTailForAllActiveEnemies();
    }

    if (minimapEnemyEsp && CanUseMiniMapEnemyEsp()) {
        static int miniMapEspFrameCounter = 0;
        miniMapEspFrameCounter++;
        if ((miniMapEspFrameCounter % 30) == 0) {
            RefreshMiniMapEnemyEsp();
        }
    }

    if (killNearbyOnWeaponSwap) {
        UpdateWeaponSwapKillMonitor();
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

    int loginHintTick = 0;
    while (!IsDialogLoginValidated()) {
        ++loginHintTick;
        if (isLibraryLoaded(targetLibName) && (loginHintTick % 15) == 0) {
            ShowWordsHintText("FACA LOGIN PARA LIBERAR O MENU", 2.4f);
        }
        usleep(200000);
    }

    __android_log_print(ANDROID_LOG_INFO, "MOD_DIALOG", "Login validado; liberando instalacao dos hooks");

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

    ShowQueuedLibLoadDialog(env, context);

    const char *features[] = {
            OBFUSCATE("Category_Menu de Modificações"),
            OBFUSCATE("Collapse_Player e Recursos"),
            OBFUSCATE("CollapseAdd_Category_Player e Recursos"),
            OBFUSCATE("CollapseAdd_4_Toggle_Vida Infinita (15/02/2026)"),
            OBFUSCATE("CollapseAdd_1_SeekBar_Dano de bala (15/02/2026)_1_999"),
            OBFUSCATE("CollapseAdd_2_InputValue_Adicionar Moedas (15/02/2026)"),
            OBFUSCATE("CollapseAdd_3_InputValue_Adicionar Gems (15/02/2026)"),
            OBFUSCATE("CollapseAdd_5_SeekBar_Balas das Armas (15/02/2026)_1_999999"),

            OBFUSCATE("Collapse_Itens"),
            OBFUSCATE("CollapseAdd_Category_Itens"),
            OBFUSCATE("CollapseAdd_12_Button_Adicionar Todas as Partes de Armas (15/02/2026)"),
            OBFUSCATE("CollapseAdd_13_Button_Adicionar Todas as Peles (15/02/2026)"),
            OBFUSCATE("CollapseAdd_14_Button_Adicionar 10 Whisky (15/02/2026)"),

            OBFUSCATE("Collapse_Combate"),
            OBFUSCATE("CollapseAdd_Category_Combate"),
            OBFUSCATE("CollapseAdd_35_Toggle_AimBot Agressivo (15/02/2026)"),
            OBFUSCATE("CollapseAdd_22_Toggle_Sempre Headshot (15/02/2026)"),
            OBFUSCATE("CollapseAdd_43_Toggle_Auto Kill Seguro (08/03/2026)"),
            OBFUSCATE("CollapseAdd_44_Button_Kill All Agora (08/03/2026)"),
            OBFUSCATE("CollapseAdd_86_Toggle_Matar Proximos ao Trocar de Arma"),
            OBFUSCATE("CollapseAdd_87_SeekBar_Raio do Kill na Troca_1_30"),

            OBFUSCATE("Collapse_ESP Canvas"),
            OBFUSCATE("CollapseAdd_Category_ESP Canvas"),
            OBFUSCATE("CollapseAdd_75_Toggle_ESP Canvas (Overlay Java)"),
            OBFUSCATE("CollapseAdd_64_SeekBar_ESP Vermelho_0_255"),
            OBFUSCATE("CollapseAdd_65_SeekBar_ESP Verde_0_255"),
            OBFUSCATE("CollapseAdd_66_SeekBar_ESP Azul_0_255"),
            OBFUSCATE("CollapseAdd_67_SeekBar_ESP Espessura Box_1_8"),
            OBFUSCATE("CollapseAdd_68_SeekBar_ESP Espessura Linha_1_8"),
            OBFUSCATE("CollapseAdd_69_SeekBar_ESP Tamanho Texto_8_32"),
            OBFUSCATE("CollapseAdd_80_Toggle_ESP Box"),
            OBFUSCATE("CollapseAdd_70_Toggle_ESP Snapline"),
            OBFUSCATE("CollapseAdd_71_Toggle_ESP Label"),
            OBFUSCATE("CollapseAdd_79_Toggle_ESP Skeleton"),
            OBFUSCATE("CollapseAdd_76_Toggle_ESP 360 Off-screen"),
            OBFUSCATE("CollapseAdd_77_Toggle_Label do ESP 360"),
            OBFUSCATE("CollapseAdd_78_Toggle_Contador do ESP 360"),
            OBFUSCATE("CollapseAdd_72_Spinner_ESP Origem Linha_Topo,Centro,Base"),
            OBFUSCATE("CollapseAdd_73_SeekBar_ESP Offset X (Centro=200)_0_400"),
            OBFUSCATE("CollapseAdd_74_SeekBar_ESP Offset Y_0_400"),

            OBFUSCATE("Collapse_ESP Nativa"),
            OBFUSCATE("CollapseAdd_Category_ESP Nativa"),
            OBFUSCATE("CollapseAdd_40_Toggle_ESP Completo (Barras de Vida) (08/03/2026)"),
            OBFUSCATE("CollapseAdd_41_Button_Atualizar ESP Agora (08/03/2026)"),
            OBFUSCATE("CollapseAdd_47_Toggle_ESP Inimigos no Minimapa (08/03/2026)"),

            OBFUSCATE("Collapse_Visual e UI"),
            OBFUSCATE("CollapseAdd_Category_Visual e UI"),
            OBFUSCATE("CollapseAdd_36_Button_Criar Mission Hints Visuais (08/03/2026)"),
            OBFUSCATE("CollapseAdd_37_Button_Remover Mission Hints Visuais (08/03/2026)"),
            OBFUSCATE("CollapseAdd_38_Button_Mostrar Marcador no Alvo Atual (08/03/2026)"),
            OBFUSCATE("CollapseAdd_39_Button_Ocultar Marcador do Alvo (08/03/2026)"),
            OBFUSCATE("CollapseAdd_45_Toggle_Trilhas de Tiro em Todos os Alvos (08/03/2026)"),
            OBFUSCATE("CollapseAdd_46_Button_Limpar Trilhas de Tiro (08/03/2026)"),
            OBFUSCATE("CollapseAdd_48_Button_Mostrar Texto de Teste (08/03/2026)"),
            OBFUSCATE("CollapseAdd_49_InputText_Mostrar Texto Custom (08/03/2026)"),

            OBFUSCATE("Collapse_Movimento"),
            OBFUSCATE("CollapseAdd_Category_Movimento"),
            OBFUSCATE("CollapseAdd_32_Toggle_Modo Voo (Experimental) (15/02/2026)"),
            OBFUSCATE("CollapseAdd_33_SeekBar_Velocidade Vertical_1_20"),
            OBFUSCATE("CollapseAdd_34_SeekBar_Ganho de Altura_1_50"),
            OBFUSCATE("CollapseAdd_50_Toggle_NPCs Voadores (08/03/2026)"),
            OBFUSCATE("CollapseAdd_51_SeekBar_Forca do Hover NPC_1_30"),
            OBFUSCATE("CollapseAdd_81_SeekBar_Altura do Hover NPC_1_40"),
            OBFUSCATE("CollapseAdd_82_SeekBar_Drift Horizontal NPC_1_30"),
            OBFUSCATE("CollapseAdd_83_SeekBar_Suavidade do Hover NPC_1_20"),
            OBFUSCATE("CollapseAdd_84_Toggle_Oscilacao do Hover NPC"),
            OBFUSCATE("CollapseAdd_85_SeekBar_Amplitude da Oscilacao NPC_1_20"),

            OBFUSCATE("Collapse_Policia"),
            OBFUSCATE("CollapseAdd_Category_Policia"),
            OBFUSCATE("CollapseAdd_58_Button_Matar Somente Policiais (08/03/2026)"),
            OBFUSCATE("CollapseAdd_59_Toggle_Auto Limpar Policiais (08/03/2026)"),

            OBFUSCATE("Collapse_Teleport"),
            OBFUSCATE("CollapseAdd_Category_Teleport"),
            OBFUSCATE("CollapseAdd_60_Button_Teleportar Para Alvo Atual (10/03/2026)"),
            OBFUSCATE("CollapseAdd_61_Button_Teleportar Para Hostil Mais Proximo (10/03/2026)"),
            OBFUSCATE("CollapseAdd_62_Button_Teleportar Para NPC de Missao (10/03/2026)"),
            OBFUSCATE("CollapseAdd_63_Button_Teleportar Para NPC do Mapa (10/03/2026)"),

            OBFUSCATE("Collapse_Chaos"),
            OBFUSCATE("CollapseAdd_Category_Chaos"),
            OBFUSCATE("CollapseAdd_52_Button_Chaos Time (08/03/2026)"),
            OBFUSCATE("CollapseAdd_53_Button_Chaos Spawn (08/03/2026)"),
            OBFUSCATE("CollapseAdd_54_Button_Chaos UI (08/03/2026)"),
            OBFUSCATE("CollapseAdd_55_Button_Chaos Weapon (08/03/2026)"),
            OBFUSCATE("CollapseAdd_56_Toggle_NPC War Entre Hostis (08/03/2026)"),
            OBFUSCATE("CollapseAdd_57_Button_Chaos FX (08/03/2026)"),
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
    if (!IsDialogLoginValidated()) {
        RegisterDialogContext(env, obj);
        ShowQueuedLibLoadDialog(env, obj);
        __android_log_print(ANDROID_LOG_WARN, "MOD_DIALOG", "Mudanca ignorada antes do login: %d", featNum);
        return;
    }

    LOGD(OBFUSCATE("Recurso: %d - %s | Valor: = %d | Bool: = %d | Texto: = %s"), featNum,
         env->GetStringUTFChars(featName, 0), value,
         boolean, str != NULL ? env->GetStringUTFChars(str, 0) : "");

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
        case 58: // Matar somente policiais
            pendingKillPoliceOnly = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_POLICE", "Kill Police agendado");
            break;
        case 59: // Auto limpar policiais
            autoClearPolice = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_POLICE",
                                boolean ? "Auto limpar policiais ativado" : "Auto limpar policiais desativado");
            break;
        case 60: // Teleportar para alvo atual
            pendingTeleportRequest = TeleportCurrentTarget;
            __android_log_print(ANDROID_LOG_INFO, "MOD_TELEPORT", "Teleport para alvo atual agendado");
            break;
        case 61: // Teleportar para hostil mais proximo
            pendingTeleportRequest = TeleportNearestHostile;
            __android_log_print(ANDROID_LOG_INFO, "MOD_TELEPORT", "Teleport para hostil mais proximo agendado");
            break;
        case 62: // Teleportar para NPC de missao
            pendingTeleportRequest = TeleportNearestMissionNpc;
            __android_log_print(ANDROID_LOG_INFO, "MOD_TELEPORT", "Teleport para NPC de missao agendado");
            break;
        case 63: // Teleportar para NPC do mapa
            pendingTeleportRequest = TeleportNearestMapNpc;
            __android_log_print(ANDROID_LOG_INFO, "MOD_TELEPORT", "Teleport para NPC do mapa agendado");
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
        case 50: // NPCs voadores
            npcFlyMode = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "NPCs voadores ativados");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "NPCs voadores desativados");
            }
            SetEnemyFlightState(npcFlyMode);
            break;
        case 51: // Forca do hover NPC
            if (value >= 1 && value <= 30) {
                npcFlyLift = (float)value / 10.0f;
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Forca do hover NPC ajustada para: %.2f", npcFlyLift);
            }
            break;
        case 81: // Altura do hover NPC
            if (value >= 1 && value <= 40) {
                npcFlyHoverHeight = (float)value / 10.0f;
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Altura do hover NPC ajustada para: %.2f", npcFlyHoverHeight);
            }
            break;
        case 82: // Drift horizontal NPC
            if (value >= 1 && value <= 30) {
                npcFlyDriftScale = (float)value / 10.0f;
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Drift horizontal NPC ajustado para: %.2f", npcFlyDriftScale);
            }
            break;
        case 83: // Suavidade do hover NPC
            if (value >= 1 && value <= 20) {
                npcFlyHoverResponsiveness = (float)value / 10.0f;
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Suavidade do hover NPC ajustada para: %.2f", npcFlyHoverResponsiveness);
            }
            break;
        case 84: // Oscilacao do hover NPC
            npcFlyHoverWave = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Oscilacao do hover NPC %s", npcFlyHoverWave ? "ativada" : "desativada");
            break;
        case 85: // Amplitude da oscilacao NPC
            if (value >= 1 && value <= 20) {
                npcFlyHoverWaveAmplitude = (float)value / 20.0f;
                __android_log_print(ANDROID_LOG_INFO, "MOD_FLY", "Amplitude da oscilacao NPC ajustada para: %.2f", npcFlyHoverWaveAmplitude);
            }
            break;
        case 52: // Chaos Time
            pendingChaosTime = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos Time agendado");
            break;
        case 53: // Chaos Spawn
            pendingChaosSpawn = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos Spawn agendado");
            break;
        case 54: // Chaos UI
            pendingChaosUI = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos UI agendado");
            break;
        case 55: // Chaos Weapon
            pendingChaosWeapon = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos Weapon agendado");
            break;
        case 56: // NPC War entre hostis
            npcWarMode = boolean;
            if (boolean) {
                __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "NPC War entre hostis ativado");
            } else {
                __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "NPC War entre hostis desativado");
            }
            break;
        case 57: // Chaos FX
            pendingChaosFx = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos FX agendado");
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
        case 86: // Matar proximos ao trocar de arma
            killNearbyOnWeaponSwap = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_AUTOKILL", "Kill por troca de arma %s", killNearbyOnWeaponSwap ? "ativado" : "desativado");
            break;
        case 87: // Raio do kill na troca
            if (value >= 1 && value <= 30) {
                weaponSwapKillRadius = (float)value;
                __android_log_print(ANDROID_LOG_INFO, "MOD_AUTOKILL", "Raio do kill na troca ajustado para: %.1f", weaponSwapKillRadius);
            }
            break;
        case 45: // Trilhas de Tiro em Todos os Alvos
            if (boolean) {
                if (CanUseBulletTail()) {
                    bulletTailEsp = true;
                    pendingBulletTailShot = true;
                    __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Trilhas de tiro por frame ativadas");
                } else {
                    bulletTailEsp = false;
                    __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "Trilhas de tiro bloqueadas: sem NPCs armados compativeis, factory ou jogador invalido");
                }
            } else {
                bulletTailEsp = false;
                pendingBulletTailClear = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Trilhas de tiro por frame desativadas");
            }
            break;
        case 46: // Limpar Trilhas de Tiro
            if (IsProbablyValidPtr(GetBulletTailFactoryInstance())) {
                pendingBulletTailClear = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Limpeza de BulletTail agendada");
            } else {
                __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "Limpeza de BulletTail bloqueada: factory indisponivel");
            }
            break;
        case 47: // ESP Inimigos no Minimapa
            if (boolean) {
                if (CanUseMiniMapEnemyEsp()) {
                    minimapEnemyEsp = true;
                    pendingMiniMapEspRefresh = true;
                    __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "ESP de inimigos no minimapa ativado");
                } else {
                    minimapEnemyEsp = false;
                    __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "ESP de inimigos no minimapa bloqueado: UI ou inimigos nao estao prontos");
                }
            } else {
                minimapEnemyEsp = false;
                pendingMiniMapEspClear = true;
                __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "ESP de inimigos no minimapa desativado");
            }
            break;
        case 48: // Mostrar Texto de Teste
            pendingShowWordsTest = true;
            __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Texto de teste agendado para UI_WordsHints");
            break;
        case 49: // Mostrar Texto Custom
            if (str != NULL) {
                const char* inputText = env->GetStringUTFChars(str, 0);
                if (inputText && inputText[0] != '\0') {
                    std::strncpy(pendingCustomWordsText, inputText, sizeof(pendingCustomWordsText) - 1);
                    pendingCustomWordsText[sizeof(pendingCustomWordsText) - 1] = '\0';
                    pendingShowWordsCustom = true;
                    __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Texto custom agendado: %s", pendingCustomWordsText);
                } else {
                    __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "Texto custom ignorado: string vazia");
                }
                if (inputText) {
                    env->ReleaseStringUTFChars(str, inputText);
                }
            } else {
                __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "Texto custom ignorado: jstring nula");
            }
            break;
        case 75: // ESP Canvas
            espCanvas = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "ESP canvas ativada" : "ESP canvas desativada");
            break;
        case 64: // ESP Vermelho
            if (value >= 0 && value <= 255) {
                espColorRed = value;
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP vermelho ajustado para: %d", espColorRed);
            }
            break;
        case 65: // ESP Verde
            if (value >= 0 && value <= 255) {
                espColorGreen = value;
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP verde ajustado para: %d", espColorGreen);
            }
            break;
        case 66: // ESP Azul
            if (value >= 0 && value <= 255) {
                espColorBlue = value;
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP azul ajustado para: %d", espColorBlue);
            }
            break;
        case 67: // ESP Espessura Box
            if (value >= 1 && value <= 8) {
                espBoxThickness = static_cast<float>(value);
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP box thickness ajustada para: %.1f", espBoxThickness);
            }
            break;
        case 68: // ESP Espessura Linha
            if (value >= 1 && value <= 8) {
                espLineThickness = static_cast<float>(value);
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP line thickness ajustada para: %.1f", espLineThickness);
            }
            break;
        case 69: // ESP Tamanho Texto
            if (value >= 8 && value <= 32) {
                espTextSize = static_cast<float>(value);
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP text size ajustado para: %.1f", espTextSize);
            }
            break;
        case 80: // ESP Box
            espShowBox = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "ESP box ativada" : "ESP box desativada");
            break;
        case 70: // ESP Snapline
            espShowSnapline = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "ESP snapline ativada" : "ESP snapline desativada");
            break;
        case 71: // ESP Label
            espShowLabel = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "ESP label ativada" : "ESP label desativada");
            break;
        case 79: // ESP Skeleton
            espShowSkeleton = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "ESP skeleton ativada" : "ESP skeleton desativada");
            break;
        case 76: // ESP 360 Off-screen
            espShowOffscreen360 = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "ESP 360 off-screen ativada" : "ESP 360 off-screen desativada");
            break;
        case 77: // Label do ESP 360
            espShowOffscreenLabel = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "Label do ESP 360 ativada" : "Label do ESP 360 desativada");
            break;
        case 78: // Contador do ESP 360
            espShowOffscreenCount = boolean;
            __android_log_print(ANDROID_LOG_INFO, "MOD_ESP",
                                boolean ? "Contador do ESP 360 ativado" : "Contador do ESP 360 desativado");
            break;
        case 72: // ESP Origem Linha
            if (value >= 0 && value <= 2) {
                espSnaplineOriginMode = value;
                const char* originText = "topo";
                if (value == 1) {
                    originText = "centro";
                } else if (value == 2) {
                    originText = "base";
                }
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP origem da linha ajustada para: %s", originText);
            }
            break;
        case 73: // ESP Offset X
            if (value >= 0 && value <= 400) {
                espSnaplineOffsetX = static_cast<float>(value - 200);
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP offset X ajustado para: %.1f", espSnaplineOffsetX);
            }
            break;
        case 74: // ESP Offset Y
            if (value >= 0 && value <= 400) {
                espSnaplineOffsetY = static_cast<float>(value);
                __android_log_print(ANDROID_LOG_INFO, "MOD_ESP", "ESP offset Y ajustado para: %.1f", espSnaplineOffsetY);
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

extern "C"
jstring SubmitNativeLogin(JNIEnv* env, jclass, jobject context, jstring email, jstring password) {
    if (!env || !context) {
        return env ? env->NewStringUTF("Contexto invalido para login.") : nullptr;
    }

    const char* emailChars = email ? env->GetStringUTFChars(email, nullptr) : nullptr;
    const char* passwordChars = password ? env->GetStringUTFChars(password, nullptr) : nullptr;

    const char* result = SubmitJavaLogin(
        env,
        context,
        emailChars ? emailChars : "",
        passwordChars ? passwordChars : ""
    );

    if (emailChars) {
        env->ReleaseStringUTFChars(email, emailChars);
    }
    if (passwordChars) {
        env->ReleaseStringUTFChars(password, passwordChars);
    }

    if (!result || result[0] == '\0') {
        return env->NewStringUTF("");
    }

    return env->NewStringUTF(result);
}

extern "C"
jstring GetNativeLoginSummary(JNIEnv* env, jclass) {
    if (!env) {
        return nullptr;
    }

    const char* summary = GetJavaLoginSuccessSummary();
    return env->NewStringUTF((summary && summary[0]) ? summary : "{}");
}

int RegisterMainActivity(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("SubmitNativeLogin"),
             OBFUSCATE("(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;"),
             reinterpret_cast<void *>(SubmitNativeLogin)},
            {OBFUSCATE("GetNativeLoginSummary"),
             OBFUSCATE("()Ljava/lang/String;"),
             reinterpret_cast<void *>(GetNativeLoginSummary)},
    };

    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/MainActivity"));
    if (!clazz)
        return JNI_ERR;
    if (env->RegisterNatives(clazz, methods, sizeof(methods) / sizeof(methods[0])) != 0)
        return JNI_ERR;

    return JNI_OK;
}

int RegisterESPView(JNIEnv *env) {
    JNINativeMethod methods[] = {
            {OBFUSCATE("DrawOn"), OBFUSCATE("(Landroid/graphics/Canvas;)V"),
             reinterpret_cast<void *>(DrawOn)},
    };
    jclass clazz = env->FindClass(OBFUSCATE("vdev/com/android/support/ESPView"));
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
    if (RegisterMainActivity(env) != 0)
        return JNI_ERR;
    if (RegisterESPView(env) != 0)
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

static int CollectLivingPlayers(void** outPlayers, int maxPlayers) {
    if (!outPlayers || maxPlayers <= 0) return 0;

    for (int i = 0; i < maxPlayers; ++i) {
        outPlayers[i] = nullptr;
    }

    int count = 0;

    try {
        void* trackedPlayers[256] = {};
        int trackedCount = CollectTrackedPlayers(trackedPlayers, 256);
        for (int i = 0; i < trackedCount; ++i) {
            AppendUniquePlayer(outPlayers, count, maxPlayers, trackedPlayers[i]);
        }

        void* enemies[256] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 256);
        for (int i = 0; i < enemyCount; ++i) {
            if (!IsProbablyValidPtr(enemies[i])) continue;
            void* enemyPlayer = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemies[i]) + 0xC); // PlayerBase.m_dPlayer
            AppendUniquePlayer(outPlayers, count, maxPlayers, enemyPlayer);
        }

        void* myPlayer = GetMyPlayerInstance();
        for (int i = 0; i < count; ++i) {
            void* player = outPlayers[i];
            if (!IsProbablyValidPtr(player) || player == myPlayer) {
                continue;
            }

            int baseType = -1;
            int animalType = -1;
            int gunID = 0;
            if (!ResolvePlayerMetaFromPlayer(player, baseType, animalType, gunID, nullptr)) {
                outPlayers[i] = nullptr;
            }
        }

        int compacted = 0;
        for (int i = 0; i < count; ++i) {
            if (!IsProbablyValidPtr(outPlayers[i])) continue;
            outPlayers[compacted++] = outPlayers[i];
        }
        return compacted;
    } catch (...) {
        return count;
    }
}

static bool IsTeleportHostileType(int baseType, int animalType, int gunID) {
    switch (baseType) {
        case EnemyNPC:
        case Zombies:
        case Ogre:
            return true;
        case Animal:
            return animalType == Cheetah || animalType == Bear || animalType == Wolf01 ||
                   animalType == Wolf02 || animalType == Wolf03;
        case MissionPerson:
            return animalType == Sheriff || animalType == BountyHunter || gunID > 0;
        default:
            return false;
    }
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

void RunWeaponSwapKillNearbyOnce(float radiusMeters) {
    if (!CanRunAutoKill()) return;

    try {
        void* myPlayer = GetMyPlayerInstance();
        if (!IsProbablyValidPtr(myPlayer)) return;

        Vector3 myPos = {0.0f, 0.0f, 0.0f};
        if (!GetPlayerWorldPosition(myPlayer, myPos)) return;

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
        void* enermyGC = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x24);
        void* enemyFactory = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x28);
        void* missionCtrl = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x2C);
        void* missionEntity = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x30);

        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        if (enemyCount <= 0) return;

        const float radiusSq = radiusMeters * radiusMeters;
        int touched = 0;
        for (int i = 0; i < enemyCount; ++i) {
            void* enemyBase = enemies[i];
            if (!IsProbablyValidPtr(enemyBase)) continue;

            void* player = nullptr;
            int baseType = -1;
            int animalType = -1;
            int gunID = 0;
            Vector3 enemyPos = {0.0f, 0.0f, 0.0f};
            if (!ResolveCombatData(enemyBase, &player, baseType, animalType, gunID, enemyPos)) continue;

            float dx = enemyPos.x - myPos.x;
            float dy = enemyPos.y - myPos.y;
            float dz = enemyPos.z - myPos.z;
            float distanceSq = dx * dx + dy * dy + dz * dz;
            if (distanceSq > radiusSq) continue;

            void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0x14);
            if (!IsProbablyValidPtr(baseData)) continue;
            void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8);
            if (!IsProbablyValidPtr(property)) continue;

            int currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14);
            int maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10);
            if (currentBlood <= 0 || maxBlood <= 0) continue;

            int lethalDamage = currentBlood + maxBlood + 5000;
            beHit(enemyBase, lethalDamage, 0, nullptr);
            touched++;

            if (IsProbablyValidPtr(missionCtrl) && IsProbablyValidPtr(player)) {
                if (addrKilledAI != 0) killedAI(missionCtrl, player, nullptr);
                if (addrClearEnemy != 0) clearEnemy(missionCtrl, player, nullptr);
            }

            if (IsProbablyValidPtr(missionEntity) && addrMissionContainEnemy != 0 && addrMissionDeleteEnemy != 0) {
                if (missionContainEnemy(missionEntity, enemyBase, nullptr)) {
                    missionDeleteEnemy(missionEntity, enemyBase, nullptr);
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

        if (touched > 0) {
            __android_log_print(ANDROID_LOG_INFO, "MOD_AUTOKILL", "Kill por troca de arma tocou %d inimigos em %.1fm", touched, radiusMeters);
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_AUTOKILL", "Falha protegida em RunWeaponSwapKillNearbyOnce");
    }
}

void UpdateWeaponSwapKillMonitor() {
    static bool initialized = false;
    static int lastGunType = -1;

    void* myPlayer = GetMyPlayerInstance();
    if (!IsProbablyValidPtr(myPlayer)) {
        initialized = false;
        lastGunType = -1;
        return;
    }

    uintptr_t addrGetCurrentGunType = getAbsoluteAddress(targetLibName, 0x3455A4); // Player.GetCurrentGunType()
    if (addrGetCurrentGunType == 0) return;

    auto getCurrentGunType = reinterpret_cast<PlayerGetCurrentGunTypeFunc>(addrGetCurrentGunType);
    int currentGunType = getCurrentGunType(myPlayer, nullptr);

    if (!initialized) {
        initialized = true;
        lastGunType = currentGunType;
        return;
    }

    if (currentGunType != lastGunType) {
        lastGunType = currentGunType;
        if (killNearbyOnWeaponSwap && CanRunAutoKill()) {
            RunWeaponSwapKillNearbyOnce(weaponSwapKillRadius);
        }
    }
}

void RunKillPoliceOnlyOnce() {
    try {
        uintptr_t addrBeHit = getAbsoluteAddress(targetLibName, 0x31B830); // PlayerBase.BeHit()
        uintptr_t addrKilledAI = getAbsoluteAddress(targetLibName, 0x2812C4); // MissionCtrl.KilledAI(Player)
        uintptr_t addrClearEnemy = getAbsoluteAddress(targetLibName, 0x280390); // MissionCtrl.ClearEnemy(Player)
        uintptr_t addrFactoryContainPlayerBase = getAbsoluteAddress(targetLibName, 0x2E4A34); // EnemyFactory.ContainPlayerBase(PlayerBase)
        uintptr_t addrFactoryDeletePlayerBase = getAbsoluteAddress(targetLibName, 0x2E4AB4); // EnemyFactory.DeletePlayerBase(PlayerBase)
        uintptr_t addrEnemyGC = getAbsoluteAddress(targetLibName, 0x2E791C); // EnermyGC.EnemyGC(Player)
        if (addrBeHit == 0) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_POLICE", "Kill Police bloqueado: BeHit indisponivel");
            return;
        }

        auto beHit = reinterpret_cast<PlayerBaseBeHitFunc>(addrBeHit);
        auto killedAI = reinterpret_cast<MissionCtrlPlayerActionFunc>(addrKilledAI);
        auto clearEnemy = reinterpret_cast<MissionCtrlPlayerActionFunc>(addrClearEnemy);
        auto factoryContainPlayerBase = reinterpret_cast<EnemyFactoryContainPlayerBaseFunc>(addrFactoryContainPlayerBase);
        auto factoryDeletePlayerBase = reinterpret_cast<EnemyFactoryDeletePlayerBaseFunc>(addrFactoryDeletePlayerBase);
        auto enemyGC = reinterpret_cast<EnemyGCFunc>(addrEnemyGC);

        void* gameCtrl = GetGameCtrlInstance();
        void* missionCtrl = GetMissionCtrlInstance();
        void* enermyGC = IsProbablyValidPtr(gameCtrl) ? *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x24) : nullptr;
        void* enemyFactory = IsProbablyValidPtr(gameCtrl) ? *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x28) : nullptr;

        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        if (enemyCount <= 0) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_POLICE", "Kill Police bloqueado: sem inimigos armados ativos");
            return;
        }

        int touched = 0;
        for (int i = 0; i < enemyCount; ++i) {
            void* playerBase = enemies[i];
            void* player = nullptr;
            if (!IsBulletTailCompatibleEnemyBase(playerBase, &player)) continue;
            if (!IsProbablyValidPtr(playerBase)) continue;

            void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0x14); // PlayerBase.m_dPlayerBaseData
            if (!IsProbablyValidPtr(baseData)) continue;

            void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8); // PlayerBaseData.m_dProperty
            if (!IsProbablyValidPtr(property)) continue;

            int currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14); // PlayerBaseProperty.m_dCurrentBlood
            int maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10);     // PlayerBaseProperty.m_dMaxBlood
            if (currentBlood <= 0 || maxBlood <= 0) continue;

            int lethalDamage = currentBlood + maxBlood + 5000;
            beHit(playerBase, lethalDamage, 0, nullptr);
            touched++;

            if (IsProbablyValidPtr(missionCtrl) && addrKilledAI != 0) {
                killedAI(missionCtrl, player, nullptr);
            }
            if (IsProbablyValidPtr(missionCtrl) && addrClearEnemy != 0) {
                clearEnemy(missionCtrl, player, nullptr);
            }
            if (IsProbablyValidPtr(enemyFactory) && addrFactoryContainPlayerBase != 0 && addrFactoryDeletePlayerBase != 0) {
                if (factoryContainPlayerBase(enemyFactory, playerBase, nullptr)) {
                    factoryDeletePlayerBase(enemyFactory, playerBase, nullptr);
                }
            }
            if (IsProbablyValidPtr(enermyGC) && addrEnemyGC != 0) {
                enemyGC(enermyGC, player, nullptr);
            }
        }

        ShowWordsHintText(touched > 0 ? "NPCS ARMADOS ELIMINADOS" : "SEM NPC ARMADO VALIDO", 2.2f);
        __android_log_print(ANDROID_LOG_INFO, "MOD_POLICE", "Kill Police tocou %d NPCs armados compativeis com trail", touched);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_POLICE", "Falha protegida em RunKillPoliceOnlyOnce");
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

        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        if (enemyCount <= 0) return;

        void* myPlayer = GetMyPlayerInstance();
        int applied = 0;

        for (int i = 0; i < enemyCount; ++i) {
            void* enemyBase = enemies[i];
            if (!IsProbablyValidPtr(enemyBase)) continue;

            void* player = nullptr;
            int baseType = -1;
            int animalType = -1;
            int gunID = 0;
            Vector3 enemyPos = {0.0f, 0.0f, 0.0f};
            if (!ResolveCombatData(enemyBase, &player, baseType, animalType, gunID, enemyPos)) continue;
            if (!IsProbablyValidPtr(player) || player == myPlayer) continue;
            if (!IsHostileEspType(baseType, animalType, gunID)) continue;

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

void* GetBulletTailFactoryInstance() {
    uintptr_t addrGetInstance = getAbsoluteAddress(targetLibName, 0x394D8C); // BulletTailFactory.GetInstance()
    if (addrGetInstance == 0) return nullptr;
    auto getInstance = reinterpret_cast<GetBulletTailFactoryInstanceFunc>(addrGetInstance);
    return getInstance(nullptr);
}

static bool ResolvePlayerStartPos(void* myPlayer, Vector3& outStartPos) {
    if (!IsProbablyValidPtr(myPlayer)) return false;

    try {
        uintptr_t addrPlayerGetPosition = getAbsoluteAddress(targetLibName, 0x341378); // Player.GetPosition()
        if (addrPlayerGetPosition != 0) {
            auto getPlayerPosition = reinterpret_cast<PlayerGetPositionFunc>(addrPlayerGetPosition);
            outStartPos = getPlayerPosition(myPlayer, nullptr);
            return true;
        }

        uintptr_t addrMyCtrlGetPosition = getAbsoluteAddress(targetLibName, 0x4499F8); // MyCtrlPlayer.GetPosition()
        void* myCtrlPlayer = GetMyCtrlPlayerInstance();
        if (addrMyCtrlGetPosition != 0 && IsProbablyValidPtr(myCtrlPlayer)) {
            auto getMyCtrlPosition = reinterpret_cast<MyCtrlPlayerGetPositionFunc>(addrMyCtrlGetPosition);
            outStartPos = getMyCtrlPosition(myCtrlPlayer, nullptr);
            return true;
        }
    } catch (...) {
    }

    return false;
}

static bool IsBulletTailCompatibleEnemyBase(void* enemyBase, void** outPlayer) {
    if (outPlayer) *outPlayer = nullptr;
    if (!IsProbablyValidPtr(enemyBase)) return false;

    try {
        void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0x14); // PlayerBase.m_dPlayerBaseData
        if (!IsProbablyValidPtr(baseData)) return false;

        void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8); // PlayerBaseData.m_dProperty
        if (!IsProbablyValidPtr(property)) return false;

        int baseType = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0xC);      // PlayerBaseProperty.baseType
        int maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10);      // PlayerBaseProperty.m_dMaxBlood
        int currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14);  // PlayerBaseProperty.m_dCurrentBlood
        if (baseType != EnemyNPC || maxBlood <= 0 || currentBlood <= 0) {
            return false;
        }

        void* aiData = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0xC); // PlayerBaseData.m_dAIdata
        if (!IsProbablyValidPtr(aiData)) return false;

        int gunID = *reinterpret_cast<int*>(reinterpret_cast<char*>(aiData) + 0x10); // AIdata.gunID
        if (gunID <= 0) return false;

        void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0xC); // PlayerBase.m_dPlayer
        if (!IsProbablyValidPtr(player)) return false;
        if (!HasTrailCompatiblePlayerAnchors(player)) return false;

        if (outPlayer) *outPlayer = player;
        return true;
    } catch (...) {
        return false;
    }
}

static bool PlayerContainsTransform(void* player, void* targetTransform) {
    if (!IsProbablyValidPtr(player) || !IsProbablyValidPtr(targetTransform)) return false;

    try {
        const int transformOffsets[] = {
                0x24, // Root
                0x28, // Body
                0x38, // Neck
                0x3C, // Head
                0x48, // RHand
                0x54, // LHand
                0x74, // gunRightHand
                0x80  // gunMiddleFront
        };

        for (int offset : transformOffsets) {
            void* current = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + offset);
            if (current == targetTransform) return true;
        }
    } catch (...) {
    }

    return false;
}

static void* ResolvePlayerFromTrackedTransform(void* targetTransform) {
    if (!IsProbablyValidPtr(targetTransform)) return nullptr;

    void* players[256] = {};
    int trackedPlayers = CollectLivingPlayers(players, 256);
    for (int i = 0; i < trackedPlayers; ++i) {
        void* player = players[i];
        if (!IsProbablyValidPtr(player)) continue;
        if (player == GetMyPlayerInstance()) continue;
        if (PlayerContainsTransform(player, targetTransform)) {
            return player;
        }
    }

    return nullptr;
}

static void* ResolveTargetPlayerForBulletTail() {
    void* myCtrlPlayer = GetMyCtrlPlayerInstance();
    if (!IsProbablyValidPtr(myCtrlPlayer)) return nullptr;

    void* targetTransform = FindBestTarget(myCtrlPlayer);
    if (!IsProbablyValidPtr(targetTransform)) return nullptr;
    return ResolvePlayerFromTrackedTransform(targetTransform);
}

bool CanUseBulletTail() {
    if (!IsProbablyValidPtr(GetBulletTailFactoryInstance())) return false;

    void* myPlayer = GetMyPlayerInstance();
    if (!IsProbablyValidPtr(myPlayer)) return false;

    Vector3 startPos = {0.0f, 0.0f, 0.0f};
    if (!ResolvePlayerStartPos(myPlayer, startPos)) return false;

    void* enemies[16] = {};
    int enemyCount = CollectActiveEnemyBases(enemies, 16);
    for (int i = 0; i < enemyCount; ++i) {
        if (IsBulletTailCompatibleEnemyBase(enemies[i], nullptr)) {
            return true;
        }
    }
    return false;
}

bool GenerateBulletTailForPlayer(void* factory, const Vector3& startPos, void* targetPlayer, bool isHit) {
    if (!IsProbablyValidPtr(factory) || !IsProbablyValidPtr(targetPlayer)) return false;

    uintptr_t addrGenerate = getAbsoluteAddress(targetLibName, 0x394DF0); // BulletTailFactory.GenerateEnemyBulletTail()
    if (addrGenerate == 0) return false;

    auto generateBulletTail = reinterpret_cast<GenerateEnemyBulletTailFunc>(addrGenerate);
    generateBulletTail(factory, startPos, targetPlayer, isHit, nullptr);
    return true;
}

int GenerateBulletTailForAllActiveEnemies() {
    try {
        void* factory = GetBulletTailFactoryInstance();
        void* myPlayer = GetMyPlayerInstance();
        if (!IsProbablyValidPtr(factory) || !IsProbablyValidPtr(myPlayer)) {
            return 0;
        }

        Vector3 startPos = {0.0f, 0.0f, 0.0f};
        if (!ResolvePlayerStartPos(myPlayer, startPos)) {
            return 0;
        }

        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        if (enemyCount <= 0) {
            return 0;
        }

        int generated = 0;
        for (int i = 0; i < enemyCount; ++i) {
            void* enemyBase = enemies[i];
            void* targetPlayer = nullptr;
            if (!IsBulletTailCompatibleEnemyBase(enemyBase, &targetPlayer)) continue;

            if (GenerateBulletTailForPlayer(factory, startPos, targetPlayer, true)) {
                generated++;
            }
        }

        return generated;
    } catch (...) {
        return 0;
    }
}

void TriggerBulletTailNow() {
    try {
        int generated = GenerateBulletTailForAllActiveEnemies();
        if (generated <= 0) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "BulletTail bloqueado: nenhum inimigo ativo ou contexto invalido");
            return;
        }
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Trilhas de tiro geradas em %d alvos", generated);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Falha protegida ao gerar BulletTail");
    }
}

void ClearBulletTailNow() {
    try {
        void* factory = GetBulletTailFactoryInstance();
        if (!IsProbablyValidPtr(factory)) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "BulletTail clear ignorado: factory indisponivel");
            return;
        }

        uintptr_t addrDestroyAll = getAbsoluteAddress(targetLibName, 0x395780); // BulletTailFactory.DestroyAllBulletTail()
        if (addrDestroyAll == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "BulletTail clear bloqueado: endereco invalido");
            return;
        }

        auto destroyAllBulletTail = reinterpret_cast<DestroyAllBulletTailFunc>(addrDestroyAll);
        destroyAllBulletTail(factory, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Todas as trilhas de tiro foram limpas");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Falha protegida ao limpar BulletTail");
    }
}

void* GetMiniMapIconCtrlInstance() {
    void* uiManage = GetUIManageInstance();
    if (!IsProbablyValidPtr(uiManage)) return nullptr;

    try {
        void* uiCommon = *reinterpret_cast<void**>(reinterpret_cast<char*>(uiManage) + 0x10); // UI_Manage.uiCommon
        if (!IsProbablyValidPtr(uiCommon)) return nullptr;

        void* uiMiniMapCtrl = *reinterpret_cast<void**>(reinterpret_cast<char*>(uiCommon) + 0x14); // UI_Common.uiMiniMapCtrl
        if (!IsProbablyValidPtr(uiMiniMapCtrl)) return nullptr;

        void* uiIconCtrl = *reinterpret_cast<void**>(reinterpret_cast<char*>(uiMiniMapCtrl) + 0x14); // UI_MiniMapCtrl.uiIconCtrl
        return IsProbablyValidPtr(uiIconCtrl) ? uiIconCtrl : nullptr;
    } catch (...) {
        return nullptr;
    }
}

static void* GetWordsHintsInstance() {
    void* uiManage = GetUIManageInstance();
    if (!IsProbablyValidPtr(uiManage)) return nullptr;

    try {
        void* uiCommon = *reinterpret_cast<void**>(reinterpret_cast<char*>(uiManage) + 0x10); // UI_Manage.uiCommon
        if (!IsProbablyValidPtr(uiCommon)) return nullptr;

        void* wordsHints = *reinterpret_cast<void**>(reinterpret_cast<char*>(uiCommon) + 0x10); // UI_Common.uiWordHints
        return IsProbablyValidPtr(wordsHints) ? wordsHints : nullptr;
    } catch (...) {
        return nullptr;
    }
}

static void* CreateManagedString(const char* text) {
    if (!text || !text[0]) return nullptr;

    void* handle = dlopen((const char*)targetLibName, RTLD_NOW);
    if (!handle) return nullptr;

    void* sym = dlsym(handle, "il2cpp_string_new");
    if (!sym) return nullptr;

    auto il2cppStringNew = reinterpret_cast<Il2CppStringNewFunc>(sym);
    return il2cppStringNew(text);
}

void ShowWordsHintText(const char* text, float showTime) {
    if (!text || !text[0]) {
        __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "UI_WordsHints ignorado: texto vazio");
        return;
    }

    void* wordsHints = GetWordsHintsInstance();
    if (!IsProbablyValidPtr(wordsHints)) {
        __android_log_print(ANDROID_LOG_WARN, "MOD_VISUAL", "UI_WordsHints indisponivel");
        return;
    }

    try {
        uintptr_t addrPlayDisappearAni = getAbsoluteAddress(targetLibName, 0x313B50); // UI_WordsHints.PlayDisappearAni(string, float)
        if (addrPlayDisappearAni == 0) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Endereco invalido para UI_WordsHints.PlayDisappearAni");
            return;
        }

        void* managedString = CreateManagedString(text);
        if (!managedString) {
            __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Falha ao criar string IL2CPP para UI_WordsHints");
            return;
        }

        auto playWordsHint = reinterpret_cast<PlayWordsHintsWithTimeFunc>(addrPlayDisappearAni);
        playWordsHint(wordsHints, managedString, showTime, nullptr);
        __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "Texto exibido via UI_WordsHints: %s", text);
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Falha protegida ao exibir texto via UI_WordsHints");
    }
}

void QueueLoginSuccessHints(const char* displayName, int remainingDays) {
    const char* resolvedName = (displayName && displayName[0]) ? displayName : "ASSINANTE";
    for (int i = 0; i < 4; ++i) {
        std::snprintf(pendingLoginSuccessHints[i], sizeof(pendingLoginSuccessHints[i]),
                      "BEM-VINDO, %s", resolvedName);
    }

    for (int i = 4; i < 8; ++i) {
        std::snprintf(pendingLoginSuccessHints[i], sizeof(pendingLoginSuccessHints[i]),
                      "%s, LOGIN VIP VALIDADO", resolvedName);
    }

    for (int i = 8; i < 12; ++i) {
        if (remainingDays >= 0) {
            std::snprintf(pendingLoginSuccessHints[i], sizeof(pendingLoginSuccessHints[i]),
                          "LOGIN EXPIRA EM %d DIAS", remainingDays);
        } else {
            std::snprintf(pendingLoginSuccessHints[i], sizeof(pendingLoginSuccessHints[i]),
                          "LOGIN EXPIRA EM DATA INDISPONIVEL");
        }
    }

    pendingLoginSuccessHintSequence = true;
}

void RunChaosTimeOnce() {
    try {
        uintptr_t addrSetTimeScale = getAbsoluteAddress(targetLibName, 0x2EF174); // GameCtrl.SetTimeScale(float)
        uintptr_t addrRecoverTimeScale = getAbsoluteAddress(targetLibName, 0x2EEF48); // GameCtrl.RecoverTimeScale()
        if (addrSetTimeScale == 0 || addrRecoverTimeScale == 0) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_CHAOS", "Chaos Time bloqueado: offsets invalidos");
            return;
        }

        auto setTimeScale = reinterpret_cast<GameCtrlSetTimeScaleFunc>(addrSetTimeScale);
        auto recoverTimeScale = reinterpret_cast<GameCtrlRecoverTimeScaleFunc>(addrRecoverTimeScale);

        static int chaosTimeStage = 0;
        static const float kStages[] = {0.25f, 1.75f, 4.0f, 8.0f};

        if (chaosTimeStage >= 4) {
            recoverTimeScale(nullptr);
            ShowWordsHintText("CHAOS TIME RESET", 2.2f);
            __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos Time: TimeScale restaurado");
            chaosTimeStage = 0;
            return;
        }

        float stageValue = kStages[chaosTimeStage];
        setTimeScale(stageValue, nullptr);

        char buffer[64] = {0};
        std::snprintf(buffer, sizeof(buffer), "CHAOS TIME %.2fx", stageValue);
        ShowWordsHintText(buffer, 2.0f);
        __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos Time aplicado: %.2fx", stageValue);
        chaosTimeStage++;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_CHAOS", "Falha protegida no Chaos Time");
    }
}

void RunChaosSpawnOnce() {
    try {
        void* missionCtrl = GetMissionCtrlInstance();
        if (!IsProbablyValidPtr(missionCtrl)) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_CHAOS", "Chaos Spawn bloqueado: MissionCtrl indisponivel");
            return;
        }

        uintptr_t addrGenerateTutorialEnemy = getAbsoluteAddress(targetLibName, 0x281AD8); // MissionCtrl.GenerateTutorialEnemy()
        uintptr_t addrInitNoTaskingAnimal = getAbsoluteAddress(targetLibName, 0x270834); // MissionCtrl.InitNoTaskingAnimal()
        uintptr_t addrInitNoTaskingNpc = getAbsoluteAddress(targetLibName, 0x270D34); // MissionCtrl.InitNoTaskingNpc()
        uintptr_t addrInitNoPermanentNpc = getAbsoluteAddress(targetLibName, 0x271080); // MissionCtrl.InitNoPermanentNpc()
        uintptr_t addrInitCollectableBox = getAbsoluteAddress(targetLibName, 0x2717A4); // MissionCtrl.InitCollectableBox()
        auto generateTutorialEnemy = reinterpret_cast<MissionCtrlGenerateTutorialEnemyFunc>(addrGenerateTutorialEnemy);
        auto initNoTaskingAnimal = reinterpret_cast<MissionCtrlSimpleActionFunc>(addrInitNoTaskingAnimal);
        auto initNoTaskingNpc = reinterpret_cast<MissionCtrlSimpleActionFunc>(addrInitNoTaskingNpc);
        auto initNoPermanentNpc = reinterpret_cast<MissionCtrlSimpleActionFunc>(addrInitNoPermanentNpc);
        auto initCollectableBox = reinterpret_cast<MissionCtrlSimpleActionFunc>(addrInitCollectableBox);

        GeneratePolice(missionCtrl);
        GeneratePolice(missionCtrl);
        GeneratePolice(missionCtrl);
        if (addrInitNoTaskingAnimal != 0) {
            initNoTaskingAnimal(missionCtrl, nullptr);
        }
        if (addrInitNoTaskingNpc != 0) {
            initNoTaskingNpc(missionCtrl, nullptr);
        }
        if (addrInitNoPermanentNpc != 0) {
            initNoPermanentNpc(missionCtrl, nullptr);
        }
        if (addrInitCollectableBox != 0) {
            initCollectableBox(missionCtrl, nullptr);
        }
        if (addrGenerateTutorialEnemy != 0) {
            generateTutorialEnemy(missionCtrl, nullptr);
            generateTutorialEnemy(missionCtrl, nullptr);
        }

        pendingMiniMapEspRefresh = true;
        pendingEspRefresh = true;
        ShowWordsHintText("CHAOS SPAWN MAXIMO", 2.8f);
        __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos Spawn executado com policia, tutorial, npcs, animais e caixas");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_CHAOS", "Falha protegida no Chaos Spawn");
    }
}

void RunChaosUIOnce() {
    try {
        void* creator = GetPlayingUICreatorInstance();
        if (IsProbablyValidPtr(creator)) {
            uintptr_t addrShowHeadshoot = getAbsoluteAddress(targetLibName, 0x3C9730); // UI_PlayingUI_Creator.ShowHeadshoot()
            uintptr_t addrShowMiniMap = getAbsoluteAddress(targetLibName, 0x3C91A0); // UI_PlayingUI_Creator.ShowMiniMap()
            if (addrShowHeadshoot != 0) {
                auto showHeadshoot = reinterpret_cast<ShowHeadshootUIFunc>(addrShowHeadshoot);
                showHeadshoot(creator, nullptr);
            }
            if (addrShowMiniMap != 0) {
                auto showMiniMap = reinterpret_cast<ShowMiniMapUIFunc>(addrShowMiniMap);
                showMiniMap(creator, nullptr);
            }
        }

        ShowWordsHintText("CHAOS UI", 1.8f);
        ShowWordsHintText("ESP VISUAL ESTOURADO", 2.4f);
        RefreshTargetMarkerESP();
        RefreshMiniMapEnemyEsp();
        RefreshCompleteESP();
        GenerateBulletTailForAllActiveEnemies();
        __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos UI executado");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_CHAOS", "Falha protegida no Chaos UI");
    }
}

static void ResetChaosVisualEffects(void* effectCtrl) {
    if (!IsProbablyValidPtr(effectCtrl)) return;

    uintptr_t addrDeleteEffect = getAbsoluteAddress(targetLibName, 0x254078); // GunParticalEffectsCtrl.DeleteEffect()
    uintptr_t addrHideEffect = getAbsoluteAddress(targetLibName, 0x25441C);   // GunParticalEffectsCtrl.HideEffect()
    uintptr_t addrRecoverEffect = getAbsoluteAddress(targetLibName, 0x254578); // GunParticalEffectsCtrl.RecoverEffect()
    if (addrDeleteEffect == 0 || addrHideEffect == 0 || addrRecoverEffect == 0) return;

    auto deleteEffect = reinterpret_cast<GunParticalDeleteEffectFunc>(addrDeleteEffect);
    auto hideEffect = reinterpret_cast<GunParticalToggleEffectFunc>(addrHideEffect);
    auto recoverEffect = reinterpret_cast<GunParticalToggleEffectFunc>(addrRecoverEffect);

    const int kEffectsToReset[] = {
            (int)Effects_GunHitBlood,
            (int)Effects_MissionMainEffect,
            (int)Effects_MissionBranchEffect,
            (int)Effects_QuickDrawEffect,
            (int)Effects_DartEffect,
            (int)Effects_DestinationEffect,
            (int)Effects_Gunshoot_Pistol,
            (int)Effects_Gunshoot_Rifle,
            (int)Effects_Gunshoot_Shotgun
    };

    for (int effectType : kEffectsToReset) {
        deleteEffect(effectCtrl, effectType, 0, nullptr);
    }

    // Pulso rápido para forçar o grupo visual a sair de estado oculto caso algum efeito antigo tenha ficado escondido.
    hideEffect(effectCtrl, (int)Effects_Gunshoot_Shotgun, nullptr);
    recoverEffect(effectCtrl, (int)Effects_Gunshoot_Shotgun, nullptr);
    recoverEffect(effectCtrl, (int)Effects_MissionMainEffect, nullptr);
}

void RunChaosFxOnce() {
    try {
        void* effectCtrl = GetGunParticalEffectsCtrlInstance();
        if (!IsProbablyValidPtr(effectCtrl)) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_CHAOS", "Chaos FX bloqueado: effectCtrl indisponivel");
            return;
        }

        ResetChaosVisualEffects(effectCtrl);
        TurnOffSceneMissionHints();
        TurnOnSceneMissionHints();

        void* sceneCtrl = GetSceneInOutPosCtrlInstance();
        if (IsProbablyValidPtr(sceneCtrl)) {
            uintptr_t addrDisableSceneFx = getAbsoluteAddress(targetLibName, 0x326E1C); // SceneInOutPosCtrl.DisableEffect()
            uintptr_t addrEnableSceneFx = getAbsoluteAddress(targetLibName, 0x326F08);   // SceneInOutPosCtrl.EnableEffect()
            if (addrDisableSceneFx != 0 && addrEnableSceneFx != 0) {
                auto disableSceneFx = reinterpret_cast<SceneInOutToggleEffectFunc>(addrDisableSceneFx);
                auto enableSceneFx = reinterpret_cast<SceneInOutToggleEffectFunc>(addrEnableSceneFx);
                disableSceneFx(sceneCtrl, nullptr);
                enableSceneFx(sceneCtrl, nullptr);
            }
        }

        void* creator = GetPlayingUICreatorInstance();
        if (IsProbablyValidPtr(creator)) {
            uintptr_t addrShowHeadshoot = getAbsoluteAddress(targetLibName, 0x3C9730); // UI_PlayingUI_Creator.ShowHeadshoot()
            uintptr_t addrShowMiniMap = getAbsoluteAddress(targetLibName, 0x3C91A0);   // UI_PlayingUI_Creator.ShowMiniMap()
            uintptr_t addrEnableAimCenter = getAbsoluteAddress(targetLibName, 0x3C95F4); // UI_PlayingUI_Creator.EnableAimCenterUI(bool)
            uintptr_t addrSetMissionTime = getAbsoluteAddress(targetLibName, 0x3C9B24);   // UI_PlayingUI_Creator.SetMissionTimeRegulator(float)
            uintptr_t addrDisableMissionTime = getAbsoluteAddress(targetLibName, 0x3C9C00); // UI_PlayingUI_Creator.DisableMissitonTimeRegulator()
            uintptr_t addrSetHorseSprintEnable = getAbsoluteAddress(targetLibName, 0x3CA03C); // UI_PlayingUI_Creator.SetHorseSprintEnable(bool)
            uintptr_t addrSetHorseSprintValue = getAbsoluteAddress(targetLibName, 0x3CA1EC);  // UI_PlayingUI_Creator.SetCurrentHorseSprint(float)
            if (addrShowHeadshoot != 0) {
                auto showHeadshoot = reinterpret_cast<ShowHeadshootUIFunc>(addrShowHeadshoot);
                showHeadshoot(creator, nullptr);
                showHeadshoot(creator, nullptr);
            }
            if (addrShowMiniMap != 0) {
                auto showMiniMap = reinterpret_cast<ShowMiniMapUIFunc>(addrShowMiniMap);
                showMiniMap(creator, nullptr);
            }
            if (addrEnableAimCenter != 0) {
                auto enableAimCenter = reinterpret_cast<EnableAimCenterUIFunc>(addrEnableAimCenter);
                enableAimCenter(creator, true, nullptr);
            }
            if (addrSetMissionTime != 0) {
                auto setMissionTime = reinterpret_cast<SetMissionTimeRegulatorFunc>(addrSetMissionTime);
                setMissionTime(creator, 0.42f, nullptr);
                setMissionTime(creator, 0.18f, nullptr);
            }
            if (addrDisableMissionTime != 0) {
                auto disableMissionTime = reinterpret_cast<DisableMissionTimeRegulatorFunc>(addrDisableMissionTime);
                disableMissionTime(creator, nullptr);
            }
            if (addrSetHorseSprintEnable != 0) {
                auto setHorseSprintEnable = reinterpret_cast<SetHorseSprintEnableFunc>(addrSetHorseSprintEnable);
                setHorseSprintEnable(creator, true, nullptr);
            }
            if (addrSetHorseSprintValue != 0) {
                auto setHorseSprintValue = reinterpret_cast<SetCurrentHorseSprintFunc>(addrSetHorseSprintValue);
                setHorseSprintValue(creator, 0.92f, nullptr);
                setHorseSprintValue(creator, 1.0f, nullptr);
            }
        }

        ClearCompleteESP();
        RefreshTargetMarkerESP();
        ShowTargetMarkerOnCurrentTarget();
        RefreshMiniMapEnemyEsp();
        RefreshCompleteESP();
        RefreshCompleteESP();
        ClearBulletTailNow();
        GenerateBulletTailForAllActiveEnemies();
        GenerateBulletTailForAllActiveEnemies();

        ShowWordsHintText("CHAOS FX SEGURO", 2.2f);
        ShowWordsHintText("PULSO VISUAL MAXIMO", 2.8f);
        __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos FX executado em modo seguro e intensificado");
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_CHAOS", "Falha protegida no Chaos FX");
    }
}

void RunChaosWeaponOnce() {
    try {
        void* gameCtrl = GetGameCtrlInstance();
        if (!IsProbablyValidPtr(gameCtrl)) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_CHAOS", "Chaos Weapon bloqueado: GameCtrl indisponivel");
            return;
        }

        MyPlayerOriData* playerData = GetPlayerOriData();
        if (!playerData) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_CHAOS", "Chaos Weapon bloqueado: MyPlayerOriData indisponivel");
            return;
        }

        uintptr_t addrChangedWeapon = getAbsoluteAddress(targetLibName, 0x2F1EDC); // GameCtrl.ChangedWeapon(int, bool)
        uintptr_t addrSetHorseParam = getAbsoluteAddress(targetLibName, 0x2F0F40); // GameCtrl.SetHorseParam(int)
        uintptr_t addrChangeHorse = getAbsoluteAddress(targetLibName, 0x2F2450); // GameCtrl.ChangeHorse(int)
        if (addrChangedWeapon == 0 || addrSetHorseParam == 0 || addrChangeHorse == 0) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_CHAOS", "Chaos Weapon bloqueado: offsets invalidos");
            return;
        }

        auto changedWeapon = reinterpret_cast<GameCtrlChangedWeaponFunc>(addrChangedWeapon);
        auto setHorseParam = reinterpret_cast<GameCtrlSetHorseParamFunc>(addrSetHorseParam);
        auto changeHorse = reinterpret_cast<GameCtrlChangeHorseFunc>(addrChangeHorse);

        static int weaponStage = 0;
        int stage = weaponStage % 3;
        int appliedId = 0;
        const char* appliedKind = "LOADOUT";

        if (stage == 0) {
            int gunId = playerData->gun_ID > 0 ? playerData->gun_ID :
                        (playerData->pistol_ID > 0 ? playerData->pistol_ID : playerData->long_gun_ID);
            if (gunId > 0) {
                changedWeapon(gameCtrl, gunId, true, nullptr);
                appliedId = gunId;
                appliedKind = "ARMA";
            }
        } else if (stage == 1) {
            int knifeId = playerData->knife_ID;
            if (knifeId > 0) {
                changedWeapon(gameCtrl, knifeId, false, nullptr);
                appliedId = knifeId;
                appliedKind = "FACA";
            }
        } else {
            int horseId = playerData->horse_ID;
            if (horseId < 10 || horseId > 15) {
                horseId = 10;
            } else {
                horseId++;
                if (horseId > 15) horseId = 10;
            }
            setHorseParam(gameCtrl, horseId, nullptr);
            changeHorse(gameCtrl, horseId, nullptr);
            appliedId = horseId;
            appliedKind = "CAVALO";
        }

        if (appliedId <= 0) {
            __android_log_print(ANDROID_LOG_WARN, "MOD_CHAOS", "Chaos Weapon ignorado: IDs atuais do player invalidos");
            return;
        }

        char buffer[96] = {0};
        std::snprintf(buffer, sizeof(buffer), "CHAOS %s %d", appliedKind, appliedId);
        ShowWordsHintText(buffer, 2.6f);
        __android_log_print(ANDROID_LOG_INFO, "MOD_CHAOS", "Chaos Weapon aplicado: tipo=%s id=%d", appliedKind, appliedId);
        weaponStage++;
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_CHAOS", "Falha protegida no Chaos Weapon");
    }
}

void RunNpcWarFrame() {
    static int npcWarFrameCounter = 0;
    static void* cachedAttackerPlayer = nullptr;
    static void* cachedDefenderPlayer = nullptr;
    static void* cachedAttackerBase = nullptr;
    static void* cachedDefenderBase = nullptr;
    static Vector3 cachedAttackerPos = {0.0f, 0.0f, 0.0f};
    static Vector3 cachedDefenderPos = {0.0f, 0.0f, 0.0f};
    npcWarFrameCounter++;
    if ((npcWarFrameCounter % 30) != 0) return;

    try {
        uintptr_t addrBeHit = getAbsoluteAddress(targetLibName, 0x31B830); // PlayerBase.BeHit()
        uintptr_t addrGetAttackBlood = getAbsoluteAddress(targetLibName, 0x31B7F8); // PlayerBase.GetAttackBlood()
        if (addrBeHit == 0) return;

        auto beHit = reinterpret_cast<PlayerBaseBeHitFunc>(addrBeHit);
        auto getAttackBlood = reinterpret_cast<int (*)(void*, void*)>(addrGetAttackBlood);

        void* gameCtrl = GetGameCtrlInstance();
        if (!IsProbablyValidPtr(gameCtrl)) return;

        const bool needsRefresh =
                !IsProbablyValidPtr(cachedAttackerPlayer) || !IsProbablyValidPtr(cachedDefenderPlayer) ||
                !IsProbablyValidPtr(cachedAttackerBase) || !IsProbablyValidPtr(cachedDefenderBase) ||
                (npcWarFrameCounter % 150) == 0;

        if (needsRefresh) {
            void* attackerPlayer = nullptr;
            void* defenderPlayer = nullptr;
            void* attackerBase = nullptr;
            void* defenderBase = nullptr;
            Vector3 attackerPos = {0.0f, 0.0f, 0.0f};
            Vector3 defenderPos = {0.0f, 0.0f, 0.0f};
            float bestDistanceSq = 1.0e30f;

            bool foundPolicePair = false;
            void* missionCtrl = GetMissionCtrlInstance();
            if (IsProbablyValidPtr(missionCtrl)) {
                void* policeListPtr = *reinterpret_cast<void**>(reinterpret_cast<char*>(missionCtrl) + 0x44); // MissionCtrl.policePlayers
                if (IsProbablyValidPtr(policeListPtr)) {
                    auto* policeList = reinterpret_cast<Il2CppList<void*>*>(policeListPtr);
                    if (policeList && policeList->items) {
                        int size = policeList->size;
                        if (size > 1 && size <= 64) {
                            uint32_t maxLength = policeList->items->max_length;
                            int limit = size < static_cast<int>(maxLength) ? size : static_cast<int>(maxLength);
                            for (int i = 0; i < limit; ++i) {
                                void* lhsPlayer = policeList->items->items[i];
                                void* lhsBase = nullptr;
                                Vector3 lhsPos = {0.0f, 0.0f, 0.0f};
                                if (!IsPoliceWarEligiblePlayer(lhsPlayer, &lhsBase, &lhsPos)) continue;

                                for (int j = i + 1; j < limit; ++j) {
                                    void* rhsPlayer = policeList->items->items[j];
                                    void* rhsBase = nullptr;
                                    Vector3 rhsPos = {0.0f, 0.0f, 0.0f};
                                    if (!IsPoliceWarEligiblePlayer(rhsPlayer, &rhsBase, &rhsPos)) continue;

                                    float dx = rhsPos.x - lhsPos.x;
                                    float dy = rhsPos.y - lhsPos.y;
                                    float dz = rhsPos.z - lhsPos.z;
                                    float distanceSq = dx * dx + dy * dy + dz * dz;
                                    if (distanceSq < 9.0f || distanceSq > 256.0f) continue;

                                    if (distanceSq < bestDistanceSq) {
                                        bestDistanceSq = distanceSq;
                                        attackerPlayer = lhsPlayer;
                                        defenderPlayer = rhsPlayer;
                                        attackerPos = lhsPos;
                                        defenderPos = rhsPos;
                                        attackerBase = lhsBase;
                                        defenderBase = rhsBase;
                                        foundPolicePair = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            if (!foundPolicePair) {
                void* enemies[48] = {};
                int enemyCount = CollectActiveEnemyBases(enemies, 48);
                if (enemyCount < 2) return;

                for (int i = 0; i < enemyCount; ++i) {
                    void* lhsPlayer = nullptr;
                    void* lhsBase = nullptr;
                    Vector3 lhsPos = {0.0f, 0.0f, 0.0f};
                    if (!IsNpcWarEligibleEnemyBase(enemies[i], &lhsPlayer, &lhsBase, &lhsPos)) continue;

                    for (int j = i + 1; j < enemyCount; ++j) {
                        void* rhsPlayer = nullptr;
                        void* rhsBase = nullptr;
                        Vector3 rhsPos = {0.0f, 0.0f, 0.0f};
                        if (!IsNpcWarEligibleEnemyBase(enemies[j], &rhsPlayer, &rhsBase, &rhsPos)) continue;
                        if (!IsProbablyValidPtr(rhsPlayer) || rhsPlayer == lhsPlayer) continue;

                        float dx = rhsPos.x - lhsPos.x;
                        float dy = rhsPos.y - lhsPos.y;
                        float dz = rhsPos.z - lhsPos.z;
                        float distanceSq = dx * dx + dy * dy + dz * dz;
                        if (distanceSq < 9.0f || distanceSq > 196.0f) continue;

                        if (distanceSq < bestDistanceSq) {
                            bestDistanceSq = distanceSq;
                            attackerPlayer = lhsPlayer;
                            defenderPlayer = rhsPlayer;
                            attackerPos = lhsPos;
                            defenderPos = rhsPos;
                            attackerBase = lhsBase;
                            defenderBase = rhsBase;
                        }
                    }
                }
            }

            cachedAttackerPlayer = attackerPlayer;
            cachedDefenderPlayer = defenderPlayer;
            cachedAttackerBase = attackerBase;
            cachedDefenderBase = defenderBase;
            cachedAttackerPos = attackerPos;
            cachedDefenderPos = defenderPos;
        } else {
            if (!GetPlayerWorldPosition(cachedAttackerPlayer, cachedAttackerPos) ||
                !GetPlayerWorldPosition(cachedDefenderPlayer, cachedDefenderPos)) {
                cachedAttackerPlayer = nullptr;
                cachedDefenderPlayer = nullptr;
                cachedAttackerBase = nullptr;
                cachedDefenderBase = nullptr;
                return;
            }
        }

        if (!IsProbablyValidPtr(cachedAttackerPlayer) || !IsProbablyValidPtr(cachedDefenderPlayer) ||
            !IsProbablyValidPtr(cachedAttackerBase) || !IsProbablyValidPtr(cachedDefenderBase)) {
            return;
        }

        int attackerDamage = 6;
        int defenderDamage = 6;
        if (addrGetAttackBlood != 0) {
            int lhsDamage = getAttackBlood(cachedAttackerBase, nullptr);
            int rhsDamage = getAttackBlood(cachedDefenderBase, nullptr);
            if (lhsDamage > 0 && lhsDamage < 5000) attackerDamage = lhsDamage;
            if (rhsDamage > 0 && rhsDamage < 5000) defenderDamage = rhsDamage;
        }
        attackerDamage = attackerDamage > 18 ? 18 : attackerDamage;
        defenderDamage = defenderDamage > 18 ? 18 : defenderDamage;

        beHit(cachedDefenderBase, attackerDamage, 0, nullptr);
        beHit(cachedAttackerBase, defenderDamage, 0, nullptr);

        if ((npcWarFrameCounter % 90) == 0) {
            void* bulletTailFactory = GetBulletTailFactoryInstance();
            if (IsProbablyValidPtr(bulletTailFactory)) {
                GenerateBulletTailForPlayer(bulletTailFactory, cachedAttackerPos, cachedDefenderPlayer, true);
                GenerateBulletTailForPlayer(bulletTailFactory, cachedDefenderPos, cachedAttackerPlayer, true);
            }
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_CHAOS", "Falha protegida no NPC War");
    }
}

static int GetListCountSafe(void* listPtr) {
    if (!IsProbablyValidPtr(listPtr)) return 0;

    try {
        auto* list = reinterpret_cast<Il2CppList<void*>*>(listPtr);
        if (!list || !list->items) return 0;
        int size = list->size;
        return (size > 0 && size <= 512) ? size : 0;
    } catch (...) {
        return 0;
    }
}

static const char* GetSceneName(int sceneValue) {
    switch (sceneValue) {
        case GameScene_NoviceVillage: return "CENA: NOVICE VILLAGE";
        case GameScene_Cell: return "CENA: CELL";
        case GameScene_Canyon: return "CENA: CANYON";
        case GameScene_Forest: return "CENA: FOREST";
        case GameScene_Bar: return "CENA: BAR";
        case GameScene_Mine: return "CENA: MINE";
        case GameScene_Cemetery: return "CENA: CEMETERY";
        default: return nullptr;
    }
}

static void* GetPlayerBaseFromPlayer(void* player) {
    if (!IsProbablyValidPtr(player)) return nullptr;

    try {
        void* playerBase = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0xC); // Player.m_player
        return IsProbablyValidPtr(playerBase) ? playerBase : nullptr;
    } catch (...) {
        return nullptr;
    }
}

static void* ResolveTeleportTargetPlayer(int requestMode) {
    void* myPlayer = GetMyPlayerInstance();
    if (!IsProbablyValidPtr(myPlayer)) return nullptr;

    if (requestMode == TeleportCurrentTarget) {
        void* resolvedTarget = ResolveTargetPlayerForBulletTail();
        if (IsProbablyValidPtr(resolvedTarget)) return resolvedTarget;

        void* myCtrlPlayer = GetMyCtrlPlayerInstance();
        if (!IsProbablyValidPtr(myCtrlPlayer)) return nullptr;

        void* targetTransform = FindBestTarget(myCtrlPlayer);
        return ResolvePlayerFromTrackedTransform(targetTransform);
    }

    Vector3 myPos = {0.0f, 0.0f, 0.0f};
    if (!GetPlayerWorldPosition(myPlayer, myPos)) return nullptr;

    if (requestMode == TeleportNearestHostile) {
        void* players[256] = {};
        int playerCount = CollectLivingPlayers(players, 256);
        void* bestPlayer = nullptr;
        float bestDistanceSq = 1.0e30f;

        for (int i = 0; i < playerCount; ++i) {
            void* enemyPlayer = players[i];
            if (!IsProbablyValidPtr(enemyPlayer) || enemyPlayer == myPlayer) continue;

            int baseType = -1;
            int animalType = -1;
            int gunID = 0;
            Vector3 enemyPos = {0.0f, 0.0f, 0.0f};
            if (!ResolvePlayerMetaFromPlayer(enemyPlayer, baseType, animalType, gunID, &enemyPos)) continue;
            if (!IsTeleportHostileType(baseType, animalType, gunID)) continue;

            const float dx = enemyPos.x - myPos.x;
            const float dy = enemyPos.y - myPos.y;
            const float dz = enemyPos.z - myPos.z;
            const float distanceSq = dx * dx + dy * dy + dz * dz;
            if (distanceSq < bestDistanceSq) {
                bestDistanceSq = distanceSq;
                bestPlayer = enemyPlayer;
            }
        }

        return bestPlayer;
    }

    if (requestMode == TeleportNearestMissionNpc || requestMode == TeleportNearestMapNpc) {
        void* players[256] = {};
        int playerCount = CollectLivingPlayers(players, 256);
        void* bestPlayer = nullptr;
        float bestDistanceSq = 1.0e30f;

        for (int i = 0; i < playerCount; ++i) {
            void* player = players[i];
            if (!IsProbablyValidPtr(player) || player == myPlayer) continue;

            int baseType = -1;
            int animalType = -1;
            int gunID = 0;
            Vector3 playerPos = {0.0f, 0.0f, 0.0f};
            if (!ResolvePlayerMetaFromPlayer(player, baseType, animalType, gunID, &playerPos)) continue;

            if (requestMode == TeleportNearestMissionNpc && baseType != MissionPerson) continue;
            if (requestMode == TeleportNearestMapNpc && baseType != NonPermanentNpc) continue;

            const float dx = playerPos.x - myPos.x;
            const float dy = playerPos.y - myPos.y;
            const float dz = playerPos.z - myPos.z;
            const float distanceSq = dx * dx + dy * dy + dz * dz;
            if (distanceSq < bestDistanceSq) {
                bestDistanceSq = distanceSq;
                bestPlayer = player;
            }
        }

        return bestPlayer;
    }

    return nullptr;
}

static bool TeleportMyPlayerNearTarget(void* targetPlayer, const char* reasonText) {
    if (!IsProbablyValidPtr(targetPlayer)) return false;

    void* myCtrlPlayer = GetMyCtrlPlayerInstance();
    void* ctrlPlayer = GetCtrlPlayerFromMyCtrl();
    if (!IsProbablyValidPtr(myCtrlPlayer) || !IsProbablyValidPtr(ctrlPlayer)) return false;

    uintptr_t addrSetPosMyCtrl = getAbsoluteAddress(targetLibName, 0x45D69C); // MyCtrlPlayer.SetPosition(Vector3)
    uintptr_t addrSetPosPlayer = getAbsoluteAddress(targetLibName, 0x348A38); // Player.SetPosition(Vector3)
    uintptr_t addrSetCurrentVelocity = getAbsoluteAddress(targetLibName, 0x35C8F8); // Player.SetCurrentVelocity(Vector3)
    uintptr_t addrSetNavMeshEnable = getAbsoluteAddress(targetLibName, 0x34897C); // Player.SetNavMesEnable(bool)
    uintptr_t addrSetNavMeshSpeed = getAbsoluteAddress(targetLibName, 0x35C038); // Player.SetNavMesSpeed(float)
    if (addrSetPosMyCtrl == 0 || addrSetPosPlayer == 0 || addrSetCurrentVelocity == 0) return false;

    auto setPosMyCtrl = reinterpret_cast<MyCtrlPlayerSetPositionFunc>(addrSetPosMyCtrl);
    auto setPosPlayer = reinterpret_cast<PlayerSetPositionFunc>(addrSetPosPlayer);
    auto setCurrentVelocity = reinterpret_cast<PlayerSetCurrentVelocityFunc>(addrSetCurrentVelocity);
    auto setNavMeshEnable = reinterpret_cast<PlayerSetNavMeshEnableFunc>(addrSetNavMeshEnable);
    auto setNavMeshSpeed = reinterpret_cast<PlayerSetNavMeshSpeedFunc>(addrSetNavMeshSpeed);

    Vector3 myPos = {0.0f, 0.0f, 0.0f};
    Vector3 targetPos = {0.0f, 0.0f, 0.0f};
    if (!GetPlayerWorldPosition(ctrlPlayer, myPos) || !GetPlayerWorldPosition(targetPlayer, targetPos)) return false;

    Vector3 offsetDir = NormalizeXZ({targetPos.x - myPos.x, 0.0f, targetPos.z - myPos.z});
    if (Vector3LengthXZ(offsetDir) <= 0.001f) {
        offsetDir = {1.0f, 0.0f, 0.0f};
    }

    Vector3 finalPos = targetPos;
    finalPos.x -= offsetDir.x * 1.8f;
    finalPos.z -= offsetDir.z * 1.8f;
    finalPos.y += 0.15f;

    Vector3 zeroVel = {0.0f, 0.0f, 0.0f};
    if (addrSetNavMeshEnable != 0) {
        setNavMeshEnable(ctrlPlayer, true, nullptr);
    }
    if (addrSetNavMeshSpeed != 0) {
        setNavMeshSpeed(ctrlPlayer, 4.0f, nullptr);
    }
    setCurrentVelocity(ctrlPlayer, zeroVel, nullptr);
    setPosPlayer(ctrlPlayer, finalPos, nullptr);
    setPosMyCtrl(myCtrlPlayer, finalPos, nullptr);

    char buffer[96] = {0};
    std::snprintf(buffer, sizeof(buffer), "TELEPORT: %s", reasonText ? reasonText : "ALVO");
    ShowWordsHintText(buffer, 2.4f);
    return true;
}

bool RunTeleportToRequest(int requestMode) {
    const char* reasonText = nullptr;
    switch (requestMode) {
        case TeleportCurrentTarget: reasonText = "ALVO ATUAL"; break;
        case TeleportNearestHostile: reasonText = "HOSTIL MAIS PROXIMO"; break;
        case TeleportNearestMissionNpc: reasonText = "NPC DE MISSAO"; break;
        case TeleportNearestMapNpc: reasonText = "NPC DO MAPA"; break;
        default: return false;
    }

    void* targetPlayer = ResolveTeleportTargetPlayer(requestMode);
    if (!IsProbablyValidPtr(targetPlayer)) {
        ShowWordsHintText("TELEPORT FALHOU: ALVO INDISPONIVEL", 2.4f);
        return false;
    }

    if (!TeleportMyPlayerNearTarget(targetPlayer, reasonText)) {
        ShowWordsHintText("TELEPORT FALHOU: POSICAO INVALIDA", 2.4f);
        return false;
    }

    return true;
}

static bool GetTargetKindText(char* outText, size_t outSize) {
    if (!outText || outSize == 0) return false;
    outText[0] = '\0';

    void* targetPlayer = ResolveTargetPlayerForBulletTail();
    if (!IsProbablyValidPtr(targetPlayer)) return false;

    void* playerBase = GetPlayerBaseFromPlayer(targetPlayer);
    if (!IsProbablyValidPtr(playerBase)) return false;

    try {
        void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0x14); // PlayerBase.m_dPlayerBaseData
        if (!IsProbablyValidPtr(baseData)) return false;

        void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8); // PlayerBaseData.m_dProperty
        if (!IsProbablyValidPtr(property)) return false;

        int baseType = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0xC);
        int modelType = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x8);

        switch (baseType) {
            case EnemyNPC:
                std::snprintf(outText, outSize, "ALVO: NPC ARMADO");
                return true;
            case Zombies:
                std::snprintf(outText, outSize, "ALVO: ZUMBI");
                return true;
            case Ogre:
                std::snprintf(outText, outSize, "ALVO: OGRO");
                return true;
            case Animal:
                switch (modelType) {
                    case 8: std::snprintf(outText, outSize, "ALVO: CHEETAH"); return true;
                    case 9: std::snprintf(outText, outSize, "ALVO: BEAR"); return true;
                    case 10: std::snprintf(outText, outSize, "ALVO: WOLF"); return true;
                    case 11: std::snprintf(outText, outSize, "ALVO: DEER"); return true;
                    case 12: std::snprintf(outText, outSize, "ALVO: EAGLE"); return true;
                    case 13: std::snprintf(outText, outSize, "ALVO: FOX"); return true;
                    default: std::snprintf(outText, outSize, "ALVO: ANIMAL"); return true;
                }
            case MissionPerson:
                std::snprintf(outText, outSize, "ALVO: NPC DE MISSAO");
                return true;
            case NonPermanentNpc:
                std::snprintf(outText, outSize, "ALVO: NPC DO MAPA");
                return true;
            case Horse:
                std::snprintf(outText, outSize, "ALVO: CAVALO");
                return true;
            default:
                return false;
        }
    } catch (...) {
        return false;
    }
}

static const char* GetMissionStatusText() {
    void* missionCtrl = GetMissionCtrlInstance();
    if (!IsProbablyValidPtr(missionCtrl)) return nullptr;

    try {
        uintptr_t addrGetMainState = getAbsoluteAddress(targetLibName, 0x271EE0); // MissionCtrl.GetMissionMainState()
        uintptr_t addrGetBranchState = getAbsoluteAddress(targetLibName, 0x271F6C); // MissionCtrl.GetMissionBranchState()
        if (addrGetMainState == 0 || addrGetBranchState == 0) return nullptr;

        auto getMainState = reinterpret_cast<MissionCtrlGetStateFunc>(addrGetMainState);
        auto getBranchState = reinterpret_cast<MissionCtrlGetStateFunc>(addrGetBranchState);

        int mainState = getMainState(missionCtrl, nullptr);
        int branchState = getBranchState(missionCtrl, nullptr);

        if (mainState == 1) return "MISSAO PRINCIPAL ATIVA";
        if (branchState == 1) return "MISSAO SECUNDARIA ATIVA";
        if (mainState == 2 && branchState == 2) return "TODAS AS MISSOES FINALIZADAS";
        if (mainState == 0 && branchState == 0) return "SEM MISSAO ATIVA";
    } catch (...) {
    }

    return nullptr;
}

void ProcessGameplayHints() {
    static bool initialized = false;
    static bool loginSuccessSequenceActive = false;
    static int loginSuccessSequenceIndex = 0;
    static long long nextLoginSuccessHintAtMs = 0;
    static bool lastCompleteEsp = false;
    static bool lastAutoKill = false;
    static bool lastBulletTailEsp = false;
    static bool lastAimBotAggressive = false;
    static bool lastNpcWar = false;
    static int lastScene = -1;
    static int lastEnemyCount = -1;
    static int lastPoliceCount = -1;
    static int lastTrackedPlayers = -1;
    static char lastTargetText[64] = {0};
    static char lastMissionText[64] = {0};

    if (!initialized) {
        void* initialTracked[256] = {};
        lastCompleteEsp = completeEsp;
        lastAutoKill = autoKill;
        lastBulletTailEsp = bulletTailEsp;
        lastAimBotAggressive = aimBotAggressive;
        lastNpcWar = npcWarMode;
        lastScene = GetCurrentGameSceneValue();
        lastTrackedPlayers = CollectTrackedPlayers(initialTracked, 256);
        initialized = true;
    }

    if (pendingLoginSuccessHintSequence) {
        pendingLoginSuccessHintSequence = false;
        loginSuccessSequenceActive = true;
        loginSuccessSequenceIndex = 0;
        nextLoginSuccessHintAtMs = 0;
    }

    if (loginSuccessSequenceActive) {
        const long long nowMs = GetMonotonicTimeMs();
        if (nowMs >= nextLoginSuccessHintAtMs) {
            const char* nextText = pendingLoginSuccessHints[loginSuccessSequenceIndex];
            if (nextText && nextText[0]) {
                ShowWordsHintText(nextText, 2.6f);
            }

            loginSuccessSequenceIndex++;
            if (loginSuccessSequenceIndex >= 12) {
                loginSuccessSequenceActive = false;
            } else {
                nextLoginSuccessHintAtMs = nowMs + 2200;
            }
        }
    }

    if (lastCompleteEsp != completeEsp) {
        ShowWordsHintText(completeEsp ? "ESP COMPLETO ON" : "ESP COMPLETO OFF", 2.0f);
        lastCompleteEsp = completeEsp;
    }

    if (lastAutoKill != autoKill) {
        ShowWordsHintText(autoKill ? "AUTO KILL ON" : "AUTO KILL OFF", 2.0f);
        lastAutoKill = autoKill;
    }

    if (lastBulletTailEsp != bulletTailEsp) {
        ShowWordsHintText(bulletTailEsp ? "TRAIL ESP ON" : "TRAIL ESP OFF", 2.0f);
        lastBulletTailEsp = bulletTailEsp;
    }

    if (lastAimBotAggressive != aimBotAggressive) {
        ShowWordsHintText(aimBotAggressive ? "AIMBOT AGRESSIVO ON" : "AIMBOT AGRESSIVO OFF", 2.0f);
        lastAimBotAggressive = aimBotAggressive;
    }

    if (lastNpcWar != npcWarMode) {
        ShowWordsHintText(npcWarMode ? "NPC WAR ON" : "NPC WAR OFF", 2.0f);
        lastNpcWar = npcWarMode;
    }

    int currentScene = GetCurrentGameSceneValue();
    if (currentScene != lastScene) {
        const char* sceneText = GetSceneName(currentScene);
        if (sceneText) ShowWordsHintText(sceneText, 2.5f);
        lastScene = currentScene;
    }

    void* enemies[128] = {};
    int enemyCount = CollectActiveEnemyBases(enemies, 128);
    if (enemyCount != lastEnemyCount) {
        char buffer[64] = {0};
        if (enemyCount == 0 && lastEnemyCount > 0) {
            ShowWordsHintText("AREA LIMPA", 2.5f);
        } else if (enemyCount > 0 && enemyCount <= 5) {
            std::snprintf(buffer, sizeof(buffer), "INIMIGOS RESTANTES: %d", enemyCount);
            ShowWordsHintText(buffer, 2.2f);
        }
        lastEnemyCount = enemyCount;
    }

    void* missionCtrl = GetMissionCtrlInstance();
    if (IsProbablyValidPtr(missionCtrl)) {
        int policeCount = GetListCountSafe(*reinterpret_cast<void**>(reinterpret_cast<char*>(missionCtrl) + 0x44)); // policePlayers
        if (policeCount != lastPoliceCount) {
            char buffer[64] = {0};
            if (policeCount > 0) {
                std::snprintf(buffer, sizeof(buffer), "POLICIA ATIVA: %d", policeCount);
                ShowWordsHintText(buffer, 2.3f);
            } else if (lastPoliceCount > 0) {
                ShowWordsHintText("POLICIA DISPERSA", 2.0f);
            }
            lastPoliceCount = policeCount;
        }

        const char* missionText = GetMissionStatusText();
        if (missionText && std::strncmp(missionText, lastMissionText, sizeof(lastMissionText) - 1) != 0) {
            ShowWordsHintText(missionText, 2.3f);
            std::strncpy(lastMissionText, missionText, sizeof(lastMissionText) - 1);
            lastMissionText[sizeof(lastMissionText) - 1] = '\0';
        }
    }

    int trackedPlayers = 0;
    void* tracked[256] = {};
    trackedPlayers = CollectTrackedPlayers(tracked, 256);
    if (lastTrackedPlayers >= 0 && trackedPlayers > lastTrackedPlayers) {
        int delta = trackedPlayers - lastTrackedPlayers;
        char buffer[64] = {0};
        if (delta == 1) {
            std::snprintf(buffer, sizeof(buffer), "NPC GERADO COM SUCESSO");
        } else {
            std::snprintf(buffer, sizeof(buffer), "NPCS GERADOS: +%d", delta);
        }
        ShowWordsHintText(buffer, 2.2f);
    }
    lastTrackedPlayers = trackedPlayers;

    char targetText[64] = {0};
    if (GetTargetKindText(targetText, sizeof(targetText))) {
        if (std::strncmp(targetText, lastTargetText, sizeof(lastTargetText) - 1) != 0) {
            ShowWordsHintText(targetText, 1.8f);
            std::strncpy(lastTargetText, targetText, sizeof(lastTargetText) - 1);
            lastTargetText[sizeof(lastTargetText) - 1] = '\0';
        }
    } else if (lastTargetText[0] != '\0') {
        lastTargetText[0] = '\0';
    }
}

bool CanUseMiniMapEnemyEsp() {
    if (!IsProbablyValidPtr(GetMiniMapIconCtrlInstance())) return false;

    void* enemies[8] = {};
    int enemyCount = CollectActiveEnemyBases(enemies, 8);
    for (int i = 0; i < enemyCount; ++i) {
        void* enemyBase = enemies[i];
        if (!IsProbablyValidPtr(enemyBase)) continue;

        void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0xC); // PlayerBase.m_dPlayer
        if (!IsProbablyValidPtr(player)) continue;

        void* root = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x24); // Player.Root
        if (IsProbablyValidPtr(root)) return true;
    }

    return false;
}

void ClearMiniMapEnemyEsp() {
    void* iconCtrl = GetMiniMapIconCtrlInstance();
    if (!IsProbablyValidPtr(iconCtrl)) return;

    try {
        uintptr_t addrDestroyHint = getAbsoluteAddress(targetLibName, 0x41F774); // UI_MiniMap_IconCtrl.DestroyHint(MiniMapHintsType, Transform, bool)
        if (addrDestroyHint == 0) return;

        auto destroyHint = reinterpret_cast<DestroyMiniMapHintByTargetFunc>(addrDestroyHint);
        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        for (int i = 0; i < enemyCount; ++i) {
            void* enemyBase = enemies[i];
            if (!IsProbablyValidPtr(enemyBase)) continue;

            void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0xC);
            if (!IsProbablyValidPtr(player)) continue;

            void* root = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x24);
            if (!IsProbablyValidPtr(root)) continue;

            destroyHint(iconCtrl, 10, root, false, nullptr); // MiniMapHintsType.EnemyPos
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Falha ao limpar ESP do minimapa");
    }
}

void RefreshMiniMapEnemyEsp() {
    void* iconCtrl = GetMiniMapIconCtrlInstance();
    if (!IsProbablyValidPtr(iconCtrl)) return;

    try {
        uintptr_t addrMiniMapHintsCtor = getAbsoluteAddress(targetLibName, 0x26B194); // MiniMapHintsParam..ctor(MiniMapHintsType, Transform)
        uintptr_t addrGenerateHint = getAbsoluteAddress(targetLibName, 0x41E88C); // UI_MiniMap_IconCtrl.GenerateMiniMapHints
        uintptr_t addrDestroyHint = getAbsoluteAddress(targetLibName, 0x41F774); // UI_MiniMap_IconCtrl.DestroyHint(MiniMapHintsType, Transform, bool)
        if (addrMiniMapHintsCtor == 0 || addrGenerateHint == 0 || addrDestroyHint == 0) return;

        auto miniMapHintsCtor = reinterpret_cast<MiniMapHintsCtorByTargetFunc>(addrMiniMapHintsCtor);
        auto generateHint = reinterpret_cast<GenerateMiniMapHintsFunc>(addrGenerateHint);
        auto destroyHint = reinterpret_cast<DestroyMiniMapHintByTargetFunc>(addrDestroyHint);

        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);
        int shown = 0;
        for (int i = 0; i < enemyCount; ++i) {
            void* enemyBase = enemies[i];
            if (!IsProbablyValidPtr(enemyBase)) continue;

            void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(enemyBase) + 0xC); // PlayerBase.m_dPlayer
            if (!IsProbablyValidPtr(player)) continue;

            void* root = *reinterpret_cast<void**>(reinterpret_cast<char*>(player) + 0x24); // Player.Root
            if (!IsProbablyValidPtr(root)) continue;

            // MiniMapHintsParam is a managed object; instantiate using the class of an existing hint object path is unavailable,
            // so we allocate conservatively as plain storage only after verifying constructor address.
            void* param = malloc(0x40);
            if (!param) continue;
            memset(param, 0, 0x40);

            miniMapHintsCtor(param, 10, root, nullptr); // MiniMapHintsType.EnemyPos
            destroyHint(iconCtrl, 10, root, false, nullptr);
            generateHint(iconCtrl, param, nullptr);
            free(param);
            shown++;
        }

        if (shown > 0) {
            __android_log_print(ANDROID_LOG_INFO, "MOD_VISUAL", "ESP do minimapa atualizado em %d inimigos", shown);
        }
    } catch (...) {
        __android_log_print(ANDROID_LOG_ERROR, "MOD_VISUAL", "Falha protegida ao atualizar ESP do minimapa");
    }
}
