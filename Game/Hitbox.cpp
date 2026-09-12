#include "Hitbox.h"
#include "../Core/Raycast3D.h"
#include <cmath>

// =====================================================================
//  RaycastCharacterMesh
//
//  Луч пускается в мировой системе (X, Z). Высоты три:
//  ноги, торс, голова. Поскольку луч плоский (нет pitch),
//  одного луча недостаточно — нужны лучи на разных высотах,
//  иначе попадём только в торс.
//
//  Формат передачи в RaycastMesh:
//    ox     — мировой X
//    rayY   — высота (Y в 3D-системе)
//    oy     — мировой Y (Z в 3D-системе)
//    dx,0,dy — направление луча (dy = 0 — плоский shot)
// =====================================================================
HitInfo RaycastCharacterMesh(const Mesh& mesh,
    double ox, double oy,
    double dx, double dy)
{
    HitInfo out = { false, 0.0, -1, 1.0 };

    // Проверяем три высоты: ноги (0.2), торс (0.5), голова (0.85).
    double eyeHeights[] = { 0.5, 0.85, 0.2 };
    const int nRays = 3;

    double bestT = 1e30;
    int    bestIdx = -1;
    double bestDmg = 1.0;

    for (int r = 0; r < nRays; ++r) {
        double rayY = eyeHeights[r];

        PrimHit ph = RaycastMesh(mesh,
            ox, rayY, oy,          // X, высота, мировой Y(=Z)
            dx, 0.0, dy);          // направление в X, 0, Z
        if (!ph.hit) continue;

        // Нормируем длину луча, потому что dx, dy не обязательно единичны
        // (в Combat.cpp они sin/cos, но пусть будет надёжно).
        double len = sqrt(dx * dx + dy * dy);
        if (len < 1e-9) continue;
        double distXZ = ph.t / len;

        if (distXZ >= bestT) continue;

        bestT = distXZ;
        bestIdx = ph.primIdx;
        bestDmg = mesh.parts[ph.primIdx].damageMult;
    }

    if (bestIdx < 0) return out;

    out.hit = true;
    out.distance = bestT;
    out.partIdx = bestIdx;
    out.damageMult = bestDmg;
    return out;
}