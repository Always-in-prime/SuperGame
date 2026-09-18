#include "Spawner.h"
#include "World.h"
#include "Map.h"
#include <math.h>

// ---------------------------------------------------------------------
static void SetStart(Player& p, double x, double y, double a) {
    p.startX = x; p.startY = y; p.startAngle = a;
    p.x = p.prevX = p.renderX = x;
    p.y = p.prevY = p.renderY = y;
    p.angle = p.prevAngle = p.renderAngle = a;
    p.hp = 100;
    p.deathTimer = 0.0f;
    p.shootCooldown = 0.0f;
    p.muzzleFlash = 0.0f;
    p.walkPhase = 0.0f;
    p.hitMarkerTimer = 0.0f;
    p.damageFlashTimer = 0.0f;
    p.killConfirmTimer = 0.0f;
    p.deathNotifyTimer = 0.0f;
    p.shakeMag = 0.0f;
}

// ---------------------------------------------------------------------
void Spawner_PickTwoFarthest(int& outS1, int& outS2) {
    int n = GetSpawnCount();
    outS1 = 0;
    outS2 = (n > 1) ? 1 : 0;

    double best = -1.0;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double x1, y1, x2, y2;
            GetSpawnPoint(i, x1, y1);
            GetSpawnPoint(j, x2, y2);
            double dx = x2 - x1, dy = y2 - y1;
            double d = dx * dx + dy * dy;
            if (d > best) {
                best = d;
                outS1 = i;
                outS2 = j;
            }
        }
    }
}

// ---------------------------------------------------------------------
void Spawner_PlacePlayers(World& w) {
    int s1, s2;
    Spawner_PickTwoFarthest(s1, s2);

    double x1, y1, x2, y2;
    GetSpawnPoint(s1, x1, y1);
    GetSpawnPoint(s2, x2, y2);

    double a1 = atan2(x2 - x1, y2 - y1);
    double a2 = atan2(x1 - x2, y1 - y2);

    SetStart(w.players[0], x1, y1, a1);
    SetStart(w.players[1], x2, y2, a2);
}