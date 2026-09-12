#pragma once
#include "Common.h"
#include "Config.h"

struct World;

// ============================================================
//  Логика раунда/матча.
//  Хранит состояние паузы между раундами и решает, когда
//  заканчивается матч.
// ============================================================

struct Match {
    bool  roundEnding;
    float roundEndTimer;
    int   lastWinner;   // 0 = P1, 1 = P2, -1 = нет

    // Счёт до победы (пока константа, позже — из GameSettings)
    int   scoreLimit;   // 0 = бесконечно
};

void Match_Init(Match& m);

// Обновить таймеры. Если пауза закончилась — вернёт true,
// и World должен регенерировать арену.
bool Match_Update(Match& m, World& w, float dt);

// Отметить конец раунда победой указанного игрока.
void Match_EndRound(Match& m, int winnerIndex);

// Проверить: кто-то достиг scoreLimit? -1 если нет, иначе индекс игрока.
int Match_CheckWin(const Match& m, const World& w);