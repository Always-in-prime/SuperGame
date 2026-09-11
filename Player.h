#pragma once
#include "Common.h"

// Инициализация глобальных значений (вызывается один раз при старте)
void InitGame();

// Игровая логика
void TryMove(Player& p, double dx, double dy);
void TryShoot(Player& shooter, Player& target);
void RespawnPlayer(Player& p);
void UpdateGame();