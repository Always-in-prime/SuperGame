#include "CharacterModel.h"
#include "../Core/TextureBank.h"
#include <cmath>

// ---------------------------------------------------------------------
//  Индексы текстур в банке (заполняются в CharacterModel_InitTextures)
// ---------------------------------------------------------------------
int g_charTexIds[PART_TEX_COUNT] = { -1, -1, -1, -1, -1 };

// ---------------------------------------------------------------------
//  Генераторы текстур (перенесены из BodyTextures.cpp, но без массива —
//  просто функции (u,v) -> цвет).
// ---------------------------------------------------------------------
static inline uint32_t C(int r, int g, int b) { return RGB(r, g, b); }

// --- ШЛЕМ ---
static uint32_t HeadTex(double u, double v) {
    double cs = cos(u * 2.0 * PI);
    double sAbs = fabs(sin(u * 2.0 * PI));

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

// --- ТОРС ---
static uint32_t TorsoTex(double u, double v) {
    double cs = cos(u * 2.0 * PI);
    double sAbs = fabs(sin(u * 2.0 * PI));

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

// --- НОГА ---
static uint32_t LegTex(double u, double v) {
    double cs = cos(u * 2.0 * PI);
    double sAbs = fabs(sin(u * 2.0 * PI));

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

// --- РУКА ---
static uint32_t ArmTex(double u, double v) {
    double cs = cos(u * 2.0 * PI);
    double sAbs = fabs(sin(u * 2.0 * PI));

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

// --- НАПЛЕЧНИК ---
static uint32_t ShoulderTex(double u, double v) {
    double cs = cos(u * 2.0 * PI);
    double sAbs = fabs(sin(u * 2.0 * PI));

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

// ---------------------------------------------------------------------
//  Регистрация текстур
// ---------------------------------------------------------------------
void CharacterModel_InitTextures() {
    g_charTexIds[PART_HEAD] = TexBank_Register("head", HeadTex);
    g_charTexIds[PART_TORSO] = TexBank_Register("torso", TorsoTex);
    g_charTexIds[PART_LEG] = TexBank_Register("leg", LegTex);
    g_charTexIds[PART_ARM] = TexBank_Register("arm", ArmTex);
    g_charTexIds[PART_SHOULDER] = TexBank_Register("shoulder", ShoulderTex);
}

// ---------------------------------------------------------------------
//  Анимация: возвращает смещённые координаты части с учётом walkPhase.
// ---------------------------------------------------------------------
struct AnimPart {
    double cx, cy, cz;   // смещение центра
    double zMin, zMax;   // для цилиндров
};

static AnimPart AnimateLeg(bool left, double walkPhase) {
    AnimPart a;
    double legSwing = sin(walkPhase * 2.0) * 0.075;
    a.cx = left ? -0.085 : 0.085;
    a.cy = 0.0;
    a.cz = left ? legSwing : -legSwing;
    a.zMin = 0.0;
    a.zMax = 0.36;
    return a;
}

static AnimPart AnimateArm(bool left, double walkPhase) {
    AnimPart a;
    double legSwing = sin(walkPhase * 2.0) * 0.075;
    double armSwing = -legSwing * 0.80;
    a.cx = left ? -0.248 : 0.248;
    a.cy = 0.0;
    a.cz = left ? armSwing : -armSwing;
    a.zMin = 0.42;
    a.zMax = 0.74;
    return a;
}

// ---------------------------------------------------------------------
//  Сборка ЛОКАЛЬНОГО меша (используется и для мирового)
// ---------------------------------------------------------------------
void BuildCharacterMeshLocal(Mesh& mesh, double walkPhase) {
    double bob = sin(walkPhase * 2.0) * 0.010;

    // --- Голова (сфера) ---
    Mesh_Add(mesh, MakeSphere(0.0, 0.90 + bob, 0.0,
        0.115,
        g_charTexIds[PART_HEAD], RGB(255, 215, 175),
        2.0));

    // --- Торс (вертикальный цилиндр вокруг Y) ---
    // В старой модели это был цилиндр XZ-плоскости. Теперь ось Y.
    Mesh_Add(mesh, MakeCylY(0.0, 0.36 + bob, 0.80 + bob, 0.0,
        0.185,
        g_charTexIds[PART_TORSO], RGB(60, 80, 140),
        1.0));

    // --- Наплечники (сферы по бокам сверху) ---
    Mesh_Add(mesh, MakeSphere(-0.235, 0.72 + bob, 0.0,
        0.108,
        g_charTexIds[PART_SHOULDER], RGB(85, 90, 110),
        0.8));
    Mesh_Add(mesh, MakeSphere(0.235, 0.72 + bob, 0.0,
        0.108,
        g_charTexIds[PART_SHOULDER], RGB(85, 90, 110),
        0.8));

    // --- Ноги (вертикальные цилиндры) ---
    AnimPart legL = AnimateLeg(true, walkPhase);
    AnimPart legR = AnimateLeg(false, walkPhase);

    Mesh_Add(mesh, MakeCylY(legL.cx, legL.zMin + bob, legL.zMax + bob, legL.cz,
        0.075,
        g_charTexIds[PART_LEG], RGB(55, 65, 105),
        1.0));
    Mesh_Add(mesh, MakeCylY(legR.cx, legR.zMin + bob, legR.zMax + bob, legR.cz,
        0.075,
        g_charTexIds[PART_LEG], RGB(55, 65, 105),
        1.0));

    // --- Руки (вертикальные цилиндры) ---
    AnimPart armL = AnimateArm(true, walkPhase);
    AnimPart armR = AnimateArm(false, walkPhase);

    Mesh_Add(mesh, MakeCylY(armL.cx, armL.zMin + bob, armL.zMax + bob, armL.cz,
        0.058,
        g_charTexIds[PART_ARM], RGB(55, 90, 195),
        0.7));
    Mesh_Add(mesh, MakeCylY(armR.cx, armR.zMin + bob, armR.zMax + bob, armR.cz,
        0.058,
        g_charTexIds[PART_ARM], RGB(55, 90, 195),
        0.7));
}

// ---------------------------------------------------------------------
//  Сборка МИРОВОГО меша
// ---------------------------------------------------------------------
void BuildCharacterMesh(Mesh& mesh, const Player& p) {
    // Угол и позиция — из Player.
    Mesh_Init(mesh, p.renderX, 0.0, p.renderY, p.renderAngle, 1.0);
    BuildCharacterMeshLocal(mesh, p.walkPhase);
}