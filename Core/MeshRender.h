#pragma once
#include "Mesh.h"
#include "TextureBank.h"
#include "../Render/Framebuffer.h"

// ============================================================
//  ≈диный рендер меша в framebuffer.
//
//  ƒва режима:
//    - CameraSpace: меш задан в системе камеры (оружие от 1-го лица).
//    - WorldSpace : меш задан в мире (персонаж, пикапы, декор).
//
//  ќба используют общий RaycastMesh из Raycast3D.h.
// ============================================================

// ‘ункци€ освещени€. ¬озвращает множитель 0..2.
typedef double (*LightFn)(double nx, double ny, double nz);

double LightDefault(double nx, double ny, double nz);   // дл€ оружи€
double LightCharacter(double nx, double ny, double nz); // дл€ персонажей
double LightFlat(double nx, double ny, double nz);      // без освещени€

// ---------------------------------------------------------------------
//  CameraSpace Ч оружие и другие объекты в системе камеры.
//
//   оординаты меша: origin в глазах наблюдател€, +Y вверх, +Z вперЄд.
//  Ћуч пускаетс€ из (0,0,0) в направлении (u, v, 1).
// ---------------------------------------------------------------------
void RenderMeshCameraSpace(const Mesh& mesh,
    int screen_left, int view_width, int half_height,
    int max_row_bottom);   // нижн€€ граница (дл€ оружи€)

// ---------------------------------------------------------------------
//  WorldSpace Ч объекты в мире.
//
//  ћеш имеет posX/posY/posZ/angle/scale Ч реальные мировые координаты.
//  camX, camY, camZ, camAngle Ч позици€ и поворот наблюдател€.
//  zbuffer Ч дл€ перекрыти€ стен.
// ---------------------------------------------------------------------
void RenderMeshWorldSpace(const Mesh& mesh,
    double camX, double camY, double camZ,
    double camAngle,
    int screen_left, int view_width, int half_height,
    const double* zbuffer);