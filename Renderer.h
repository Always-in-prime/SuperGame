#pragma once
#include "Common.h"

// Отрисовка вида одного игрока в заданную половину экрана
void RenderPlayerView(HDC memDC, Player& player, Player& other,
    int screen_left, int screen_right);

// Главная отрисовка (оба вида + разделитель + подсказки)
void DrawGame(HDC hdc);