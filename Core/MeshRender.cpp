#include "MeshRender.h"
#include "Raycast3D.h"
#include "MathUtils.h"
#include "Player.h"
#include <cmath>

// ---------------------------------------------------------------------
//  Освещение
// ---------------------------------------------------------------------
double LightDefault(double nx, double ny, double nz) {
    const double L1x = -0.35, L1y = 0.50, L1z = 0.80;
    double d1 = nx * L1x + ny * L1y + nz * L1z;
    if (d1 < 0) d1 = 0;

    const double L2x = 0.55, L2y = -0.20, L2z = -0.45;
    double d2 = nx * L2x + ny * L2y + nz * L2z;
    if (d2 < 0) d2 = 0;

    double light = 0.42 + 0.62 * d1 + 0.22 * d2;
    if (light > 1.25) light = 1.25;
    return light;
}

double LightCharacter(double nx, double ny, double nz) {
    const double Lx = 0.30, Ly = 0.70, Lz = 0.65;
    double diff = nx * Lx + ny * Ly + nz * Lz;
    if (diff < 0) diff = 0;

    double light = 0.50 + 0.60 * diff;
    if (light > 1.20) light = 1.20;
    return light;
}

double LightFlat(double nx, double ny, double nz) {
    (void)nx; (void)ny; (void)nz;
    return 1.0;
}

// ---------------------------------------------------------------------
//  Утилита: покрасить пиксель с учётом текстуры и освещения
// ---------------------------------------------------------------------
static uint32_t ShadeColor(const Primitive& prim, const PrimHit& hit,
    LightFn lightFn)
{
    uint32_t base = (prim.texId >= 0)
        ? TexBank_Sample(prim.texId, hit.u, hit.v)
        : prim.col;

    double light = lightFn(hit.nx, hit.ny, hit.nz);

    int r = (int)(GetRValue(base) * light);
    int g = (int)(GetGValue(base) * light);
    int b = (int)(GetBValue(base) * light);
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return RGB(r, g, b);
}

// =====================================================================
//  CameraSpace — оружие и другие объекты в системе камеры.
//
//  Луч: (u, v, 1), где
//    u — гориз. положение (cameraX, -planeLen..+planeLen)
//    v — верт. положение (cameraY, -planeLen..+planeLen)
//    1 — фиксированная "вперёд".
//
//  При такой параметризации dir · forward == 1 тождественно,
//  поэтому perp = hit.t (расстояние вдоль forward).
// =====================================================================
void RenderMeshCameraSpace(const Mesh& mesh,
    int screen_left, int view_width, int half_height,
    int max_row_bottom)
{
    if (mesh.count == 0) return;

    const double planeLen = tan(FOV * 0.5);
    const int max_y = WINDOW_HEIGHT - 1;

    // Оружие занимает нижнюю половину экрана — экономим циклы.
    int sy0 = half_height / 2;
    int sy1 = max_row_bottom;
    if (sy1 > max_y) sy1 = max_y;
    if (sy0 > sy1) return;

    const int sx0 = screen_left;
    const int sx1 = screen_left + view_width - 1;

    for (int sy = sy0; sy <= sy1; ++sy) {
        // v: центр экрана = 0, вниз — отрицательное
        double v = ((double)(half_height - sy) / (double)half_height) * planeLen;

        for (int sx = sx0; sx <= sx1; ++sx) {
            double u = ((2.0 * (sx - screen_left + 0.5) / (double)view_width) - 1.0)
                * planeLen;

            // Луч в camera-space.
            double dx = u;
            double dy = v;
            double dz = 1.0;

            PrimHit hit = RaycastMesh(mesh, 0.0, 0.0, 0.0, dx, dy, dz);
            if (!hit.hit) continue;

            const Primitive& prim = mesh.parts[hit.primIdx];
            uint32_t col = ShadeColor(prim, hit, LightDefault);
            SetPixelFast(sx, sy, col);
        }
    }
}

// =====================================================================
//  WorldSpace — объект в мире.
//
//  Луч: (dirX, cameraY, dirZ), где
//    dirX = camSin + cameraX*camCos
//    dirZ = camCos - cameraX*camSin
//    cameraY — экранный Y, влияет на вертикальное направление.
//
//  При такой параметризации:
//    dir · forward = camSin*(camSin + u*camCos) +
//                    camCos*(camCos - u*camSin) == 1,
//  значит perp = hit.t.
// =====================================================================
void RenderMeshWorldSpace(const Mesh& mesh,
    double camX, double camY, double camZ,
    double camAngle,
    int screen_left, int view_width, int half_height,
    const double* zbuffer)
{
    if (mesh.count == 0) return;

    // Быстрый отсев: меш позади камеры?
    double rx = mesh.posX - camX;
    double rz = mesh.posZ - camZ;
    double camSin = sin(camAngle);
    double camCos = cos(camAngle);
    double along = rx * camSin + rz * camCos;
    if (along < -1.0) return;

    const double planeLen = tan(FOV * 0.5);

    for (int x = 0; x < view_width; ++x) {
        double cameraX = (2.0 * (x + 0.5) / (double)view_width - 1.0) * planeLen;

        // Горизонтальная часть луча.
        double dirX = camSin + cameraX * camCos;
        double dirZ = camCos - cameraX * camSin;

        // Для каждой строки экрана пускаем отдельный 3D-луч,
        // чтобы получить полноценный силуэт (без "пунктира").
        for (int sy = 0; sy < WINDOW_HEIGHT; ++sy) {
            double cameraY = ((double)(half_height - sy) / (double)half_height)
                * planeLen;

            double dx = dirX;
            double dy = cameraY;
            double dz = dirZ;

            PrimHit hit = RaycastMesh(mesh, camX, camY, camZ, dx, dy, dz);
            if (!hit.hit) continue;

            // В этой параметризации hit.t — уже перпендикулярное расстояние
            // вдоль forward. Никаких делений не нужно.
            double perp = hit.t;
            if (perp < 0.05) continue;
            if (perp > zbuffer[x]) continue;

            const Primitive& prim = mesh.parts[hit.primIdx];
            uint32_t col = ShadeColor(prim, hit, LightCharacter);
            SetPixelFast(screen_left + x, sy, col);
        }
    }
}