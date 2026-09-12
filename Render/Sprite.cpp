#include "Sprite.h"
#include "Framebuffer.h"
#include "Common.h"
#include "Player.h"
#include "CharacterModel.h"
#include "BodyTextures.h"
#include <math.h>

// =====================================================================
//  Рендер противника:
//    - геометрия берётся из PLAYER_PARTS (общая с Hitbox)
//    - текстуры берутся из BodyTextures
//    - анимация применяется поверх статичной модели
// =====================================================================

struct SpriteHit {
    double tNear;
    int    idx;
};

static inline uint32_t ShadeN(uint32_t base, double nx, double ny, double nz, double rim) {
    const double Lx = 0.35, Ly = 0.55, Lz = 0.75;
    double diff = nx * Lx + ny * Ly + nz * Lz;
    if (diff < 0) diff = 0;
    double light = 0.42 + 0.65 * diff;
    if (light > 1.22) light = 1.22;

    int r = (int)(GetRValue(base) * light) + (int)(rim * 55);
    int g = (int)(GetGValue(base) * light) + (int)(rim * 100);
    int b = (int)(GetBValue(base) * light) + (int)(rim * 155);
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return RGB(r, g, b);
}

static void AnimatePart(const BodyPart& src, double walkPhase,
    double& outX, double& outY,
    double& outZmin, double& outZmax)
{
    outX = src.cx;
    outY = src.cy;
    outZmin = src.zMin;
    outZmax = src.zMax;

    double legSwing = sin(walkPhase * 2.0) * 0.075;
    double armSwing = -legSwing * 0.80;
    double bob = sin(walkPhase * 2.0) * 0.010;

    outZmin += bob;
    outZmax += bob;

    if (src.texId == PART_LEG) {
        if (src.cx < 0) outY += legSwing;
        else            outY -= legSwing;
    }
    if (src.texId == PART_ARM) {
        if (src.cx < 0) outY += armSwing;
        else            outY -= armSwing;
    }
}

// =====================================================================
void RenderOtherPlayer(const Player& viewer, const Player& target,
    int screen_left, int view_width, int half_height, const double* zbuffer)
{
    if (target.deathTimer > 0) return;
    InitBodyTextures();

    double vx = viewer.renderX, vy = viewer.renderY, va = viewer.renderAngle;
    double tx = target.renderX, ty = target.renderY, ta = target.renderAngle;

    double dxw = tx - vx, dyw = ty - vy;
    double dist = sqrt(dxw * dxw + dyw * dyw);
    if (dist < 0.30) return;

    double angle_to = atan2(dxw, dyw);
    double angle_diff = angle_to - va;
    while (angle_diff > PI) angle_diff -= 2.0 * PI;
    while (angle_diff < -PI) angle_diff += 2.0 * PI;
    const double halfFov = FOV * 0.5;
    if (fabs(angle_diff) > halfFov + 0.6) return;

    const double camSin = sin(va);
    const double camCos = cos(va);
    const double planeLen = tan(halfFov);

    double cosTa = cos(ta), sinTa = sin(ta);

    // ---- Анимация ----
    struct AnimPart {
        int    kind;
        double cx, cy, r;
        double zMin, zMax;
        int    texId;
    };
    AnimPart anim[PLAYER_MAX_PARTS];

    int usedParts = PLAYER_PART_COUNT;
    if (usedParts > PLAYER_MAX_PARTS) usedParts = PLAYER_MAX_PARTS;

    for (int i = 0; i < usedParts; ++i) {
        double ox, oy, zMin, zMax;
        AnimatePart(PLAYER_PARTS[i], target.walkPhase, ox, oy, zMin, zMax);
        anim[i].kind = PLAYER_PARTS[i].kind;
        anim[i].cx = ox;
        anim[i].cy = oy;
        anim[i].r = PLAYER_PARTS[i].r;
        anim[i].zMin = zMin;
        anim[i].zMax = zMax;
        anim[i].texId = PLAYER_PARTS[i].texId;
    }

    // ---- По колонкам ----
    for (int x = 0; x < view_width; ++x) {
        double u = (2.0 * (x + 0.5) / (double)view_width - 1.0) * planeLen;
        double dirX = camSin + u * camCos;
        double dirY = camCos - u * camSin;

        double wx = vx - tx, wy = vy - ty;
        double ldx = dirX * cosTa - dirY * sinTa;
        double ldy = dirX * sinTa + dirY * cosTa;
        double lox = wx * cosTa - wy * sinTa;
        double loy = wx * sinTa + wy * cosTa;

        double zWall = zbuffer[x];
        if (zWall < 0.05) continue;

        SpriteHit hits[PLAYER_MAX_PARTS];
        int nHits = 0;

        for (int p = 0; p < usedParts; ++p) {
            const AnimPart& pt = anim[p];

            if (pt.kind == PART_SPHERE) {
                double ox = lox - pt.cx, oy = loy - pt.cy;
                double A = ldx * ldx + ldy * ldy;
                if (A < 1e-9) continue;
                double B = 2.0 * (ox * ldx + oy * ldy);
                double Cc = ox * ox + oy * oy - pt.r * pt.r;
                double disc = B * B - 4.0 * A * Cc;
                if (disc < 0.0) continue;
                double sq = sqrt(disc);
                double t0 = (-B - sq) / (2.0 * A);
                if (t0 < 0.05) continue;
                hits[nHits].tNear = t0;
                hits[nHits].idx = p;
                ++nHits;
            }
            else {
                double ex = (lox - pt.cx) / pt.r;
                double ey = (loy - pt.cy) / pt.r;
                double dxn = ldx / pt.r;
                double dyn = ldy / pt.r;
                double A = dxn * dxn + dyn * dyn;
                if (A < 1e-9) continue;
                double B = 2.0 * (ex * dxn + ey * dyn);
                double Cc = ex * ex + ey * ey - 1.0;
                double disc = B * B - 4.0 * A * Cc;
                if (disc < 0.0) continue;
                double sq = sqrt(disc);
                double t0 = (-B - sq) / (2.0 * A);
                if (t0 < 0.05) continue;
                hits[nHits].tNear = t0;
                hits[nHits].idx = p;
                ++nHits;
            }
        }

        if (nHits == 0) continue;

        // Дальние → ближние
        for (int i = 0; i < nHits - 1; ++i)
            for (int j = i + 1; j < nHits; ++j)
                if (hits[i].tNear < hits[j].tNear) {
                    SpriteHit tmp = hits[i];
                    hits[i] = hits[j];
                    hits[j] = tmp;
                }

        for (int hi = 0; hi < nHits; ++hi) {
            const AnimPart& pt = anim[hits[hi].idx];
            double tN = hits[hi].tNear;
            if (tN > zWall) continue;

            double fog = (tN > 4.0) ? 4.0 / tN : 1.0;
            if (fog < 0.30) fog = 0.30;

            double wall_h = (double)WINDOW_HEIGHT / tN;
            int yTop = half_height + (int)(wall_h * (0.5 - pt.zMax));
            int yBot = half_height + (int)(wall_h * (0.5 - pt.zMin));
            if (yTop < 0) yTop = 0;
            if (yBot >= WINDOW_HEIGHT) yBot = WINDOW_HEIGHT - 1;
            if (yTop > yBot) continue;

            double lhx = lox + ldx * tN;
            double lhy = loy + ldy * tN;
            double relx = lhx - pt.cx;
            double rely = lhy - pt.cy;
            double tAng = atan2(relx, rely);

            double nlen = sqrt(relx * relx + rely * rely);
            double nx = 0, ny = 0;
            if (nlen > 1e-6) { nx = relx / nlen; ny = rely / nlen; }
            double vdot = nx * ldx + ny * ldy;
            double rim = 1.0 - fabs(vdot);
            if (rim < 0.0) rim = 0.0;
            rim = rim * rim * rim;

            double invH = 1.0 / (pt.zMax - pt.zMin);
            for (int y = yTop; y <= yBot; ++y) {
                double wz = 0.5 + (half_height - y) / wall_h;
                double tv = (wz - pt.zMin) * invH;
                if (tv < 0.0) tv = 0.0;
                if (tv > 1.0) tv = 1.0;

                uint32_t base = SampleBodyTex(pt.texId, tAng, tv);
                uint32_t col = ShadeN(base, nx, ny, 0.0, rim);

                int r = (int)(GetRValue(col) * fog);
                int g = (int)(GetGValue(col) * fog);
                int b = (int)(GetBValue(col) * fog);
                SetPixelFast(screen_left + x, y, RGB(r, g, b));
            }
        }
    }
}