#pragma once
#include "Primitive.h"
#include "Mesh.h"

// ============================================================
//  Единый raycast против примитива/меша.
//
//  Возвращает:
//    - попал ли луч
//    - расстояние вдоль луча (в МИРОВЫХ единицах)
//    - индекс примитива
//    - текстурные координаты u,v в точке попадания
//    - нормаль поверхности (в МИРОВОЙ системе)
//
//  Луч задаётся в МИРОВОЙ системе. Внутри RaycastMesh переводит
//  его в локальную систему меша через предпосчитанный MeshXform.
//
//  Производительность:
//    - MeshXform строится один раз на кадр (или на выстрел) через
//      BuildXform(mesh) и переиспользуется для всех лучей.
//    - AABB меша в локальной системе — broadphase-отсев
//      в RaycastMesh до перебора примитивов.
// ============================================================

struct PrimHit {
    bool   hit;
    double t;         // расстояние вдоль луча в МИРОВЫХ единицах
    int    primIdx;   // индекс примитива в Mesh
    double u, v;      // текстурные координаты [0,1)
    double nx, ny, nz;// нормаль (мировая система)
};

// Предпосчитанный трансформ меша.
// Заполняется BuildXform(mesh) — все тригонометрии и деления
// делаются один раз на кадр, а не на каждый луч.
struct MeshXform {
    double posX, posY, posZ;  // мировая позиция (уже с offX/Y/Z)
    double cosA, sinA;        // cos(angle), sin(angle)
    double invScale;          // 1 / scale
    double scale;             // scale (для восстановления t)
    double invScaleSq;        // invScale * invScale

    // AABB меша в ЛОКАЛЬНОЙ системе. Заполняется BuildXform.
    // Используется для broadphase-отсева в RaycastMesh.
    double aabbMinX, aabbMinY, aabbMinZ;
    double aabbMaxX, aabbMaxY, aabbMaxZ;
};

// Собрать трансформ из меша. Вызывать один раз на кадр на объект.
inline MeshXform BuildXform(const Mesh& m) noexcept {
    MeshXform xf;
    xf.posX = m.posX + m.offX;
    xf.posY = m.posY + m.offY;
    xf.posZ = m.posZ + m.offZ;
    xf.cosA = std::cos(m.angle);
    xf.sinA = std::sin(m.angle);
    const double s = (m.scale > 1e-6) ? m.scale : 1.0;
    xf.scale = s;
    xf.invScale = 1.0 / s;
    xf.invScaleSq = xf.invScale * xf.invScale;

    // ---- AABB в локальных координатах ----
    double mnX = 1e30, mnY = 1e30, mnZ = 1e30;
    double mxX = -1e30, mxY = -1e30, mxZ = -1e30;

    for (int i = 0; i < m.count; ++i) {
        const Primitive& p = m.parts[i];
        double px0, py0, pz0, px1, py1, pz1;
        switch (p.kind) {
        case PRIM_SPHERE:
            px0 = p.cx - p.r; py0 = p.cy - p.r; pz0 = p.cz - p.r;
            px1 = p.cx + p.r; py1 = p.cy + p.r; pz1 = p.cz + p.r;
            break;
        case PRIM_CYL_Y:
            px0 = p.cx - p.r; py0 = p.aMin;     pz0 = p.cz - p.r;
            px1 = p.cx + p.r; py1 = p.aMax;     pz1 = p.cz + p.r;
            break;
        case PRIM_CYL_Z:
            px0 = p.cx - p.r; py0 = p.cy - p.r; pz0 = p.aMin;
            px1 = p.cx + p.r; py1 = p.cy + p.r; pz1 = p.aMax;
            break;
        case PRIM_CYL_X:
            px0 = p.aMin;     py0 = p.cy - p.r; pz0 = p.cz - p.r;
            px1 = p.aMax;     py1 = p.cy + p.r; pz1 = p.cz + p.r;
            break;
        default:
            continue;
        }
        if (px0 < mnX) mnX = px0;
        if (py0 < mnY) mnY = py0;
        if (pz0 < mnZ) mnZ = pz0;
        if (px1 > mxX) mxX = px1;
        if (py1 > mxY) mxY = py1;
        if (pz1 > mxZ) mxZ = pz1;
    }

    // Если примитивов нет — AABB вырожденный, тест всегда промахнётся.
    xf.aabbMinX = mnX; xf.aabbMinY = mnY; xf.aabbMinZ = mnZ;
    xf.aabbMaxX = mxX; xf.aabbMaxY = mxY; xf.aabbMaxZ = mxZ;
    return xf;
}

// Пересечение луча с одним примитивом. Луч: origin + t*dir.
PrimHit RaycastPrimitive(const Primitive& p,
    double ox, double oy, double oz,
    double dx, double dy, double dz) noexcept;

// Пересечение луча со всеми примитивами меша — ближайшее.
// maxT > 0 — ограничение сверху (например, до стены).
PrimHit RaycastMesh(const Mesh& m, const MeshXform& xf,
    double ox, double oy, double oz,
    double dx, double dy, double dz,
    double maxT = 0.0) noexcept;

// Удобный overload: строит xform на месте.
// ВНИМАНИЕ: делает 2 тригонометрии за вызов. Для горячего пути
// всегда используйте версию с явным MeshXform.
PrimHit RaycastMesh(const Mesh& m,
    double ox, double oy, double oz,
    double dx, double dy, double dz,
    double maxT = 0.0) noexcept;