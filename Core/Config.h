#pragma once

// ============================================================
//  Компилируемые константы игры.
//  Меняются только пересборкой. Для рантайм-настроек — Core/Settings.h.
// ============================================================

// ---- Раунд ----
#define SHOT_COOLDOWN       0.5f
#define MUZZLE_TIME         0.12f
#define RESPAWN_TIME        3.0f
#define ROUND_END_DELAY     1.5f

// ---- Попадание ----
#define SHOT_DAMAGE         50
#define PLAYER_RADIUS       0.25

// ---- Feedback таймеры ----
#define HITMARKER_TIME      0.15f
#define DAMAGE_FLASH_TIME   0.25f
#define KILL_CONFIRM_TIME   1.0f
#define DEATH_NOTIFY_TIME   1.0f

// ---- Screen shake ----
#define SHAKE_ON_SHOOT      2.5f
#define SHAKE_ON_HIT        6.0f
#define SHAKE_DECAY         20.0f

// ---- Тайминг ----
#define FIXED_DT            (1.0f / 120.0f)
#define MAX_FRAME_TIME      0.25f
#define MAX_PHYSICS_STEPS   8

// ---- Геометрия мира ----
// Мир: Y=0 — пол, Y=WALL_HEIGHT — потолок.
// Камера (глаза игрока) на EYE_HEIGHT от пола.
// Персонаж стоит на полу, его глаза тоже примерно на EYE_HEIGHT.
#define WALL_HEIGHT   1.0
#define EYE_HEIGHT    0.7