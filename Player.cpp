#include "Player.h"
#include "Map.h"

// ---------- Глобальные значения ----------
double move_speed = 0.06;
double rot_speed = 0.04;
double FOV = PI / 3.0;

Player p1 = { 1.5, 1.5, 0.0, 0, 100, 0, 0, 0, 1.5, 1.5, 0.0 };
Player p2 = { 1.5, 2.5, 0.0, 0, 100, 0, 0, 0, 1.5, 2.5, 0.0 };

void InitGame() {
    // (на будущее — если понадобится что-то инициализировать при старте)
}

// ---------- Попытка выстрела ----------
void TryShoot(Player& shooter, Player& target) {
    if (shooter.deathTimer > 0) return;
    if (shooter.shootCooldown > 0) return;

    shooter.shootCooldown = SHOT_COOLDOWN;
    shooter.muzzleFlash = MUZZLE_TIME;

    double dirx = sin(shooter.angle);
    double diry = cos(shooter.angle);

    double to_tx = target.x - shooter.x;
    double to_ty = target.y - shooter.y;

    // Проекция цели на направление выстрела
    double dist_along = to_tx * dirx + to_ty * diry;
    if (dist_along <= 0.2) return;

    // Перпендикулярное расстояние от цели до луча
    double px = shooter.x + dirx * dist_along;
    double py = shooter.y + diry * dist_along;
    double miss = sqrt((target.x - px) * (target.x - px) + (target.y - py) * (target.y - py));
    if (miss > HIT_RADIUS) return;

    // Проверка стены между стрелком и целью
    for (double d = 0.15; d < dist_along; d += 0.1) {
        int cx = (int)(shooter.x + dirx * d);
        int cy = (int)(shooter.y + diry * d);
        if (cx < 0 || cx >= MAP_WIDTH || cy < 0 || cy >= MAP_HEIGHT) return;
        if (map[cy][cx] == '#') return;
    }

    // Попадание!
    if (target.deathTimer > 0) return; // уже мёртв — не добиваем
    target.hp -= SHOT_DAMAGE;
    if (target.hp <= 0) {
        target.hp = 0;
        target.deathTimer = RESPAWN_TIME;
        shooter.score++;
    }
}

// ---------- Коллизии ----------
void TryMove(Player& p, double dx, double dy) {
    if (p.deathTimer > 0) return;

    double new_x = p.x + dx;
    double new_y = p.y + dy;
    if (map[(int)p.y][(int)new_x] != '#') p.x = new_x;
    if (map[(int)new_y][(int)p.x] != '#') p.y = new_y;
}

// ---------- Респавн ----------
void RespawnPlayer(Player& p) {
    p.x = p.startX;
    p.y = p.startY;
    p.angle = p.startAngle;
    p.hp = 100;
    p.deathTimer = 0;
    p.shootCooldown = 0;
    p.muzzleFlash = 0;
}

// ---------- Игровая логика ----------
void UpdateGame() {
    // Кулдауны и таймеры
    Player* allPlayers[2] = { &p1, &p2 };
    for (int i = 0; i < 2; i++) {
        Player* pl = allPlayers[i];
        if (pl->shootCooldown > 0) pl->shootCooldown--;
        if (pl->muzzleFlash > 0)   pl->muzzleFlash--;
        if (pl->deathTimer > 0) {
            pl->deathTimer--;
            if (pl->deathTimer == 0) RespawnPlayer(*pl);
        }
    }

    // --- Игрок 1 ---
    if (GetAsyncKeyState('W') & 0x8000) TryMove(p1, sin(p1.angle) * move_speed, cos(p1.angle) * move_speed);
    if (GetAsyncKeyState('S') & 0x8000) TryMove(p1, -sin(p1.angle) * move_speed, -cos(p1.angle) * move_speed);
    if (GetAsyncKeyState('A') & 0x8000) p1.angle -= rot_speed;
    if (GetAsyncKeyState('D') & 0x8000) p1.angle += rot_speed;
    if (GetAsyncKeyState(VK_SPACE) & 0x8000) TryShoot(p1, p2);

    // --- Игрок 2 ---
    if (GetAsyncKeyState(VK_UP) & 0x8000) TryMove(p2, sin(p2.angle) * move_speed, cos(p2.angle) * move_speed);
    if (GetAsyncKeyState(VK_DOWN) & 0x8000) TryMove(p2, -sin(p2.angle) * move_speed, -cos(p2.angle) * move_speed);
    if (GetAsyncKeyState(VK_LEFT) & 0x8000) p2.angle -= rot_speed;
    if (GetAsyncKeyState(VK_RIGHT) & 0x8000) p2.angle += rot_speed;
    if (GetAsyncKeyState(VK_RCONTROL) & 0x8000) TryShoot(p2, p1);
}