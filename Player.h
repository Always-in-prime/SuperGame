#pragma once
#include "Common.h"

// Таймеры в секундах — не зависят от FPS
#define SHOT_COOLDOWN 0.5f
#define MUZZLE_TIME   0.12f
#define RESPAWN_TIME  3.0f
#define HIT_RADIUS    0.4
#define SHOT_DAMAGE   50

struct Player {
    double x, y, angle;
    int    score;
    int    hp;
    float  deathTimer;
    float  shootCooldown;
    float  muzzleFlash;
    double startX, startY, startAngle;
};

extern Player p1, p2;
extern double FOV;
extern double move_speed;
extern double rot_speed;

void InitGame();
void UpdateGame(float dt);
void TryMove(Player& p, double dx, double dy);
void TryShoot(Player& shooter, Player& target);
void RespawnPlayer(Player& p);