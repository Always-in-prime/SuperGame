#include "Player.h"
#include "Map.h"
#include <math.h>
#include "Hitbox.h"
#include "CharacterModel.h"

double move_speed = 4.5;
double rot_speed = 3.2;
double FOV = 3.14159265 / 3.0;

Player p1{};
Player p2{};

bool  g_roundEnding = false;
float g_roundEndTimer = 0.0f;
int   g_lastWinner = -1;

// ---------------------------------------------------------------------
//  Хелперы
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

static void PickTwoSpawns(int& s1, int& s2) {
    int n = GetSpawnCount();
    s1 = 0;
    s2 = (n > 1) ? 1 : 0;
    double best = -1.0;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double x1, y1, x2, y2;
            GetSpawnPoint(i, x1, y1);
            GetSpawnPoint(j, x2, y2);
            double dx = x2 - x1, dy = y2 - y1;
            double d = dx * dx + dy * dy;
            if (d > best) { best = d; s1 = i; s2 = j; }
        }
    }
}

static void ResetRoundPositions() {
    int s1, s2;
    PickTwoSpawns(s1, s2);
    double x1, y1, x2, y2;
    GetSpawnPoint(s1, x1, y1);
    GetSpawnPoint(s2, x2, y2);
    double a1 = atan2(x2 - x1, y2 - y1);
    double a2 = atan2(x1 - x2, y1 - y2);
    SetStart(p1, x1, y1, a1);
    SetStart(p2, x2, y2, a2);
}

// ---------------------------------------------------------------------
//  Инициализация / регенерация
// ---------------------------------------------------------------------
void InitGame() {
    GenerateMap();
    g_roundEnding = false;
    g_roundEndTimer = 0.0f;
    g_lastWinner = -1;
    p1.score = 0;
    p2.score = 0;
    ResetRoundPositions();
}

void RegenerateArena() {
    GenerateMap();
    ResetRoundPositions();
}

// ---------------------------------------------------------------------
//  Интерполяция
// ---------------------------------------------------------------------
void SavePrevStates() {
    auto save = [](Player& p) {
        p.prevX = p.x;
        p.prevY = p.y;
        p.prevAngle = p.angle;
    };
    save(p1);
    save(p2);
}

static double LerpAngle(double a, double b, double t) {
    double d = b - a;
    while (d > PI) d -= 2.0 * PI;
    while (d < -PI) d += 2.0 * PI;
    return a + d * t;
}

void InterpolateRenderStates(float alpha) {
    auto go = [alpha](Player& p) {
        p.renderX = p.prevX + (p.x - p.prevX) * alpha;
        p.renderY = p.prevY + (p.y - p.prevY) * alpha;
        p.renderAngle = LerpAngle(p.prevAngle, p.angle, alpha);
    };
    go(p1);
    go(p2);
}

// ---------------------------------------------------------------------
//  Коллизия
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

void TryMove(Player& p, double dx, double dy) {
    if (g_roundEnding) return;
    if (p.deathTimer > 0) return;
    if (IsWalkable(p.x + dx, p.y, PLAYER_RADIUS)) p.x += dx;
    if (IsWalkable(p.x, p.y + dy, PLAYER_RADIUS)) p.y += dy;
}

// ---------------------------------------------------------------------
//  Стрельба
// ---------------------------------------------------------------------
void TryShoot(Player& shooter, Player& target) {
    if (g_roundEnding) return;
    if (shooter.deathTimer > 0) return;
    if (shooter.shootCooldown > 0) return;

    shooter.shootCooldown = SHOT_COOLDOWN;
    shooter.muzzleFlash = MUZZLE_TIME;

    if (shooter.shakeMag < SHAKE_ON_SHOOT) shooter.shakeMag = SHAKE_ON_SHOOT;

    double dirx = sin(shooter.angle);
    double diry = cos(shooter.angle);

    // ---- 1. Луч против единой модели противника ----
    HitInfo hit = RaycastCharacter(shooter.x, shooter.y, dirx, diry,
        target.x, target.y, target.angle);
    if (!hit.hit) return;

    // ---- 2. Стены не должны перекрывать путь до попадания ----
    for (double d = 0.15; d < hit.distance - 0.05; d += 0.1) {
        int cx = (int)(shooter.x + dirx * d);
        int cy = (int)(shooter.y + diry * d);
        if (cx < 0 || cx >= MAP_WIDTH || cy < 0 || cy >= MAP_HEIGHT) return;
        if (map[cy][cx] == '#') return;
    }

    if (target.deathTimer > 0) return;

    // ---- 3. Урон с учётом множителя части тела ----
    double mult = PLAYER_PARTS[hit.partIdx].damageMult;
    int    dmg = (int)(SHOT_DAMAGE * mult);
    if (dmg < 1) dmg = 1;

    target.hp -= dmg;

    shooter.hitMarkerTimer = HITMARKER_TIME;
    target.damageFlashTimer = DAMAGE_FLASH_TIME;
    if (target.shakeMag < SHAKE_ON_HIT) target.shakeMag = SHAKE_ON_HIT;

    if (target.hp <= 0) {
        target.hp = 0;
        target.deathTimer = RESPAWN_TIME;
        shooter.score++;

        g_roundEnding = true;
        g_roundEndTimer = ROUND_END_DELAY;
        g_lastWinner = (&shooter == &p1) ? 0 : 1;

        shooter.killConfirmTimer = KILL_CONFIRM_TIME;
        target.deathNotifyTimer = DEATH_NOTIFY_TIME;
        if (target.shakeMag < SHAKE_ON_HIT * 1.5f)
            target.shakeMag = SHAKE_ON_HIT * 1.5f;
    }
}

void RespawnPlayer(Player& p) {
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

// ---------------------------------------------------------------------
//  Один шаг физики
// ---------------------------------------------------------------------
void StepPhysics(float dt) {
    // ---- Feedback таймеры тикают ВСЕГДА (в т.ч. во время конца раунда) ----
    Player* all[2] = { &p1, &p2 };
    for (int i = 0; i < 2; ++i) {
        Player* pl = all[i];
        if (pl->hitMarkerTimer > 0.0f) pl->hitMarkerTimer -= dt;
        if (pl->damageFlashTimer > 0.0f) pl->damageFlashTimer -= dt;
        if (pl->killConfirmTimer > 0.0f) pl->killConfirmTimer -= dt;
        if (pl->deathNotifyTimer > 0.0f) pl->deathNotifyTimer -= dt;
        if (pl->shakeMag > 0.0f) {
            pl->shakeMag -= SHAKE_DECAY * dt;
            if (pl->shakeMag < 0.0f) pl->shakeMag = 0.0f;
        }
    }

    // ----- Пауза между раундами -----
    if (g_roundEnding) {
        if (p1.muzzleFlash > 0) p1.muzzleFlash -= dt;
        if (p2.muzzleFlash > 0) p2.muzzleFlash -= dt;

        g_roundEndTimer -= dt;
        if (g_roundEndTimer <= 0.0f) {
            g_roundEnding = false;
            g_roundEndTimer = 0.0f;
            RegenerateArena();
            g_lastWinner = -1;
        }
        return;
    }

    // ----- Обычные таймеры -----
    for (int i = 0; i < 2; ++i) {
        Player* pl = all[i];
        if (pl->shootCooldown > 0) pl->shootCooldown -= dt;
        if (pl->muzzleFlash > 0) pl->muzzleFlash -= dt;
        if (pl->deathTimer > 0) {
            pl->deathTimer -= dt;
            if (pl->deathTimer <= 0) RespawnPlayer(*pl);
        }
    }

    // ----- Управление P1 -----
    double p1x0 = p1.x, p1y0 = p1.y;
    if (GetAsyncKeyState('W') & 0x8000)
        TryMove(p1, sin(p1.angle) * move_speed * dt, cos(p1.angle) * move_speed * dt);
    if (GetAsyncKeyState('S') & 0x8000)
        TryMove(p1, -sin(p1.angle) * move_speed * dt, -cos(p1.angle) * move_speed * dt);
    if (GetAsyncKeyState('A') & 0x8000) p1.angle -= rot_speed * dt;
    if (GetAsyncKeyState('D') & 0x8000) p1.angle += rot_speed * dt;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) TryShoot(p1, p2);
    {
        double mdx = p1.x - p1x0, mdy = p1.y - p1y0;
        double moved = sqrt(mdx * mdx + mdy * mdy);
        p1.walkPhase += (float)(moved * 9.0);
        while (p1.walkPhase > 2.0f * (float)PI) p1.walkPhase -= 2.0f * (float)PI;
    }

    // ----- Управление P2 -----
    double p2x0 = p2.x, p2y0 = p2.y;
    if (GetAsyncKeyState(VK_UP) & 0x8000)
        TryMove(p2, sin(p2.angle) * move_speed * dt, cos(p2.angle) * move_speed * dt);
    if (GetAsyncKeyState(VK_DOWN) & 0x8000)
        TryMove(p2, -sin(p2.angle) * move_speed * dt, -cos(p2.angle) * move_speed * dt);
    if (GetAsyncKeyState(VK_LEFT) & 0x8000)  p2.angle -= rot_speed * dt;
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) p2.angle += rot_speed * dt;
    if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) TryShoot(p2, p1);
    {
        double mdx = p2.x - p2x0, mdy = p2.y - p2y0;
        double moved = sqrt(mdx * mdx + mdy * mdy);
        p2.walkPhase += (float)(moved * 9.0);
        while (p2.walkPhase > 2.0f * (float)PI) p2.walkPhase -= 2.0f * (float)PI;
    }
}