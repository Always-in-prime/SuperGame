#pragma once
#include "Mesh.h"
#include "Raycast3D.h"
#include "TextureBank.h"
#include "../Render/Framebuffer.h"
#include <cstdint>

// ============================================================
//  ≈диный рендер меша в framebuffer.
//
//  ƒва режима:
//    - CameraSpace Ч оружие и объекты в системе камеры.
//    - WorldSpace  Ч объекты в мире (персонажи, пикапы, декор).
//
//   лючевой инвариант:
//    MeshXform строитс€ ќƒ»Ќ раз на кадр на объект и передаЄтс€
//    в RaycastMesh. Ёто убирает 4 тригонометрии с каждого пиксел€.
//
//  ѕубличный API принимает готовый MeshXform (быстра€ верси€)
//  либо Mesh (back-compat, строит xform внутри).
// ============================================================

// ‘ункци€ освещени€. ¬озвращает множитель ~0.4..1.25.
using LightFn = double (*)(double nx, double ny, double nz);

double LightDefault(double nx, double ny, double nz) noexcept;
double LightCharacter(double nx, double ny, double nz) noexcept;
double LightFlat(double nx, double ny, double nz) noexcept;

// ---------------------------------------------------------------------
//  CameraSpace Ч оружие и другие объекты в системе камеры.
//
//   оординаты меша: origin в глазах наблюдател€, +Y вверх, +Z вперЄд.
//  Ћуч пускаетс€ из (0,0,0) в направлении (u, v, 1).
//
//  viewport:
//    screen_left    Ч X левого кра€ вьюпорта в окне
//    view_width     Ч ширина вьюпорта в пиксел€х
//    half_height    Ч половина высоты окна (центр по Y)
//    max_row_bottom Ч нижн€€ граница отрисовки (дл€ оружи€)
//
//  Ѕыстра€ верси€ Ч принимает готовый xform.
// ---------------------------------------------------------------------
void RenderMeshCameraSpace(const Mesh& mesh, const MeshXform& xf,
    int screen_left, int view_width, int half_height,
    int max_row_bottom);

// Back-compat overload: строит xform внутри (2 тригонометрии за вызов).
void RenderMeshCameraSpace(const Mesh& mesh,
    int screen_left, int view_width, int half_height,
    int max_row_bottom);

// ---------------------------------------------------------------------
//  WorldSpace Ч объекты в мире.
//
//  mesh     Ч меш с posX/posY/posZ/angle/scale
//  xf       Ч предпосчитанный трансформ
//  cam*     Ч позици€ и угол наблюдател€
//  zbuffer  Ч массив глубины по X (перекрытие стенами)
//
//  Ѕыстра€ верси€ Ч принимает готовый xform.
// ---------------------------------------------------------------------
void RenderMeshWorldSpace(const Mesh& mesh, const MeshXform& xf,
    double camX, double camY, double camZ,
    double camAngle,
    int screen_left, int view_width, int half_height,
    const double* zbuffer);

// Back-compat overload: строит xform внутри.
void RenderMeshWorldSpace(const Mesh& mesh,
    double camX, double camY, double camZ,
    double camAngle,
    int screen_left, int view_width, int half_height,
    const double* zbuffer);