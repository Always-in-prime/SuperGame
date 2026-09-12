#pragma once
#include "Common.h"

// ============================================================
//  Рантайм-настройки игры.
//  В отличие от Config.h, эти параметры можно менять без пересборки —
//  они загружаются из settings.ini и (в будущем) крутятся в меню.
// ============================================================

struct GameSettings {
    // ---- Управление ----
    float rotSpeed = 3.2f;    // скорость поворота (рад/сек)
    float moveSpeed = 4.5f;    // скорость передвижения (м/сек)

    // ---- Визуал ----
    bool  screenShake = true;  // тряска камеры при выстреле/уроне
    bool  headBob = true;  // покачивание камеры при ходьбе

    // ---- Матч ----
    int   matchScore = 10;    // до какого счёта идёт матч (0 = бесконечно)

    // ---- Отладка ----
    bool  showFps = false;
};

// Путь к ini задаётся как "settings.ini" — файл рядом с exe.
// Load: если файла нет или он повреждён, возвращает дефолты.
void Settings_Load(GameSettings& s, const char* iniPath = "settings.ini");
void Settings_Save(const GameSettings& s, const char* iniPath = "settings.ini");

// Полный путь к settings.ini рядом с exe. Заполняет outBuf.
// Возвращает outBuf. Нужен, чтобы Load/Save работали из любой директории.
const char* Settings_GetDefaultPath(char* outBuf, int bufSize);