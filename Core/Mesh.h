#pragma once
#include "Primitive.h"

// ============================================================
//  Mesh — набор примитивов + трансформация.
//  Используется и для оружия, и для персонажей, и для пикапов.
// ============================================================

#define MESH_MAX_PARTS 32

struct Mesh {
    Primitive parts[MESH_MAX_PARTS];
    int       count;

    // Мировая позиция и поворот объекта.
    double posX, posY, posZ;
    double angle;          // поворот вокруг Y (Y — вверх)
    double scale;          // общий масштаб (обычно 1.0)

    // Дополнительные смещения (для анимации, отдачи, покачивания).
    double offX, offY, offZ;
};

// ----- Управление мешем -----
inline void Mesh_Init(Mesh& m,
    double posX, double posY, double posZ,
    double angle, double scale = 1.0) {
    m.count = 0;
    m.posX = posX; m.posY = posY; m.posZ = posZ;
    m.angle = angle;
    m.scale = scale;
    m.offX = m.offY = m.offZ = 0.0;
}

inline void Mesh_Add(Mesh& m, const Primitive& p) {
    if (m.count >= MESH_MAX_PARTS) return;
    m.parts[m.count++] = p;
}