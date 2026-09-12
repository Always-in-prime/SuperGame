#include "BodyTextures.h"
#include <math.h>

uint32_t bodyTex[PART_TEX_COUNT][BODYTEX_W][BODYTEX_H];

static bool s_ready = false;

// ---------------------------------------------------------------------
//  Утилиты
// ---------------------------------------------------------------------
static inline uint32_t C(int r, int g, int b) { return RGB(r, g, b); }

// =====================================================================
//  Генераторы (переносим 1:1 из старого Sprite.cpp, без изменений логики)
// =====================================================================

static uint32_t HeadTex(double u, double v) {
    double cs = cos(u);
    double sAbs = fabs(sin(u));

    if (v > 0.86) {
        uint32_t c = C(95, 112, 152);
        if (sAbs < 0.12 && cs > 0.55) c = C(210, 175, 90);
        if (v > 0.95) c = C(135, 155, 195);
        return c;
    }
    if (v > 0.28) {
        if (cs > 0.55 && sAbs < 0.78) {
            if (v > 0.52 && v < 0.78) {
                uint32_t c = C(20, 50, 85);
                if (sAbs < 0.42 && cs > 0.72) c = C(65, 175, 225);
                if (sAbs > 0.18 && sAbs < 0.42) c = C(35, 95, 145);
                return c;
            }
            return C(30, 35, 48);
        }
        uint32_t c = C(85, 100, 138);
        if (cs < -0.4) c = C(52, 62, 90);
        if (sAbs > 0.85) c = C(45, 55, 80);
        return c;
    }
    return (cs > 0.4) ? C(190, 150, 120) : C(45, 50, 65);
}

static uint32_t TorsoTex(double u, double v) {
    double cs = cos(u);
    double sAbs = fabs(sin(u));

    if (v > 0.92) return C(55, 65, 95);

    if (v > 0.82) {
        if (cs > 0.3 && sAbs < 0.75) {
            return (sAbs < 0.15) ? C(95, 115, 165) : C(75, 92, 135);
        }
        return C(40, 50, 75);
    }

    if (v > 0.30) {
        if (cs > 0.2) {
            uint32_t c = C(60, 78, 118);
            if (sAbs < 0.10 && v > 0.45 && v < 0.68) c = C(85, 108, 155);
            if (sAbs < 0.18 && v > 0.55 && v < 0.75) {
                c = C(80, 170, 220);
                if (sAbs < 0.09) c = C(180, 230, 255);
            }
            if ((v > 0.42 && v < 0.45) || (v > 0.76 && v < 0.79))
                if (sAbs < 0.60) c = C(190, 155, 75);
            return c;
        }
        uint32_t c = C(30, 40, 60);
        if (sAbs < 0.40 && v > 0.45 && v < 0.80) c = C(45, 55, 78);
        if (sAbs < 0.10 && v > 0.55 && v < 0.62) c = C(80, 180, 220);
        return c;
    }

    if (v > 0.12) {
        if (cs > 0.5 && sAbs < 0.20) return C(190, 155, 80);
        if (cs > 0.15) return C(90, 65, 40);
        return C(55, 38, 22);
    }

    return (sAbs > 0.3) ? C(52, 65, 95) : C(38, 48, 72);
}

static uint32_t LegTex(double u, double v) {
    double cs = cos(u);
    double sAbs = fabs(sin(u));

    if (v > 0.85) return C(50, 62, 92);
    if (v > 0.55) {
        if (sAbs < 0.15) return C(30, 40, 65);
        return C(48, 60, 90);
    }
    if (v > 0.35) {
        if (cs > 0.3 && sAbs < 0.70) {
            uint32_t c = C(95, 115, 160);
            if (v > 0.40 && v < 0.50 && sAbs < 0.35) c = C(130, 150, 195);
            if (v > 0.44 && v < 0.47 && sAbs < 0.60) c = C(180, 155, 80);
            return c;
        }
        return C(50, 62, 92);
    }
    if (v > 0.10) {
        if (cs > 0.15 && sAbs < 0.65) {
            uint32_t c = C(75, 92, 135);
            if (v > 0.20 && v < 0.25 && sAbs < 0.55) c = C(110, 130, 175);
            return c;
        }
        return C(45, 55, 82);
    }
    if (v > 0.04) {
        uint32_t c = C(50, 55, 68);
        if (cs > 0.2 && sAbs < 0.5) c = C(75, 80, 95);
        if (cs > 0.5 && sAbs < 0.30) c = C(105, 112, 130);
        if (sAbs > 0.4 && sAbs < 0.7) c = C(120, 95, 50);
        return c;
    }
    return C(18, 18, 22);
}

static uint32_t ArmTex(double u, double v) {
    double cs = cos(u);
    double sAbs = fabs(sin(u));

    if (v > 0.75) {
        uint32_t c = C(55, 68, 100);
        if (cs > 0.3 && sAbs < 0.6) c = C(78, 95, 135);
        return c;
    }
    if (v > 0.25) {
        if (cs > 0.2 && sAbs < 0.65) {
            uint32_t c = C(85, 105, 150);
            if (v > 0.40 && v < 0.55 && sAbs < 0.5) c = C(110, 130, 175);
            if ((v > 0.28 && v < 0.31) || (v > 0.68 && v < 0.71))
                if (sAbs < 0.55) c = C(180, 148, 75);
            return c;
        }
        return C(42, 52, 78);
    }
    return (v < 0.10) ? C(25, 28, 38) : C(35, 40, 52);
}

static uint32_t ShoulderTex(double u, double v) {
    double cs = cos(u);
    double sAbs = fabs(sin(u));

    if (v > 0.85) {
        uint32_t c = C(95, 115, 160);
        if (sAbs < 0.3) c = C(130, 150, 195);
        return c;
    }
    if (v > 0.35) {
        uint32_t c = C(70, 85, 122);
        if (cs > 0.2 && sAbs < 0.65) c = C(95, 115, 160);
        if (v > 0.42 && v < 0.46 && sAbs < 0.75) c = C(190, 155, 80);
        if (v > 0.65 && v < 0.69 && sAbs < 0.75) c = C(190, 155, 80);
        return c;
    }
    if (v > 0.15) {
        uint32_t c = C(45, 55, 85);
        if (cs > 0.4 && sAbs < 0.5) c = C(70, 85, 120);
        return c;
    }
    return C(28, 35, 52);
}

// =====================================================================
void InitBodyTextures() {
    if (s_ready) return;
    s_ready = true;

    for (int i = 0; i < BODYTEX_W; ++i) {
        double u = (double)i / BODYTEX_W * 2.0 * PI;
        for (int j = 0; j < BODYTEX_H; ++j) {
            double v = 1.0 - (double)j / (BODYTEX_H - 1);
            bodyTex[PART_HEAD][i][j] = HeadTex(u, v);
            bodyTex[PART_TORSO][i][j] = TorsoTex(u, v);
            bodyTex[PART_LEG][i][j] = LegTex(u, v);
            bodyTex[PART_ARM][i][j] = ArmTex(u, v);
            bodyTex[PART_SHOULDER][i][j] = ShoulderTex(u, v);
        }
    }
}

uint32_t SampleBodyTex(int texId, double angle, double vNorm) {
    while (angle < 0)             angle += 2.0 * PI;
    while (angle >= 2.0 * PI)     angle -= 2.0 * PI;

    int i = (int)(angle / (2.0 * PI) * BODYTEX_W);
    if (i < 0) i = 0;
    if (i >= BODYTEX_W) i = BODYTEX_W - 1;

    int j = (int)((1.0 - vNorm) * (BODYTEX_H - 1));
    if (j < 0) j = 0;
    if (j >= BODYTEX_H) j = BODYTEX_H - 1;

    return bodyTex[texId][i][j];
}