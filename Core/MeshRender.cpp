#include "MeshRender.h"
#include "Raycast3D.h"
#include "MathUtils.h"
#include "Player.h"
#include <cmath>
#include <algorithm>
#undef max

// =====================================================================
//  Освещение
//
//  Чистые функции, без ветвлений в горячем пути.
//  Возвращают множитель ~0.4..1.25 в зависимости от нормали.
// =====================================================================

double LightDefault(double nx, double ny, double nz) noexcept {
    constexpr double kL1x = -0.35, kL1y = 0.50, kL1z = 0.80;
    constexpr double kL2x = 0.55, kL2y = -0.20, kL2z = -0.45;
    constexpr double kAmb = 0.42;
    constexpr double kMax = 1.25;

    const double d1 = nx * kL1x + ny * kL1y + nz * kL1z;
    const double d2 = nx * kL2x + ny * kL2y + nz * kL2z;

    const double lit = 0.62 * (d1 > 0.0 ? d1 : 0.0)
        + 0.22 * (d2 > 0.0 ? d2 : 0.0);

    const double light = kAmb + lit;
    return light > kMax ? kMax : light;
}

double LightCharacter(double nx, double ny, double nz) noexcept {
    constexpr double kLx = 0.30, kLy = 0.70, kLz = 0.65;
    constexpr double kAmb = 0.50, kMul = 0.60, kMax = 1.20;

    const double diff = nx * kLx + ny * kLy + nz * kLz;
    const double light = kAmb + kMul * (diff > 0.0 ? diff : 0.0);
    return light > kMax ? kMax : light;
}

double LightFlat(double /*nx*/, double /*ny*/, double /*nz*/) noexcept {
    return 1.0;
}

// =====================================================================
//  Утилита: покрасить пиксель с учётом текстуры и освещения.
// =====================================================================
namespace {

    inline uint8_t ClampByte(int v) noexcept {
        if (v < 0)   return 0;
        if (v > 255) return 255;
        return static_cast<uint8_t>(v);
    }

    inline uint32_t ShadeColor(const Primitive& prim, const PrimHit& hit,
        LightFn lightFn) noexcept
    {
        const uint32_t base = (prim.texId >= 0)
            ? TexBank_Sample(prim.texId, hit.u, hit.v)
            : prim.col;

        const double light = lightFn(hit.nx, hit.ny, hit.nz);

        const int r = static_cast<int>(GetR(base) * light);
        const int g = static_cast<int>(GetG(base) * light);
        const int b = static_cast<int>(GetB(base) * light);

        return RGB(ClampByte(r), ClampByte(g), ClampByte(b));
    }

}  // namespace

// =====================================================================
//  CameraSpace — оружие и другие объекты в системе камеры.
//
//  Параметризация луча: (u, v, 1), origin = (0,0,0).
//  При такой параметризации dir · forward == 1 тождественно,
//  поэтому perp = hit.t — делений не требуется.
//
//  Оптимизации:
//    - xform — уже готов, никакой тригонометрии в цикле.
//    - planeLen — константа FOV, вне цикла.
//    - u вычисляется инкрементом, без деления на каждом шаге.
//    - sy-диапазон сужен: оружие живёт в нижней половине экрана.
// =====================================================================
void RenderMeshCameraSpace(const Mesh& mesh, const MeshXform& xf,
    int screen_left, int view_width, int half_height,
    int max_row_bottom)
{
    if (mesh.count == 0) return;

    const double planeLen = std::tan(FOV * 0.5);
    const int    max_y = WINDOW_HEIGHT - 1;

    // Оружие живёт в нижней половине экрана — экономим ~50% циклов.
    int sy0 = half_height / 2;
    int sy1 = max_row_bottom;
    if (sy1 > max_y) sy1 = max_y;
    if (sy0 > sy1)   return;

    const int sx0 = screen_left;
    const int sx1 = screen_left + view_width - 1;

    // u(sx) = ((2*(sx - screen_left + 0.5) / view_width) - 1) * planeLen
    // Линейно по sx. u(sx0) = u0, шаг — du.
    const double invW = 1.0 / static_cast<double>(view_width);
    const double du = 2.0 * planeLen * invW;
    const double u0 = (2.0 * 0.5 * invW - 1.0) * planeLen;

    const double invH = 1.0 / static_cast<double>(half_height);

    for (int sy = sy0; sy <= sy1; ++sy) {
        const double v = (static_cast<double>(half_height - sy) * invH) * planeLen;

        double u = u0;

        for (int sx = sx0; sx <= sx1; ++sx, u += du) {
            const PrimHit hit = RaycastMesh(mesh, xf, 0.0, 0.0, 0.0, u, v, 1.0);
            if (!hit.hit) continue;

            const Primitive& prim = mesh.parts[hit.primIdx];
            SetPixelFast(sx, sy, ShadeColor(prim, hit, LightDefault));
        }
    }
}

void RenderMeshCameraSpace(const Mesh& mesh,
    int screen_left, int view_width, int half_height,
    int max_row_bottom)
{
    RenderMeshCameraSpace(mesh, BuildXform(mesh),
        screen_left, view_width, half_height, max_row_bottom);
}

// =====================================================================
//  WorldSpace — объект в мире.
//
//  Параметризация:
//    dirX = camSin + cameraX * camCos
//    dirZ = camCos - cameraX * camSin
//    dirY = cameraY
//  Тогда dir · forward == 1, значит perp = hit.t.
//
//  Оптимизации:
//    - xform — готов.
//    - camSin/camCos — один раз.
//    - Радиус меша из AABB (не константа).
//    - Быстрый отсев «позади камеры».
//    - Проекция центра меша + консервативный радиус → сужение sx/sy.
//    - Предварительный отсев столбцов по zbuffer:
//      если минимально возможное расстояние до меша больше
//      глубины стены в столбце — столбец за стеной, пропускаем.
//    - cameraX/cameraY — через инкремент.
// =====================================================================
void RenderMeshWorldSpace(const Mesh& mesh, const MeshXform& xf,
    double camX, double camY, double camZ,
    double camAngle,
    int screen_left, int view_width, int half_height,
    const double* zbuffer)
{
    if (mesh.count == 0) return;

    const double camSin = std::sin(camAngle);
    const double camCos = std::cos(camAngle);
    const double planeLen = std::tan(FOV * 0.5);

    // ---- Отсев «позади камеры» ----
    const double rx = xf.posX - camX;
    const double rz = xf.posZ - camZ;
    const double forwardDist = rx * camSin + rz * camCos;

    // ---- Радиус от origin меша (pos) до дальней точки AABB ----
    // ВАЖНО: не полудиагональ AABB. У персонажа pos = ноги (y=0),
    // а AABB простирается до макушки (y≈1.02) и наплечников (|x|≈0.34).
    // Полудиагональ дала бы ~0.7, а реальный радиус от pos — ~1.12.
    // Именно из-за этого срезалась верхняя половина (голова + торс).
    const double r2 =
        std::max(xf.aabbMinX * xf.aabbMinX, xf.aabbMaxX * xf.aabbMaxX) +
        std::max(xf.aabbMinY * xf.aabbMinY, xf.aabbMaxY * xf.aabbMaxY) +
        std::max(xf.aabbMinZ * xf.aabbMinZ, xf.aabbMaxZ * xf.aabbMaxZ);
    const double localRadius = std::sqrt(r2);
    const double meshRadius = localRadius * xf.scale;

    if (forwardDist < -meshRadius) return;

    // ---- Проекция центра на экран ----
    const bool needFullPath = (forwardDist <= 0.05);

    int sx0 = 0, sx1 = view_width;
    int sy0 = 0, sy1 = WINDOW_HEIGHT;

    if (!needFullPath) {
        const double centerU = (rx * camCos - rz * camSin) / forwardDist;
        const double centerV = (xf.posY - camY) / forwardDist;

        const double rU = meshRadius / forwardDist;
        const double rV = meshRadius / forwardDist;

        const double centerPx = (centerU / planeLen * 0.5 + 0.5)
            * static_cast<double>(view_width);
        const double centerPy = (0.5 - centerV / planeLen * 0.5)
            * static_cast<double>(WINDOW_HEIGHT);

        const double pxRadius = rU / planeLen * 0.5
            * static_cast<double>(view_width) + 4.0;
        const double pyRadius = rV / planeLen * 0.5
            * static_cast<double>(WINDOW_HEIGHT) + 4.0;

        sx0 = static_cast<int>(centerPx - pxRadius);
        sx1 = static_cast<int>(centerPx + pxRadius) + 1;  // +1: полуоткрытый
        sy0 = static_cast<int>(centerPy - pyRadius);
        sy1 = static_cast<int>(centerPy + pyRadius) + 1;

        if (sx0 < 0)             sx0 = 0;
        if (sx1 > view_width)    sx1 = view_width;
        if (sy0 < 0)             sy0 = 0;
        if (sy1 > WINDOW_HEIGHT) sy1 = WINDOW_HEIGHT;

        if (sx0 >= sx1 || sy0 >= sy1) return;
    }

    // ---- Развёртка ----
    const double invW = 1.0 / static_cast<double>(view_width);
    const double invH = 1.0 / static_cast<double>(half_height);

    const double cameraXdX = 2.0 * planeLen * invW;
    const double cameraYdY = -planeLen * invH;

    // Консервативная нижняя граница расстояния до меша вдоль взгляда.
    // forwardDist — проекция, а не расстояние, поэтому для боковых
    // лучей это оценка снизу. meshRadius теперь корректный (от pos),
    // так что отсев безопасен.
    const double minPossibleT = forwardDist - meshRadius;

    for (int x = sx0; x < sx1; ++x) {
        // Предварительный отсев столбца по zbuffer.
        if (minPossibleT > zbuffer[x]) continue;

        const double cameraX = (2.0 * (x + 0.5) * invW - 1.0) * planeLen;
        const double dirX = camSin + cameraX * camCos;
        const double dirZ = camCos - cameraX * camSin;

        double cameraY = static_cast<double>(half_height - sy0) * planeLen * invH;

        for (int sy = sy0; sy < sy1; ++sy, cameraY += cameraYdY) {
            const PrimHit hit = RaycastMesh(mesh, xf,
                camX, camY, camZ, dirX, cameraY, dirZ);
            if (!hit.hit) continue;

            const double perp = hit.t;
            if (perp < 0.05) continue;
            if (perp > zbuffer[x]) continue;

            const Primitive& prim = mesh.parts[hit.primIdx];
            SetPixelFast(screen_left + x, sy,
                ShadeColor(prim, hit, LightCharacter));
        }
    }
}

void RenderMeshWorldSpace(const Mesh& mesh,
    double camX, double camY, double camZ,
    double camAngle,
    int screen_left, int view_width, int half_height,
    const double* zbuffer)
{
    RenderMeshWorldSpace(mesh, BuildXform(mesh),
        camX, camY, camZ, camAngle,
        screen_left, view_width, half_height, zbuffer);
}