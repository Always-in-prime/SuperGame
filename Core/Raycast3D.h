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
//      BuildXform(mesh) и переиспользуется для всех лучей. Это
//      убирает 4 тригонометрии с каждого пикселя.
//    - Ранний выход по AABB меша в локальной системе.
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

    // Кэш для быстрого «центр меша в мире» — используется для
    // консервативного AABB-отсева в вызывающем коде.
    double invScaleSq;        // invScale * invScale
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