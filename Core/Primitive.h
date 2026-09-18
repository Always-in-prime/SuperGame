#pragma once
#include "Common.h"
#include <cstdint>

// ============================================================
//  ѕримитив Ч элементарна€ 3D-форма дл€ рендера.
//  ≈диный формат дл€ оружи€, персонажей, пикапов, чего угодно.
//
//  —истема координат: XZ Ч горизонталь, Y Ч вертикаль.
//  (в отличие от старого Sprite.cpp Ч там Y был "высотой",
//   теперь высота это Y, а горизонталь Ч XZ. “ак пон€тнее.)
// ============================================================

enum PrimKind {
    PRIM_SPHERE = 0,   // сфера Ч голова, мушки, €дра
    PRIM_CYL_Y = 1,   // цилиндр вдоль Y Ч стволы, ноги (вертикальный)
    PRIM_CYL_Z = 2,   // цилиндр вдоль Z Ч руко€тки (вперЄд-назад)
    PRIM_CYL_X = 3,   // цилиндр вдоль X Ч плечи (вбок)
};

struct Primitive {
    PrimKind kind;

    // ÷ентр примитива в локальной системе объекта.
    double cx, cy, cz;

    // –адиус поперечного сечени€.
    double r;

    // √раницы вдоль главной оси (дл€ цилиндров).
    // ƒл€ сферы не используютс€ (aMin = aMax = 0).
    double aMin, aMax;

    // “екстура:
    //   texId >= 0 Ч использовать текстуру из TextureBank.
    //   texId == -1 Ч заливка плоским цветом (col).
    int      texId;
    uint32_t col;

    // ћножитель урона (дл€ частей тела Ч хедшот и т.д.).
    // ƒл€ оружи€ и декоративных объектов = 1.0.
    double damageMult;
};

// ”тилиты создани€ примитивов (упрощают заполнение).
inline Primitive MakeSphere(double x, double y, double z, double r,
    int texId, uint32_t col, double dmg = 1.0) {
    Primitive p;
    p.kind = PRIM_SPHERE;
    p.cx = x; p.cy = y; p.cz = z;
    p.r = r;
    p.aMin = p.aMax = 0;
    p.texId = texId;
    p.col = col;
    p.damageMult = dmg;
    return p;
}

inline Primitive MakeCylY(double x, double y1, double y2, double z, double r,
    int texId, uint32_t col, double dmg = 1.0) {
    Primitive p;
    p.kind = PRIM_CYL_Y;
    p.cx = x; p.cy = 0; p.cz = z;
    p.r = r;
    p.aMin = y1; p.aMax = y2;
    p.texId = texId;
    p.col = col;
    p.damageMult = dmg;
    return p;
}

inline Primitive MakeCylZ(double x, double y, double z1, double z2, double r,
    int texId, uint32_t col, double dmg = 1.0) {
    Primitive p;
    p.kind = PRIM_CYL_Z;
    p.cx = x; p.cy = y; p.cz = 0;
    p.r = r;
    p.aMin = z1; p.aMax = z2;
    p.texId = texId;
    p.col = col;
    p.damageMult = dmg;
    return p;
}

inline Primitive MakeCylX(double x1, double x2, double y, double z, double r,
    int texId, uint32_t col, double dmg = 1.0) {
    Primitive p;
    p.kind = PRIM_CYL_X;
    p.cx = 0; p.cy = y; p.cz = z;
    p.r = r;
    p.aMin = x1; p.aMax = x2;
    p.texId = texId;
    p.col = col;
    p.damageMult = dmg;
    return p;
}