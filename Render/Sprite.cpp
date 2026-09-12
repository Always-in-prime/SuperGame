#include "Sprite.h"
#include "../Game/CharacterModel.h"
#include "../Core/MeshRender.h"
#include <cmath>

// =====================================================================
//  Рендер противника.
// =====================================================================
void RenderOtherPlayer(const Player& viewer, const Player& target,
    int screen_left, int view_width, int half_height, const double* zbuffer)
{
    if (target.deathTimer > 0) return;

    // Быстрый отсев по FOV
    double dx = target.renderX - viewer.renderX;
    double dy = target.renderY - viewer.renderY;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.30) return;

    double angle_to = atan2(dx, dy);
    double angle_diff = angle_to - viewer.renderAngle;
    while (angle_diff > PI) angle_diff -= 2.0 * PI;
    while (angle_diff < -PI) angle_diff += 2.0 * PI;
    if (fabs(angle_diff) > FOV * 0.5 + 0.6) return;

    // Собираем меш
    Mesh mesh;
    BuildCharacterMesh(mesh, target);

    // Рендерим в WorldSpace. Камера — на высоте глаз.
    RenderMeshWorldSpace(mesh,
        viewer.renderX, EYE_HEIGHT, viewer.renderY,
        viewer.renderAngle,
        screen_left, view_width, half_height,
        zbuffer);
}