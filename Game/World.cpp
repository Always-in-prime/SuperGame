#include "World.h"
#include "Map.h"
#include "Spawner.h"
#include "Combat.h"
#include "MathUtils.h"
#include <math.h>

// ---------------------------------------------------------------------
//  Lifecycle
// ---------------------------------------------------------------------
void World_Init(World& w, const GameSettings& s) {
    w.settings = s;

    GenerateMap();

    Match_Init(w.match);
    w.match.scoreLimit = s.matchScore;

    w.players[0].score = 0;
    w.players[1].score = 0;

    Spawner_PlacePlayers(w);
}

void World_Regenerate(World& w) {
    GenerateMap();
    Spawner_PlacePlayers(w);
}

// ---------------------------------------------------------------------
//  Интерполяция
// ---------------------------------------------------------------------
static void InterpolateOne(Player& p, float alpha) {
    p.renderX = p.prevX + (p.x - p.prevX) * alpha;
    p.renderY = p.prevY + (p.y - p.prevY) * alpha;

    double d = p.angle - p.prevAngle;
    while (d > PI) d -= 2.0 * PI;
    while (d < -PI) d += 2.0 * PI;
    p.renderAngle = p.prevAngle + d * alpha;
}

void World_SavePrev(World& w) {
    for (int i = 0; i < 2; ++i) {
        Player& p = w.players[i];
        p.prevX = p.x;
        p.prevY = p.y;
        p.prevAngle = p.angle;
    }
}

void World_Interpolate(World& w, float alpha) {
    InterpolateOne(w.players[0], alpha);
    InterpolateOne(w.players[1], alpha);
}

// ---------------------------------------------------------------------
//  Физика
// ---------------------------------------------------------------------
static bool IsWalkable(double x, double y, double r) {
    int x0 = (int)floor(x - r);
    int x1 = (int)floor(x + r);
    int y0 = (int)floor(y - r);
    int y1 = (int)floor(y + r);

    for (int cy = y0; cy <= y1; ++cy) {
        for (int cx = x0; cx <= x1; ++cx) {
            if (cx < 0 || cx >= MAP_WIDTH || cy < 0 || cy >= MAP_HEIGHT) return false;
            if (map[cy][cx] != '#') continue;
            double cxp = x < cx ? cx : (x > cx + 1 ? cx + 1 : x);
            double cyp = y < cy ? cy : (y > cy + 1 ? cy + 1 : y);
            double dx = x - cxp;
            double dy = y - cyp;
            if (dx * dx + dy * dy < r * r) return false;
        }
    }
    return true;
}

static void TryMove(World& w, Player& p, double dx, double dy) {
    if (w.match.roundEnding) return;
    if (p.deathTimer > 0) return;
    if (IsWalkable(p.x + dx, p.y, PLAYER_RADIUS)) p.x += dx;
    if (IsWalkable(p.x, p.y + dy, PLAYER_RADIUS)) p.y += dy;
}

static void RespawnPlayer(Player& p) {
    p.x = p.prevX = p.renderX = p.startX;
    p.y = p.prevY = p.renderY = p.startY;
    p.angle = p.prevAngle = p.renderAngle = p.startAngle;
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

static void ApplyInput(World& w, int idx, const PlayerInput& in, float dt) {
    Player& p = w.players[idx];

    if (w.match.roundEnding) return;
    if (p.deathTimer > 0) return;

    double x0 = p.x, y0 = p.y;

    if (in.forward != 0.0) {
        double targetVx = sin(p.angle) * in.forward * w.settings.moveSpeed;
        double targetVy = cos(p.angle) * in.forward * w.settings.moveSpeed;
        const double ACCEL_RATE = 25.0;
        p.vx = Damp(p.vx, targetVx, ACCEL_RATE, dt);
        p.vy = Damp(p.vy, targetVy, ACCEL_RATE, dt);

        TryMove(w, p, p.vx * dt, p.vy * dt);
    }

    if (in.turn != 0.0) {
        p.angle += in.turn * w.settings.rotSpeed * dt;
    }

    if (in.shoot || in.shootPressed) {
        int targetIdx = 1 - idx;
        Combat_TryShoot(w, idx, targetIdx);
    }

    double mdx = p.x - x0, mdy = p.y - y0;
    double moved = sqrt(mdx * mdx + mdy * mdy);
    p.walkPhase += (float)(moved * 9.0);
    while (p.walkPhase > 2.0f * (float)PI)
        p.walkPhase -= 2.0f * (float)PI;
}

// ---------------------------------------------------------------------
void World_Step(World& w, float dt, const PlayerInput inputs[2]) {
    for (int i = 0; i < 2; ++i) {
        Player& p = w.players[i];
        if (p.hitMarkerTimer > 0.0f) p.hitMarkerTimer -= dt;
        if (p.damageFlashTimer > 0.0f) p.damageFlashTimer -= dt;
        if (p.killConfirmTimer > 0.0f) p.killConfirmTimer -= dt;
        if (p.deathNotifyTimer > 0.0f) p.deathNotifyTimer -= dt;
        if (p.shakeMag > 0.0f) {
            p.shakeMag -= SHAKE_DECAY * dt;
            if (p.shakeMag < 0.0f) p.shakeMag = 0.0f;
        }
    }

    if (w.match.roundEnding) {
        for (int i = 0; i < 2; ++i) {
            if (w.players[i].muzzleFlash > 0)
                w.players[i].muzzleFlash -= dt;
        }

        if (Match_Update(w.match, w, dt)) {
            World_Regenerate(w);
        }
        return;
    }

    for (int i = 0; i < 2; ++i) {
        Player& p = w.players[i];
        if (p.shootCooldown > 0) p.shootCooldown -= dt;
        if (p.muzzleFlash > 0) p.muzzleFlash -= dt;
        if (p.deathTimer > 0) {
            p.deathTimer -= dt;
            if (p.deathTimer <= 0) RespawnPlayer(p);
        }
    }

    ApplyInput(w, 0, inputs[0], dt);
    ApplyInput(w, 1, inputs[1], dt);
}