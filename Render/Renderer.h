#pragma once
#include "../Core/Common.h"
#include "../Game/World.h"

void DrawGame(HDC hdc, World& w);
void RenderPlayerView(World& w, int viewerIdx,
    int screen_left, int screen_right);