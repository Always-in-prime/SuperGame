#pragma once
#include "Common.h"
#include "Config.h"

struct Player {
    double x, y, angle;
    double vx, vy;
    double prevX, prevY, prevAngle;
    double renderX, renderY, renderAngle;

    int   score;
    int   hp;
    float deathTimer;
    float shootCooldown;
    float muzzleFlash;

    float walkPhase;

    float hitMarkerTimer;
    float damageFlashTimer;
    float killConfirmTimer;
    float deathNotifyTimer;
    float shakeMag;

    double startX, startY, startAngle;
};

// Глобальные настройки рендера (FOV — постоянный)
extern double FOV;