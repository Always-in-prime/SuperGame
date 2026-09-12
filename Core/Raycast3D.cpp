#include "Raycast3D.h"
#include <cmath>

// ---------------------------------------------------------------------
//  Общие утилиты
// ---------------------------------------------------------------------
static inline double SafeSqrt(double x) {
    return (x <= 0.0) ? 0.0 : std::sqrt(x);
}

// Решение квадратного уравнения A*t^2 + B*t + C = 0.
// Возвращает t0 <= t1 или false.
static bool SolveQuadratic(double A, double B, double C,
    double& t0, double& t1)
{
    if (std::fabs(A) < 1e-12) return false;
    double disc = B * B - 4.0 * A * C;
    if (disc < 0.0) return false;
    double sq = std::sqrt(disc);
    t0 = (-B - sq) / (2.0 * A);
    t1 = (-B + sq) / (2.0 * A);
    return true;
}

// Выбор ближайшего положительного корня.
// Если origin внутри, t0 < 0 < t1 — берём t1.
static double PickPositiveRoot(double t0, double t1) {
    if (t1 < 0.0) return -1.0;
    if (t0 > 0.0) return t0;
    return t1;
}

// ---------------------------------------------------------------------
//  Sphere
// ---------------------------------------------------------------------
static PrimHit RaySphere(const Primitive& p,
    double ox, double oy, double oz,
    double dx, double dy, double dz)
{
    PrimHit h = { false, 0, -1, 0, 0, 0, 0, 0 };

    double lx = ox - p.cx;
    double ly = oy - p.cy;
    double lz = oz - p.cz;

    double A = dx * dx + dy * dy + dz * dz;
    double B = 2.0 * (dx * lx + dy * ly + dz * lz);
    double C = lx * lx + ly * ly + lz * lz - p.r * p.r;

    double t0, t1;
    if (!SolveQuadratic(A, B, C, t0, t1)) return h;

    double t = PickPositiveRoot(t0, t1);
    if (t < 0.0 || t < 0.05) return h;

    h.hit = true;
    h.t = t;

    // Точка попадания в локальной системе примитива
    double px = (ox + dx * t) - p.cx;
    double py = (oy + dy * t) - p.cy;
    double pz = (oz + dz * t) - p.cz;

    // Нормаль = нормализованная точка попадания
    double invR = 1.0 / p.r;
    h.nx = px * invR;
    h.ny = py * invR;
    h.nz = pz * invR;

    // Сферические текстурные координаты:
    //   u — азимут (0..1)
    //   v — широта (0 — низ, 1 — верх)
    double az = std::atan2(h.nx, h.nz);       // -π..π
    h.u = az / (2.0 * PI) + 0.5;              // 0..1
    h.v = h.ny * 0.5 + 0.5;                   // 0..1

    return h;
}

// ---------------------------------------------------------------------
//  Cylinder вдоль Y: ось параллельна Y.
//  В плоскости XZ — окружность радиуса r.
// ---------------------------------------------------------------------
static PrimHit RayCylY(const Primitive& p,
    double ox, double oy, double oz,
    double dx, double dy, double dz)
{
    PrimHit h = { false, 0, -1, 0, 0, 0, 0, 0 };

    double lx = ox - p.cx;
    double lz = oz - p.cz;

    // 2D-пересечение с окружностью в плоскости XZ.
    double A = dx * dx + dz * dz;
    double B = 2.0 * (dx * lx + dz * lz);
    double C = lx * lx + lz * lz - p.r * p.r;

    double t0, t1;
    if (!SolveQuadratic(A, B, C, t0, t1)) return h;

    // Обрабатываем оба корня: нужен первый, попадающий
    // в диапазон y ∈ [aMin, aMax].
    double cand[2] = { t0, t1 };
    for (int k = 0; k < 2; ++k) {
        double t = cand[k];
        if (t < 0.05) continue;

        double py = oy + dy * t;
        if (py < p.aMin || py > p.aMax) continue;

        h.hit = true;
        h.t = t;
        break;
    }
    if (!h.hit) return h;

    // Точка и нормаль
    double hx = (ox + dx * h.t) - p.cx;
    double hz = (oz + dz * h.t) - p.cz;
    double nlen = std::sqrt(hx * hx + hz * hz);
    if (nlen > 1e-6) { h.nx = hx / nlen; h.nz = hz / nlen; }
    h.ny = 0.0;

    // Текстурные координаты:
    //   u — угол вокруг оси
    //   v — по высоте цилиндра
    double ang = std::atan2(h.nx, h.nz);
    h.u = ang / (2.0 * PI) + 0.5;
    double py = oy + dy * h.t;
    h.v = (py - p.aMin) / (p.aMax - p.aMin);
    if (h.v < 0.0) h.v = 0.0;
    if (h.v > 1.0) h.v = 1.0;

    return h;
}

// ---------------------------------------------------------------------
//  Cylinder вдоль Z: ось параллельна Z.
//  В плоскости XY — окружность радиуса r.
// ---------------------------------------------------------------------
static PrimHit RayCylZ(const Primitive& p,
    double ox, double oy, double oz,
    double dx, double dy, double dz)
{
    PrimHit h = { false, 0, -1, 0, 0, 0, 0, 0 };

    double lx = ox - p.cx;
    double ly = oy - p.cy;

    double A = dx * dx + dy * dy;
    double B = 2.0 * (dx * lx + dy * ly);
    double C = lx * lx + ly * ly - p.r * p.r;

    double t0, t1;
    if (!SolveQuadratic(A, B, C, t0, t1)) return h;

    double cand[2] = { t0, t1 };
    for (int k = 0; k < 2; ++k) {
        double t = cand[k];
        if (t < 0.05) continue;

        double pz = oz + dz * t;
        if (pz < p.aMin || pz > p.aMax) continue;

        h.hit = true;
        h.t = t;
        break;
    }
    if (!h.hit) return h;

    double hx = (ox + dx * h.t) - p.cx;
    double hy = (oy + dy * h.t) - p.cy;
    double nlen = std::sqrt(hx * hx + hy * hy);
    if (nlen > 1e-6) { h.nx = hx / nlen; h.ny = hy / nlen; }
    h.nz = 0.0;

    double ang = std::atan2(h.nx, h.ny);
    h.u = ang / (2.0 * PI) + 0.5;
    double pz = oz + dz * h.t;
    h.v = (pz - p.aMin) / (p.aMax - p.aMin);
    if (h.v < 0.0) h.v = 0.0;
    if (h.v > 1.0) h.v = 1.0;

    return h;
}

// ---------------------------------------------------------------------
//  Cylinder вдоль X: ось параллельна X.
//  В плоскости YZ — окружность радиуса r.
// ---------------------------------------------------------------------
static PrimHit RayCylX(const Primitive& p,
    double ox, double oy, double oz,
    double dx, double dy, double dz)
{
    PrimHit h = { false, 0, -1, 0, 0, 0, 0, 0 };

    double ly = oy - p.cy;
    double lz = oz - p.cz;

    double A = dy * dy + dz * dz;
    double B = 2.0 * (dy * ly + dz * lz);
    double C = ly * ly + lz * lz - p.r * p.r;

    double t0, t1;
    if (!SolveQuadratic(A, B, C, t0, t1)) return h;

    double cand[2] = { t0, t1 };
    for (int k = 0; k < 2; ++k) {
        double t = cand[k];
        if (t < 0.05) continue;

        double px = ox + dx * t;
        if (px < p.aMin || px > p.aMax) continue;

        h.hit = true;
        h.t = t;
        break;
    }
    if (!h.hit) return h;

    double hy = (oy + dy * h.t) - p.cy;
    double hz = (oz + dz * h.t) - p.cz;
    double nlen = std::sqrt(hy * hy + hz * hz);
    if (nlen > 1e-6) { h.ny = hy / nlen; h.nz = hz / nlen; }
    h.nx = 0.0;

    double ang = std::atan2(h.ny, h.nz);
    h.u = ang / (2.0 * PI) + 0.5;
    double px = ox + dx * h.t;
    h.v = (px - p.aMin) / (p.aMax - p.aMin);
    if (h.v < 0.0) h.v = 0.0;
    if (h.v > 1.0) h.v = 1.0;

    return h;
}

// =====================================================================
PrimHit RaycastPrimitive(const Primitive& p,
    double ox, double oy, double oz,
    double dx, double dy, double dz)
{
    switch (p.kind) {
    case PRIM_SPHERE: return RaySphere(p, ox, oy, oz, dx, dy, dz);
    case PRIM_CYL_Y:  return RayCylY(p, ox, oy, oz, dx, dy, dz);
    case PRIM_CYL_Z:  return RayCylZ(p, ox, oy, oz, dx, dy, dz);
    case PRIM_CYL_X:  return RayCylX(p, ox, oy, oz, dx, dy, dz);
    }
    PrimHit none = { false, 0, -1, 0, 0, 0, 0, 0 };
    return none;
}

// =====================================================================
PrimHit RaycastMesh(const Mesh& m,
    double ox, double oy, double oz,
    double dx, double dy, double dz,
    double maxT)
{
    PrimHit best = { false, 0, -1, 0, 0, 0, 0, 0 };
    double bestT = (maxT > 0.0) ? maxT : 1e30;

    // Луч нужно привести в ЛОКАЛЬНУЮ систему меша:
    // вычесть позицию, повернуть на -angle, учесть scale.
    double ca = std::cos(-m.angle);
    double sa = std::sin(-m.angle);

    // Позиция луча относительно меша (с учётом общих смещений).
    double relX = ox - (m.posX + m.offX);
    double relY = oy - (m.posY + m.offY);
    double relZ = oz - (m.posZ + m.offZ);

    // Поворот вокруг Y.
    double lox = relX * ca - relZ * sa;
    double loz = relX * sa + relZ * ca;
    double loy = relY;

    // Масштаб (если есть).
    double s = (m.scale > 1e-6) ? m.scale : 1.0;
    double invS = 1.0 / s;
    lox *= invS; loy *= invS; loz *= invS;

    // Направление — тоже поворачиваем (scale для нормализованного луча
    // не важен, только поворот).
    double ldx = dx * ca - dz * sa;
    double ldz = dx * sa + dz * ca;
    double ldy = dy;

    // Проходим по всем примитивам, ищем ближайшее.
    for (int i = 0; i < m.count; ++i) {
        PrimHit ph = RaycastPrimitive(m.parts[i], lox, loy, loz, ldx, ldy, ldz);
        if (!ph.hit) continue;
        if (ph.t >= bestT) continue;

        best = ph;
        best.primIdx = i;
        bestT = ph.t;
    }

    if (!best.hit) return best;

    // Расстояние в мировых координатах: t было посчитано
    // в локальных (делим на scale), возвращаем в мировых.
    best.t *= s;

    // Нормаль поворачиваем обратно в мировую систему.
    double wca = std::cos(m.angle);
    double wsa = std::sin(m.angle);
    double wnx = best.nx * wca - best.nz * wsa;
    double wnz = best.nx * wsa + best.nz * wca;
    best.nx = wnx;
    best.ny = best.ny;
    best.nz = wnz;

    return best;
}