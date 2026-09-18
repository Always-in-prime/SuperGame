#include "Weapon.h"
#include "../Game/WeaponModel.h"
#include "../Core/MeshRender.h"
#include <cmath>

// =====================================================================
//  Рендер оружия в camera-space.
//  RenderMeshCameraSpace уже отсекает верхнюю половину экрана —
//  оружие всегда в нижней половине вида.
// =====================================================================
void RenderWeapon(Player& p, int screen_right) {
    if (p.deathTimer > 0) return;

    const int screen_left = screen_right - WINDOW_WIDTH / 2;
    const int view_width = WINDOW_WIDTH / 2;
    const int half_h = WINDOW_HEIGHT / 2;

    // Отдача
    double recoil = 0.0;
    if (p.muzzleFlash > 0.0f) {
        recoil = (double)p.muzzleFlash / (double)MUZZLE_TIME;
        if (recoil > 1.0) recoil = 1.0;
        if (recoil < 0.0) recoil = 0.0;
    }

    // Покачивание при ходьбе
    double bobX = sin(p.walkPhase) * 0.009;
    double bobZ = fabs(cos(p.walkPhase)) * 0.007;

    // Собираем меш
    Mesh mesh;
    BuildWeaponMesh(mesh, recoil, bobX, bobZ);

    // Рендерим в CameraSpace
    RenderMeshCameraSpace(mesh, screen_left, view_width, half_h,
        WINDOW_HEIGHT - 1);

    // Дульная вспышка (отдельно, как оверлей)
    if (p.muzzleFlash > 0.0f) {
        double mx = 0.105 + mesh.offX;
        double my = -0.120 + mesh.offY;
        double mz = 1.72 + mesh.offZ;

        double planeLen = tan(FOV * 0.5);
        double inv = 1.0 / mz;
        double nu = (mx * inv) / planeLen;
        double nv = (my * inv) / planeLen;

        int fsx = screen_left + (int)((nu * 0.5 + 0.5) * view_width);
        int fsy = half_h - (int)(nv * half_h);

        // Большое свечение
        int rad = 16 + rand() % 6;
        for (int dy = -rad; dy <= rad; ++dy) {
            for (int dx = -rad; dx <= rad; ++dx) {
                int px = fsx + dx;
                int py = fsy + dy;
                if (px < 0 || px >= WINDOW_WIDTH) continue;
                if (py < 0 || py >= WINDOW_HEIGHT) continue;
                int d2 = dx * dx + dy * dy;
                if (d2 > rad * rad) continue;
                double t = (double)d2 / (double)(rad * rad);
                int alpha = (int)(220.0 * (1.0 - t));
                if (alpha < 0) alpha = 0;
                BlendPixelFast(px, py, RGB(255, 150, 50), alpha);
            }
        }

        // Яркое ядро
        int r2 = 6 + rand() % 3;
        for (int dy = -r2; dy <= r2; ++dy) {
            for (int dx = -r2; dx <= r2; ++dx) {
                int px = fsx + dx;
                int py = fsy + dy;
                if (px < 0 || px >= WINDOW_WIDTH) continue;
                if (py < 0 || py >= WINDOW_HEIGHT) continue;
                if (dx * dx + dy * dy <= r2 * r2) {
                    SetPixelFast(px, py, RGB(255, 250, 220));
                }
            }
        }
    }
}