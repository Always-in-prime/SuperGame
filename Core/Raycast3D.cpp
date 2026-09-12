#include "Raycast3D.h"
#include <cmath>

namespace {

    // ---------------------------------------------------------------------
    //  Маленькие утилиты
    // ---------------------------------------------------------------------

    constexpr double kEps = 1e-12;
    constexpr double kMinT = 0.05;   // отсекаем пересечения позади/в глазу
    constexpr double kNormalizeEps = 1e-6;

    // ---------------------------------------------------------------------
    //  Решение A*t^2 + B*t + C = 0 при известном A != 0.
    //  Возвращает t0 <= t1 или false. Дискриминант проверяется до sqrt.
    // ---------------------------------------------------------------------
    inline bool SolveQuadratic(double A, double B, double C,
        double& t0, double& t1) noexcept
    {
        const double disc = B * B - 4.0 * A * C;
        if (disc < 0.0) return false;
        const double sq = std::sqrt(disc);
        const double inv2A = 1.0 / (2.0 * A);   // A != 0 гарантирован вызывающим
        t0 = (-B - sq) * inv2A;
        t1 = (-B + sq) * inv2A;
        return true;
    }

    // Ближайший положительный корень. Если origin внутри — берём t1.
    inline double PickPositiveRoot(double t0, double t1) noexcept {
        if (t1 < 0.0) return -1.0;
        return (t0 > 0.0) ? t0 : t1;
    }

    // Нормализация текстуры v в [0,1] без ветвлений в горячем пути.
    inline double Clamp01(double v) noexcept {
        if (v < 0.0) return 0.0;
        if (v > 1.0) return 1.0;
        return v;
    }

    // Заполнить текстурные координаты для цилиндра по углу вокруг оси.
    // Общая логика для всех трёх ориентаций.
    inline double AngleToU(double a, double b) noexcept {
        const double ang = std::atan2(a, b);
        return ang * (0.5 / PI) + 0.5;
    }

    // =====================================================================
    //  Sphere
    // =====================================================================
    PrimHit RaySphere(const Primitive& p,
        double ox, double oy, double oz,
        double dx, double dy, double dz) noexcept
    {
        const double lx = ox - p.cx;
        const double ly = oy - p.cy;
        const double lz = oz - p.cz;

        const double A = dx * dx + dy * dy + dz * dz;
        if (A < kEps) return { false, 0, -1, 0, 0, 0, 0, 0 };

        const double B = 2.0 * (dx * lx + dy * ly + dz * lz);
        const double C = lx * lx + ly * ly + lz * lz - p.r * p.r;

        double t0, t1;
        if (!SolveQuadratic(A, B, C, t0, t1)) return { false, 0, -1, 0, 0, 0, 0, 0 };

        const double t = PickPositiveRoot(t0, t1);
        if (t < kMinT) return { false, 0, -1, 0, 0, 0, 0, 0 };

        PrimHit h;
        h.hit = true;
        h.t = t;

        const double invR = 1.0 / p.r;
        h.nx = ((ox + dx * t) - p.cx) * invR;
        h.ny = ((oy + dy * t) - p.cy) * invR;
        h.nz = ((oz + dz * t) - p.cz) * invR;

        h.u = AngleToU(h.nx, h.nz);
        h.v = h.ny * 0.5 + 0.5;

        h.primIdx = -1;  // заполнит RaycastMesh
        return h;
    }

    // =====================================================================
    //  Cylinder вдоль Y
    // =====================================================================
    PrimHit RayCylY(const Primitive& p,
        double ox, double oy, double oz,
        double dx, double dy, double dz) noexcept
    {
        const double lx = ox - p.cx;
        const double lz = oz - p.cz;

        const double A = dx * dx + dz * dz;
        if (A < kEps) return { false, 0, -1, 0, 0, 0, 0, 0 };

        const double B = 2.0 * (dx * lx + dz * lz);
        const double C = lx * lx + lz * lz - p.r * p.r;

        double t0, t1;
        if (!SolveQuadratic(A, B, C, t0, t1)) return { false, 0, -1, 0, 0, 0, 0, 0 };

        // Первый положительный корень, попадающий в диапазон по Y.
        double t = -1.0;
        for (int k = 0; k < 2; ++k) {
            const double tc = (k == 0) ? t0 : t1;
            if (tc < kMinT) continue;
            const double py = oy + dy * tc;
            if (py < p.aMin || py > p.aMax) continue;
            t = tc;
            break;
        }
        if (t < 0.0) return { false, 0, -1, 0, 0, 0, 0, 0 };

        PrimHit h;
        h.hit = true;
        h.t = t;

        const double hx = (ox + dx * t) - p.cx;
        const double hz = (oz + dz * t) - p.cz;
        const double nlen2 = hx * hx + hz * hz;
        if (nlen2 > kNormalizeEps) {
            const double invN = 1.0 / std::sqrt(nlen2);
            h.nx = hx * invN;
            h.nz = hz * invN;
        }
        else {
            h.nx = 0.0;
            h.nz = 0.0;
        }
        h.ny = 0.0;

        h.u = AngleToU(h.nx, h.nz);

        const double py = oy + dy * t;
        const double invRange = 1.0 / (p.aMax - p.aMin);
        h.v = Clamp01((py - p.aMin) * invRange);

        h.primIdx = -1;
        return h;
    }

    // =====================================================================
    //  Cylinder вдоль Z
    // =====================================================================
    PrimHit RayCylZ(const Primitive& p,
        double ox, double oy, double oz,
        double dx, double dy, double dz) noexcept
    {
        const double lx = ox - p.cx;
        const double ly = oy - p.cy;

        const double A = dx * dx + dy * dy;
        if (A < kEps) return { false, 0, -1, 0, 0, 0, 0, 0 };

        const double B = 2.0 * (dx * lx + dy * ly);
        const double C = lx * lx + ly * ly - p.r * p.r;

        double t0, t1;
        if (!SolveQuadratic(A, B, C, t0, t1)) return { false, 0, -1, 0, 0, 0, 0, 0 };

        double t = -1.0;
        for (int k = 0; k < 2; ++k) {
            const double tc = (k == 0) ? t0 : t1;
            if (tc < kMinT) continue;
            const double pz = oz + dz * tc;
            if (pz < p.aMin || pz > p.aMax) continue;
            t = tc;
            break;
        }
        if (t < 0.0) return { false, 0, -1, 0, 0, 0, 0, 0 };

        PrimHit h;
        h.hit = true;
        h.t = t;

        const double hx = (ox + dx * t) - p.cx;
        const double hy = (oy + dy * t) - p.cy;
        const double nlen2 = hx * hx + hy * hy;
        if (nlen2 > kNormalizeEps) {
            const double invN = 1.0 / std::sqrt(nlen2);
            h.nx = hx * invN;
            h.ny = hy * invN;
        }
        else {
            h.nx = 0.0;
            h.ny = 0.0;
        }
        h.nz = 0.0;

        h.u = AngleToU(h.nx, h.ny);

        const double pz = oz + dz * t;
        const double invRange = 1.0 / (p.aMax - p.aMin);
        h.v = Clamp01((pz - p.aMin) * invRange);

        h.primIdx = -1;
        return h;
    }

    // =====================================================================
    //  Cylinder вдоль X
    // =====================================================================
    PrimHit RayCylX(const Primitive& p,
        double ox, double oy, double oz,
        double dx, double dy, double dz) noexcept
    {
        const double ly = oy - p.cy;
        const double lz = oz - p.cz;

        const double A = dy * dy + dz * dz;
        if (A < kEps) return { false, 0, -1, 0, 0, 0, 0, 0 };

        const double B = 2.0 * (dy * ly + dz * lz);
        const double C = ly * ly + lz * lz - p.r * p.r;

        double t0, t1;
        if (!SolveQuadratic(A, B, C, t0, t1)) return { false, 0, -1, 0, 0, 0, 0, 0 };

        double t = -1.0;
        for (int k = 0; k < 2; ++k) {
            const double tc = (k == 0) ? t0 : t1;
            if (tc < kMinT) continue;
            const double px = ox + dx * tc;
            if (px < p.aMin || px > p.aMax) continue;
            t = tc;
            break;
        }
        if (t < 0.0) return { false, 0, -1, 0, 0, 0, 0, 0 };

        PrimHit h;
        h.hit = true;
        h.t = t;

        const double hy = (oy + dy * t) - p.cy;
        const double hz = (oz + dz * t) - p.cz;
        const double nlen2 = hy * hy + hz * hz;
        if (nlen2 > kNormalizeEps) {
            const double invN = 1.0 / std::sqrt(nlen2);
            h.ny = hy * invN;
            h.nz = hz * invN;
        }
        else {
            h.ny = 0.0;
            h.nz = 0.0;
        }
        h.nx = 0.0;

        h.u = AngleToU(h.ny, h.nz);

        const double px = ox + dx * t;
        const double invRange = 1.0 / (p.aMax - p.aMin);
        h.v = Clamp01((px - p.aMin) * invRange);

        h.primIdx = -1;
        return h;
    }

    // =====================================================================
    //  Dispatch по типу примитива
    // =====================================================================
    inline PrimHit DispatchPrimitive(const Primitive& p,
        double ox, double oy, double oz,
        double dx, double dy, double dz) noexcept
    {
        switch (p.kind) {
        case PRIM_SPHERE: return RaySphere(p, ox, oy, oz, dx, dy, dz);
        case PRIM_CYL_Y:  return RayCylY(p, ox, oy, oz, dx, dy, dz);
        case PRIM_CYL_Z:  return RayCylZ(p, ox, oy, oz, dx, dy, dz);
        case PRIM_CYL_X:  return RayCylX(p, ox, oy, oz, dx, dy, dz);
        }
        return { false, 0, -1, 0, 0, 0, 0, 0 };
    }

    // ---------------------------------------------------------------------
    //  Оценка AABB примитива в локальной системе.
    //  Используется для broadphase: если луч не пересекает AABB меша
    //  в локальных координатах — можно не перебирать примитивы.
    //
    //  Здесь только тест «луч vs AABB». Полноценный AABB меша
    //  считается один раз при сборке меша (см. заметки ниже).
    // ---------------------------------------------------------------------

}  // namespace

// =====================================================================
//  Публичный API
// =====================================================================

PrimHit RaycastPrimitive(const Primitive& p,
    double ox, double oy, double oz,
    double dx, double dy, double dz) noexcept
{
    return DispatchPrimitive(p, ox, oy, oz, dx, dy, dz);
}

PrimHit RaycastMesh(const Mesh& m, const MeshXform& xf,
    double ox, double oy, double oz,
    double dx, double dy, double dz,
    double maxT) noexcept
{
    if (m.count == 0) return { false, 0, -1, 0, 0, 0, 0, 0 };

    // ---- Перевод луча в локальную систему меша ----
    // Обратный поворот R(-angle) + масштабирование 1/scale.
    // cosA, sinA — предпосчитанные cos(angle), sin(angle).

    const double relX = ox - xf.posX;
    const double relY = oy - xf.posY;
    const double relZ = oz - xf.posZ;

    const double c = xf.cosA;
    const double s = xf.sinA;
    const double invS = xf.invScale;

    // R(-a) * (x, z), затем scale — всё в одном выражении, без мутаций.
    const double lox = (relX * c + relZ * s) * invS;
    const double loy = relY * invS;
    const double loz = (-relX * s + relZ * c) * invS;

    const double ldx = (dx * c + dz * s) * invS;
    const double ldy = dy * invS;
    const double ldz = (-dx * s + dz * c) * invS;

    // ---- Перебор примитивов ----
    // maxT задан в мировых единицах. Так как локальное направление
    // укорочено в scale раз относительно мирового, параметр вдоль
    // локального луча в scale раз больше при том же мировом t.
    // Значит t_local_max = maxT * scale.
    PrimHit best{ false, 0, -1, 0, 0, 0, 0, 0 };
    const double bestT0 = (maxT > 0.0) ? maxT * xf.scale : 1e30;
    double bestT = bestT0;

    for (int i = 0; i < m.count; ++i) {
        PrimHit ph = DispatchPrimitive(m.parts[i], lox, loy, loz, ldx, ldy, ldz);
        if (!ph.hit) continue;
        if (ph.t >= bestT) continue;

        best = ph;
        best.primIdx = i;
        bestT = ph.t;
    }

    if (!best.hit) return best;

    // ---- Возврат в мировую систему ----
    // t_local посчитан вдоль укороченного локального направления.
    // Мировой параметр = t_local * scale (масштаб применялся через invS).
    best.t *= xf.scale;

    // Нормаль: обратный поворот R(+angle) в мир.
    const double wnx = best.nx * c - best.nz * s;
    const double wnz = best.nx * s + best.nz * c;
    best.nx = wnx;
    best.nz = wnz;
    // best.ny не меняется.

    return best;
}

PrimHit RaycastMesh(const Mesh& m,
    double ox, double oy, double oz,
    double dx, double dy, double dz,
    double maxT) noexcept
{
    return RaycastMesh(m, BuildXform(m),
        ox, oy, oz, dx, dy, dz, maxT);
}