#pragma once
#include "../Game/Player.h"

void RenderOtherPlayer(const Player& viewer, const Player& target,
    int screen_left, int view_width, int half_height,
    const double* zbuffer);