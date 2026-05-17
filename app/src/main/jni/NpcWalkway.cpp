#include "NpcWalkway.h"
#include "Includes/Logger.h"
#include "Includes/obfuscate.h"
#include <cmath>

// Em vez de incluir Utils.h (que define funções globais causando duplicate symbols),
// declaramos apenas o que precisamos como extern (definido em Main.cpp via Utils.h)
typedef unsigned long DWORD;
DWORD getAbsoluteAddress(const char* libraryName, DWORD relativeAddr);

// IsProbablyValidPtr é static no Main.cpp — implementamos localmente
static bool IsProbablyValidPtr(void* ptr) {
    return ptr && (uintptr_t)ptr >= 0x10000000;
}

// Estrutura Vector3 (compatível com Main.cpp)
struct Vector3 {
    float x;
    float y;
    float z;
};

// =============================================================================
// NPC Walkway — Implementação
// =============================================================================
// Formação: fila indiana atrás do jogador, seguindo sua direção de movimento.
// Cada NPC recebe um destino calculado como:
//   pos = playerPos - forward * (spacing * (index + 1))
// O destino é aplicado via Player.SetNavMesDestination() ou
// escrevendo MoveParam.destPos diretamente.
// =============================================================================

bool npcWalkway = false;
float npcWalkwaySpacing = 2.5f;
int npcWalkwayMaxNpcs = 8;
float npcWalkwayRadius = 30.0f;

// Forward declarations de funções externas (definidas em Main.cpp)
extern void* GetGameCtrlInstance();
extern void* GetMyPlayerInstance();
extern bool GetPlayerWorldPosition(void* player, Vector3& outPos);
extern int CollectActiveEnemyBases(void** outEnemies, int maxEnemies);
extern void ShowWordsHintText(const char* text, float showTime);

// Offsets do dump.cs
#define targetLibName OBFUSCATE("libil2cpp.so")

// --- Tipos de função ---
typedef void (*PlayerSetNavMeshDestFunc)(void* thisPtr, Vector3 dest, void* method);
typedef Vector3 (*PlayerHeadingFunc)(void* thisPtr, void* method);

// --- Helpers ---
static Vector3 NormalizeXZ(const Vector3& v) {
    float len = sqrtf(v.x * v.x + v.z * v.z);
    if (len < 0.0001f) return {0.0f, 0.0f, 1.0f};
    return {v.x / len, v.y / len, v.z / len};
}

static void* GetPlayerHeadingFuncPtr() {
    uintptr_t addr = getAbsoluteAddress(targetLibName, 0x45DA48); // MyCtrlPlayer.GetHeading()
    if (addr == 0) return nullptr;
    return reinterpret_cast<void*>(addr);
}

static Vector3 GetPlayerForward(void* myPlayer) {
    Vector3 forward = {0.0f, 0.0f, 1.0f};
    
    // Tenta obter heading do MyCtrlPlayer
    void* gameCtrl = GetGameCtrlInstance();
    if (!IsProbablyValidPtr(gameCtrl)) return forward;
    
    void* myCtrlPlayer = *reinterpret_cast<void**>(reinterpret_cast<char*>(gameCtrl) + 0x10);
    if (!IsProbablyValidPtr(myCtrlPlayer)) return forward;
    
    uintptr_t addrHeading = getAbsoluteAddress(targetLibName, 0x45DA48);
    if (addrHeading != 0) {
        auto headingFunc = reinterpret_cast<PlayerHeadingFunc>(addrHeading);
        Vector3 heading = headingFunc(myCtrlPlayer, nullptr);
        Vector3 norm = NormalizeXZ(heading);
        if (fabsf(norm.x) > 0.001f || fabsf(norm.z) > 0.001f) {
            return norm;
        }
    }
    
    return forward;
}

static bool SetPlayerNavDestination(void* player, const Vector3& dest) {
    if (!IsProbablyValidPtr(player)) return false;
    
    uintptr_t addr = getAbsoluteAddress(targetLibName, 0x344844); // Player.SetNavMesDestination(Vector3)
    if (addr == 0) return false;
    
    auto func = reinterpret_cast<PlayerSetNavMeshDestFunc>(addr);
    func(player, dest, nullptr);
    return true;
}

static float Distance(const Vector3& a, const Vector3& b) {
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

static void* GetPlayerFromBase(void* playerBase) {
    if (!IsProbablyValidPtr(playerBase)) return nullptr;
    void* player = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0xC);
    return IsProbablyValidPtr(player) ? player : nullptr;
}

static bool IsEnemyBaseAlive(void* playerBase) {
    if (!IsProbablyValidPtr(playerBase)) return false;

    void* baseData = *reinterpret_cast<void**>(reinterpret_cast<char*>(playerBase) + 0x14);
    if (!IsProbablyValidPtr(baseData)) return false;

    void* property = *reinterpret_cast<void**>(reinterpret_cast<char*>(baseData) + 0x8);
    if (!IsProbablyValidPtr(property)) return false;

    int maxBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x10);
    int currentBlood = *reinterpret_cast<int*>(reinterpret_cast<char*>(property) + 0x14);
    return maxBlood > 0 && currentBlood > 0;
}

static bool IsTrackedNpc(void* playerBase, void* player);
static void ApplyWalkwayFormation(const Vector3& playerPos, const Vector3& forward);

// Rastreamento de quais NPCs já estão na formação. Mantemos a mesma lista
// enquanto os PlayerBase continuam vivos/validos; novas buscas só preenchem vagas.
static void* gWalkwayBases[32] = {};
static void* gWalkwayNpcs[32] = {};
static int gWalkwayNpcCount = 0;
static int gWalkwayFrameCounter = 0;

static bool IsTrackedNpc(void* playerBase, void* player) {
    for (int i = 0; i < gWalkwayNpcCount; i++) {
        if (gWalkwayBases[i] == playerBase || gWalkwayNpcs[i] == player) {
            return true;
        }
    }
    return false;
}

static void CompactTrackedNpcs(void* myPlayer) {
    int writeIndex = 0;
    for (int i = 0; i < gWalkwayNpcCount; i++) {
        void* playerBase = gWalkwayBases[i];
        void* player = gWalkwayNpcs[i];

        if (!IsEnemyBaseAlive(playerBase)) continue;

        void* livePlayer = GetPlayerFromBase(playerBase);
        if (!IsProbablyValidPtr(livePlayer) || livePlayer == myPlayer) continue;
        if (IsProbablyValidPtr(player) && livePlayer != player) continue;

        gWalkwayBases[writeIndex] = playerBase;
        gWalkwayNpcs[writeIndex] = livePlayer;
        writeIndex++;
    }

    for (int i = writeIndex; i < 32; i++) {
        gWalkwayBases[i] = nullptr;
        gWalkwayNpcs[i] = nullptr;
    }
    gWalkwayNpcCount = writeIndex;
}

static void AddNpcToWalkway(void* playerBase, void* player) {
    if (gWalkwayNpcCount >= 32 || gWalkwayNpcCount >= npcWalkwayMaxNpcs) return;
    if (!IsProbablyValidPtr(playerBase) || !IsProbablyValidPtr(player)) return;
    if (IsTrackedNpc(playerBase, player)) return;

    gWalkwayBases[gWalkwayNpcCount] = playerBase;
    gWalkwayNpcs[gWalkwayNpcCount] = player;
    gWalkwayNpcCount++;
}

static void ApplyWalkwayFormation(const Vector3& playerPos, const Vector3& forward) {
    for (int i = 0; i < gWalkwayNpcCount; i++) {
        void* npc = gWalkwayNpcs[i];
        if (!IsProbablyValidPtr(npc)) continue;

        Vector3 targetPos = playerPos;
        float offset = npcWalkwaySpacing * (i + 1);
        targetPos.x -= forward.x * offset;
        targetPos.z -= forward.z * offset;
        targetPos.y = playerPos.y;

        if ((i % 2) == 0) {
            targetPos.x += forward.z * 0.8f;
            targetPos.z -= forward.x * 0.8f;
        } else {
            targetPos.x -= forward.z * 0.8f;
            targetPos.z += forward.x * 0.8f;
        }

        SetPlayerNavDestination(npc, targetPos);
    }
}

void SetNpcWalkwayState(bool enabled) {
    npcWalkway = enabled;
    if (!enabled) {
        // Limpa rastreamento
        for (int i = 0; i < gWalkwayNpcCount; i++) {
            gWalkwayBases[i] = nullptr;
            gWalkwayNpcs[i] = nullptr;
        }
        gWalkwayNpcCount = 0;
        gWalkwayFrameCounter = 0;
        LOGI("NPC Walkway desativado — formação liberada");
    } else {
        for (int i = 0; i < 32; i++) {
            gWalkwayBases[i] = nullptr;
            gWalkwayNpcs[i] = nullptr;
        }
        gWalkwayNpcCount = 0;
        gWalkwayFrameCounter = 0;
        LOGI("NPC Walkway ativado — coletando NPCs...");
        ShowWordsHintText("NPC WALKWAY ON", 2.0f);
    }
}

void RunNpcWalkwayFrame() {
    if (!npcWalkway) return;
    
    // Throttle: atualiza a formação a cada 15 frames (~4x por segundo a 60fps)
    // Isso evita overhead excessivo e dá tempo do NavMesh processar
    gWalkwayFrameCounter++;
    if ((gWalkwayFrameCounter % 15) != 0) return;
    
    // Obtém o player
    void* myPlayer = GetMyPlayerInstance();
    if (!IsProbablyValidPtr(myPlayer)) return;
    
    Vector3 playerPos = {0.0f, 0.0f, 0.0f};
    if (!GetPlayerWorldPosition(myPlayer, playerPos)) return;
    
    // Direção forward do player (para onde ele está olhando)
    Vector3 forward = GetPlayerForward(myPlayer);

    CompactTrackedNpcs(myPlayer);
    
    // Recruta somente para preencher vagas. NPCs ja rastreados continuam na fila.
    if (gWalkwayNpcCount < npcWalkwayMaxNpcs) {
        void* enemies[128] = {};
        int enemyCount = CollectActiveEnemyBases(enemies, 128);

        for (int i = 0; i < enemyCount && gWalkwayNpcCount < npcWalkwayMaxNpcs; i++) {
            void* enemyBase = enemies[i];
            void* enemyPlayer = GetPlayerFromBase(enemyBase);
            if (!IsProbablyValidPtr(enemyPlayer) || enemyPlayer == myPlayer) continue;
            if (!IsEnemyBaseAlive(enemyBase)) continue;
            if (IsTrackedNpc(enemyBase, enemyPlayer)) continue;

            Vector3 enemyPos = {0.0f, 0.0f, 0.0f};
            if (!GetPlayerWorldPosition(enemyPlayer, enemyPos)) continue;

            if (Distance(enemyPos, playerPos) <= npcWalkwayRadius) {
                AddNpcToWalkway(enemyBase, enemyPlayer);
            }
        }
    }

    ApplyWalkwayFormation(playerPos, forward);
}
