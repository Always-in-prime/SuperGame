#include "CharacterModel.h"
#include "../Core/TextureBank.h"
#include <cmath>

// ---------------------------------------------------------------------
//  Индексы текстур
// ---------------------------------------------------------------------
int g_charTexIds[PART_TEX_COUNT] = { -1, -1, -1, -1, -1 };

// ---------------------------------------------------------------------
//  Геометрия модели — все магические числа в одном месте.
//  Ось Y — вверх. Центр персонажа — в (0, 0, 0) локально.
// ---------------------------------------------------------------------
namespace CharGeom {
    // Голова
    constexpr double kHeadY = 0.90;
    constexpr double kHeadR = 0.115;

    // Торс
    constexpr double kTorsoY0 = 0.36;
    constexpr double kTorsoY1 = 0.80;
    constexpr double kTorsoR = 0.185;

    // Наплечники
    constexpr double kShoulderX = 0.235;
    constexpr double kShoulderY = 0.72;
    constexpr double kShoulderR = 0.108;

    // Ноги
    constexpr double kLegX = 0.085;
    constexpr double kLegY0 = 0.0;
    constexpr double kLegY1 = 0.36;
    constexpr double kLegR = 0.075;

    // Руки
    constexpr double kArmX = 0.248;
    constexpr double kArmY0 = 0.42;
    constexpr double kArmY1 = 0.74;
    constexpr double kArmR = 0.058;

    // Анимация
    constexpr double kSwingLeg = 0.075;
    constexpr double kSwingArmK = 0.80;   // arm = -leg * K
    constexpr double kBobAmp = 0.010;
} // namespace CharGeom

// ---------------------------------------------------------------------
//  Генераторы текстур (u,v) -> цвет.
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
//  Анимация.
//
//  AnimPart описывает вертикальный цилиндр:
//    x     — X-смещение оси
//    y0,y1 — Y-диапазон
//    zOff  — Z-смещение (шаг вперёд/назад)
// ---------------------------------------------------------------------
struct AnimPart {
    double x;
    double y0, y1;
    double zOff;
};

// Предрасчитанный «фазовый» синус — чтобы не звать sin трижды за кадр.
struct WalkPhase {
    double s;   // sin(walkPhase * 2.0)
    double bob; // sin(walkPhase * 2.0) * kBobAmp
};

static inline WalkPhase MakeWalkPhase(double walkPhase) noexcept {
    const double s = std::sin(walkPhase * 2.0);
    return { s, s * CharGeom::kBobAmp };
}

static inline AnimPart AnimateLeg(bool left, const WalkPhase& wp) noexcept {
    const double swing = wp.s * CharGeom::kSwingLeg;
    return {
        left ? -CharGeom::kLegX : CharGeom::kLegX,
        CharGeom::kLegY0,
        CharGeom::kLegY1,
        left ? swing : -swing
    };
}

static inline AnimPart AnimateArm(bool left, const WalkPhase& wp) noexcept {
    const double armSwing = -wp.s * CharGeom::kSwingLeg * CharGeom::kSwingArmK;
    return {
        left ? -CharGeom::kArmX : CharGeom::kArmX,
        CharGeom::kArmY0,
        CharGeom::kArmY1,
        left ? armSwing : -armSwing
    };
}

// ---------------------------------------------------------------------
//  Локальный меш
// ---------------------------------------------------------------------
void BuildCharacterMeshLocal(Mesh& mesh, double walkPhase) {
    const WalkPhase wp = MakeWalkPhase(walkPhase);
    const double bob = wp.bob;

    // --- Голова ---
    Mesh_Add(mesh, MakeSphere(0.0, CharGeom::kHeadY + bob, 0.0,
        CharGeom::kHeadR,
        g_charTexIds[PART_HEAD], RGB(255, 215, 175),
        2.0));

    // --- Торс ---
    Mesh_Add(mesh, MakeCylY(0.0,
        CharGeom::kTorsoY0 + bob, CharGeom::kTorsoY1 + bob, 0.0,
        CharGeom::kTorsoR,
        g_charTexIds[PART_TORSO], RGB(60, 80, 140),
        1.0));

    // --- Наплечники ---
    Mesh_Add(mesh, MakeSphere(-CharGeom::kShoulderX, CharGeom::kShoulderY + bob, 0.0,
        CharGeom::kShoulderR,
        g_charTexIds[PART_SHOULDER], RGB(85, 90, 110),
        0.8));
    Mesh_Add(mesh, MakeSphere(CharGeom::kShoulderX, CharGeom::kShoulderY + bob, 0.0,
        CharGeom::kShoulderR,
        g_charTexIds[PART_SHOULDER], RGB(85, 90, 110),
        0.8));

    // --- Ноги ---
    const AnimPart legL = AnimateLeg(true, wp);
    const AnimPart legR = AnimateLeg(false, wp);
    const int legTex = g_charTexIds[PART_LEG];
    const uint32_t legTint = RGB(55, 65, 105);

    Mesh_Add(mesh, MakeCylY(legL.x, legL.y0 + bob, legL.y1 + bob, legL.zOff,
        CharGeom::kLegR, legTex, legTint, 1.0));
    Mesh_Add(mesh, MakeCylY(legR.x, legR.y0 + bob, legR.y1 + bob, legR.zOff,
        CharGeom::kLegR, legTex, legTint, 1.0));

    // --- Руки ---
    const AnimPart armL = AnimateArm(true, wp);
    const AnimPart armR = AnimateArm(false, wp);
    const int armTex = g_charTexIds[PART_ARM];
    const uint32_t armTint = RGB(55, 90, 195);

    Mesh_Add(mesh, MakeCylY(armL.x, armL.y0 + bob, armL.y1 + bob, armL.zOff,
        CharGeom::kArmR, armTex, armTint, 0.7));
    Mesh_Add(mesh, MakeCylY(armR.x, armR.y0 + bob, armR.y1 + bob, armR.zOff,
        CharGeom::kArmR, armTex, armTint, 0.7));
}

// ---------------------------------------------------------------------
//  Мировой меш
// ---------------------------------------------------------------------
void BuildCharacterMesh(Mesh& mesh, const Player& p) {
    Mesh_Init(mesh, p.renderX, 0.0, p.renderY, p.renderAngle, 1.0);
    BuildCharacterMeshLocal(mesh, p.walkPhase);
}