#pragma once
#include "Common.h"
#include "Config.h"

struct Player {
    double x, y, angle;
    double prevX, prevY, prevAngle;
    double renderX, renderY, renderAngle;

    int   score;
    int   hp;
    float deathTimer;
    float shootCooldown;
    float muzzleFlash;

    float walkPhase;

    // ---- Feedback ----
    float hitMarkerTimer;
    float damageFlashTimer;
    float killConfirmTimer;
    float deathNotifyTimer;
    float shakeMag;

    double startX, startY, startAngle;
};

extern Player p1, p2;
extern double FOV;
extern double move_speed;
extern double rot_speed;

extern bool  g_roundEnding;
extern float g_roundEndTimer;
extern int   g_lastWinner;

void InitGame();
void RegenerateArena();

void SavePrevStates();
void StepPhysics(float dt);
void InterpolateRenderStates(float alpha);

void TryMove(Player& p, double dx, double dy);
void TryShoot(Player& shooter, Player& target);
void RespawnPlayer(Player& p);