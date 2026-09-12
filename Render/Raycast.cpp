#include "Raycast.h"
#include "Map.h"
#include "Texture.h"
#include <math.h>

RayHit CastRay(double px, double py, double dirX, double dirY) {
    int mapX = (int)floor(px);
    int mapY = (int)floor(py);

    double ddx = (dirX == 0.0) ? 1e30 : fabs(1.0 / dirX);
    double ddy = (dirY == 0.0) ? 1e30 : fabs(1.0 / dirY);

    int stepX, stepY;
    double sdx, sdy;
    if (dirX < 0) { stepX = -1; sdx = (px - mapX) * ddx; }
    else { stepX = 1; sdx = (mapX + 1.0 - px) * ddx; }
    if (dirY < 0) { stepY = -1; sdy = (py - mapY) * ddy; }
    else { stepY = 1; sdy = (mapY + 1.0 - py) * ddy; }

    int side = 0;
    for (int i = 0; i < 128; ++i) {
        if (sdx < sdy) { sdx += ddx; mapX += stepX; side = 0; }
        else { sdy += ddy; mapY += stepY; side = 1; }
        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) break;
        if (map[mapY][mapX] == '#') break;
    }

    double dist = (side == 0) ? (sdx - ddx) : (sdy - ddy);
    if (dist < 0.0001) dist = 0.0001;

    double wallX;
    if (side == 0) wallX = py + dist * dirY;
    else           wallX = px + dist * dirX;
    wallX -= floor(wallX);

    if (side == 0 && dirX > 0) wallX = 1.0 - wallX;
    if (side == 1 && dirY < 0) wallX = 1.0 - wallX;

    int tcx = mapX, tcy = mapY;
    if (tcx < 0) tcx = 0; if (tcx >= MAP_WIDTH)  tcx = MAP_WIDTH - 1;
    if (tcy < 0) tcy = 0; if (tcy >= MAP_HEIGHT) tcy = MAP_HEIGHT - 1;

    RayHit h;
    h.dist = dist;
    h.wallX = wallX;
    h.side = side;
    h.texId = GetTileTexture(tcx, tcy);
    return h;
}