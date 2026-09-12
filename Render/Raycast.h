#pragma once
#include "../Core/Common.h"

struct RayHit {
    double dist;
    double wallX;
    int    side;
    int    texId;
};

RayHit CastRay(double px, double py, double dirX, double dirY);