#pragma once
#include "Common.h"
#include "Player.h"

void DrawGame(HDC hdc);
void RenderPlayerView(Player& player, Player& other,
    int screen_left, int screen_right);