#pragma once

// =============================================================================
// NPC Walkway — Fileira de NPCs seguindo o jogador
// =============================================================================
// Faz NPCs hostis formarem uma fila atrás do jogador, marchando em formação.
// Útil para agrupar inimigos para massacre em massa ou RP de xerife.
// =============================================================================

// Variáveis de controle (extern — definidas em NpcWalkway.cpp)
extern bool npcWalkway;
extern float npcWalkwaySpacing;   // distância entre cada NPC na fila (metros)
extern int npcWalkwayMaxNpcs;     // máximo de NPCs na formação
extern float npcWalkwayRadius;    // raio de coleta de NPCs ao redor do player

// Feature ID para RemoteFeatures
static constexpr int kFeatureNpcWalkway = 100;

// Inicializa/desliga o estado do walkway (chamado quando toggle muda)
void SetNpcWalkwayState(bool enabled);

// Chamado a cada frame pelo ProcessGameplayFrame
void RunNpcWalkwayFrame();
