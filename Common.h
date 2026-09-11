#pragma once
#include <windows.h>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cstdlib>

// ---------- ќбщие константы ----------
const double PI = 3.1415926535;

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

const int MAP_WIDTH = 12;
const int MAP_HEIGHT = 12;

const int SHOT_DAMAGE = 25;
const int SHOT_COOLDOWN = 30;   // ~0.5 сек при 60 FPS
const int MUZZLE_TIME = 6;
const int RESPAWN_TIME = 120;  // ~2 сек
const double HIT_RADIUS = 0.35;

// ---------- √лобальные настройки (определены в Player.cpp) ----------
extern double move_speed;
extern double rot_speed;
extern double FOV;

// ---------- »грок ----------
struct Player {
    double x, y;
    double angle;
    int score;
    int hp;
    int shootCooldown;   // кадров до следующего выстрела
    int muzzleFlash;     // кадров вспышки
    int deathTimer;      // >0 Ч мЄртв, отсчЄт до респавна
    double startX, startY, startAngle;
};

extern Player p1;
extern Player p2;