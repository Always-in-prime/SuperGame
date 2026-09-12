#include "Hitbox.h"
#include <math.h>

// ---------------------------------------------------------------------
//  ѕересечение луча с вертикальным цилиндром, ось Z.
//  ¬озвращает t ближайшего положительного пересечени€ боковой
//  поверхности в диапазоне [zMin, zMax], либо -1 если мимо.
// ---------------------------------------------------------------------
static double RayCylXZ(double ox, double oy,
    double dx, double dy,
    double px, double py, double r,
    double zMin, double zMax,
    double maxT)
{
    double lx = ox - px;   // <-- исправлен знак
    double ly = oy - py;   // <-- исправлен знак

    double A = dx * dx + dy * dy;
    if (A < 1e-9) return -1.0;

    double B = 2.0 * (dx * lx + dy * ly);
    double Cc = lx * lx + ly * ly - r * r;
    double disc = B * B - 4.0 * A * Cc;
    if (disc < 0.0) return -1.0;

    double sq = sqrt(disc);
    double t0 = (-B - sq) / (2.0 * A);
    double t1 = (-B + sq) / (2.0 * A);

    if (t1 < 0.0) return -1.0;

    double t = (t0 > 0.0) ? t0 : t1;
    if (maxT > 0.0 && t > maxT) return -1.0;

    (void)zMin; (void)zMax;
    return t;
}

static double RaySphereXZ(double ox, double oy,
    double dx, double dy,
    double px, double py, double r,
    double maxT)
{
    double lx = ox - px;   // <-- исправлен знак
    double ly = oy - py;   // <-- исправлен знак

    double A = dx * dx + dy * dy;
    if (A < 1e-9) return -1.0;

    double B = 2.0 * (dx * lx + dy * ly);
    double Cc = lx * lx + ly * ly - r * r;
    double disc = B * B - 4.0 * A * Cc;
    if (disc < 0.0) return -1.0;

    double sq = sqrt(disc);
    double t0 = (-B - sq) / (2.0 * A);
    double t1 = (-B + sq) / (2.0 * A);

    if (t1 < 0.0) return -1.0;

    double t = (t0 > 0.0) ? t0 : t1;
    if (maxT > 0.0 && t > maxT) return -1.0;
    return t;
}

// =====================================================================
HitInfo RaycastCharacter(double ox, double oy,
    double dx, double dy,
    double px, double py, double a,
    double maxT)
{
    HitInfo best = { false, 0.0, -1 };

    double ca = cos(a);
    double sa = sin(a);

    // ћировые оси повЄрнутой модели цели:
    //   right   = ( cos(a), -sin(a) )
    //   forward = ( sin(a),  cos(a) )
    //
    // Ћокальные (cx, cy) переводим в мировые:
    //   wx = px + cx*cos(a) + cy*sin(a)
    //   wy = py - cx*sin(a) + cy*cos(a)

    for (int i = 0; i < PLAYER_PART_COUNT; ++i) {
        const BodyPart& b = PLAYER_PARTS[i];

        double wpx = px + b.cx * ca + b.cy * sa;
        double wpy = py - b.cx * sa + b.cy * ca;

        double t = -1.0;
        if (b.kind == PART_SPHERE) {
            t = RaySphereXZ(ox, oy, dx, dy, wpx, wpy, b.r, maxT);
        }
        else {
            t = RayCylXZ(ox, oy, dx, dy, wpx, wpy, b.r,
                b.zMin, b.zMax, maxT);
        }

        if (t < 0.0) continue;
        if (!best.hit || t < best.distance) {
            best.hit = true;
            best.distance = t;
            best.partIdx = i;
        }
    }

    return best;
}