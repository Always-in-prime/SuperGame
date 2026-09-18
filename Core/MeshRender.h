#pragma once
#include "Mesh.h"
#include "Raycast3D.h"
#include "TextureBank.h"
#include "../Render/Framebuffer.h"
#include <cstdint>

// ============================================================
//  Единый рендер меша в framebuffer.
//
//  Два режима:
//    - CameraSpace — оружие и объекты в системе камеры.
//    - WorldSpace  — объекты в мире (персонажи, пикапы, декор).
//
//  Ключевой инвариант:
//    MeshXform строится ОДИН раз на кадр на объект и передаётся
//    в RaycastMesh. Это убирает 4 тригонометрии с каждого пикселя.
//
//  Публичный API принимает готовый MeshXform (быстрая версия)
//  либо Mesh (back-compat, строит xform внутри).
// ============================================================

// Функция освещения. Возвращает множитель ~0.4..1.25.
using LightFn = double (*)(double nx, double ny, double nz);

double LightDefault(double nx, double ny, double nz) noexcept;
double LightCharacter(double nx, double ny, double nz) noexcept;
double LightFlat(double nx, double ny, double nz) noexcept;

// ---------------------------------------------------------------------
//  CameraSpace — оружие и другие объекты в системе камеры.
//
//  Координаты меша: origin в глазах наблюдателя, +Y вверх, +Z вперёд.
//  Луч пускается из (0,0,0) в направлении (u, v, 1).
//
//  viewport:
//    screen_left    — X левого края вьюпорта в окне
//    view_width     — ширина вьюпорта в пикселях
//    half_height    — половина высоты окна (центр по Y)
//    max_row_bottom — нижняя граница отрисовки (для оружия)
//
//  Быстрая версия — принимает готовый xform.
// ---------------------------------------------------------------------
void RenderMeshCameraSpace(const Mesh& mesh, const MeshXform& xf,
    int screen_left, int view_width, int half_height,
    int max_row_bottom);

// Back-compat overload: строит xform внутри (2 тригонометрии за вызов).
void RenderMeshCameraSpace(const Mesh& mesh,
    int screen_left, int view_width, int half_height,
    int max_row_bottom);

// ---------------------------------------------------------------------
//  WorldSpace — объекты в мире.
//
//  mesh     — меш с posX/posY/posZ/angle/scale
//  xf       — предпосчитанный трансформ
//  cam*     — позиция и угол наблюдателя
//  zbuffer  — массив глубины по X (перекрытие стенами)
//
//  Быстрая версия — принимает готовый xform.
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