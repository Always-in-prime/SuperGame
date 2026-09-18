#include "Combat.h"
#include "World.h"
#include "Match.h"
#include "Map.h"
#include "Hitbox.h"
#include "CharacterModel.h"
#include "../Core/Mesh.h"
#include <cmath>

// ---------------------------------------------------------------------
void Combat_TryShoot(World& w, int shooterIdx, int targetIdx) {
    if (w.match.roundEnding) return;

    Player& shooter = w.players[shooterIdx];
    Player& target = w.players[targetIdx];

    if (shooter.deathTimer > 0) return;
    if (shooter.shootCooldown > 0) return;

    // ---- Кулдаун и вспышка ----
    shooter.shootCooldown = SHOT_COOLDOWN;
    shooter.muzzleFlash = MUZZLE_TIME;

    if (shooter.shakeMag < SHAKE_ON_SHOOT)
        shooter.shakeMag = SHAKE_ON_SHOOT;

    // ---- Луч в мировых координатах ----
    double dirx = sin(shooter.angle);
    double diry = cos(shooter.angle);

    // ---- Собираем меш цели в её мировых координатах ----
    Mesh targetMesh;
    BuildCharacterMesh(targetMesh, target);

    // ---- Пересечение луча с мешем ----
    HitInfo hit = RaycastCharacterMesh(targetMesh,
        shooter.x, shooter.y,
        dirx, diry);
    if (!hit.hit) return;

    // ---- Стена между стрелком и точкой попадания ----
    for (double d = 0.15; d < hit.distance - 0.05; d += 0.1) {
        int cx = (int)(shooter.x + dirx * d);
        int cy = (int)(shooter.y + diry * d);
        if (cx < 0 || cx >= MAP_WIDTH || cy < 0 || cy >= MAP_HEIGHT) return;
        if (map[cy][cx] == '#') return;
    }

    if (target.deathTimer > 0) return;

    // ---- Урон с множителем части тела ----
    double mult = hit.damageMult;
    int    dmg = (int)(SHOT_DAMAGE * mult);
    if (dmg < 1) dmg = 1;

    Combat_ApplyDamage(w, targetIdx, dmg);

    // ---- Фидбек ----
    shooter.hitMarkerTimer = HITMARKER_TIME;
}

// ---------------------------------------------------------------------
void Combat_ApplyDamage(World& w, int targetIdx, int damage) {
    Player& target = w.players[targetIdx];

    if (target.deathTimer > 0) return;

    target.hp -= damage;
    target.damageFlashTimer = DAMAGE_FLASH_TIME;
    if (target.shakeMag < SHAKE_ON_HIT)
        target.shakeMag = SHAKE_ON_HIT;

    if (target.hp > 0) return;

    // ---- Смерть ----
    target.hp = 0;
    target.deathTimer = RESPAWN_TIME;

    int shooterIdx = 1 - targetIdx;
    Player& shooter = w.players[shooterIdx];
    shooter.score++;

    Match_EndRound(w.match, shooterIdx);

    shooter.killConfirmTimer = KILL_CONFIRM_TIME;
    target.deathNotifyTimer = DEATH_NOTIFY_TIME;
    if (target.shakeMag < SHAKE_ON_HIT * 1.5f)
        target.shakeMag = SHAKE_ON_HIT * 1.5f;
}