#pragma once
#include "../Core/Common.h"
#include <cstdint>

void FB_Init();
void FB_Present(HDC hdc);

inline int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Быстрое деление на 255 для v ∈ [0, 65535].
// Точное: (v * 0x8081) >> 23 == v / 255 для всех таких v.
inline int Div255(int v) noexcept {
    return (v * 0x8081) >> 23;
}

// Указатель на начало строки y. Без bounds-check.
// Гарантируется, что 0 <= y < WINDOW_HEIGHT — вызывающий обязан
// проверить сам. Для горячего пути (заливка строк).
uint32_t* FB_RowPtr(int y) noexcept;

// Заливка строки y от x0 (включительно) до x1 (исключительно).
// Проверяет только один раз y и границы x.
void FillRow(int y, int x0, int x1, uint32_t color) noexcept;

void SetPixelFast(int x, int y, uint32_t color);
void DrawVerticalLine(int x, int y0, int y1, uint32_t color);
void DrawHorizontalLine(int x0, int x1, int y, uint32_t color);
void DrawTexturedColumn(int x, int yTop, int yBottom,
    const uint32_t* tex, int texX,
    double texYStart, double texYStep, int shade);
void BlendPixelFast(int x, int y, uint32_t color, int alpha);
void BlendVignette(int x0, int x1, uint32_t color, int maxAlpha, int thickness);
void FillRect(int x, int y, int w, int h, uint32_t color);
void FillCircle(int cx, int cy, int r, uint32_t color);
void FillConvexPoly(const POINT* pts, int n, uint32_t color);
void DrawLine(int x0, int y0, int x1, int y1, uint32_t color);