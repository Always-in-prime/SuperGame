#include "Framebuffer.h"
#include <cmath>
#include <cstring>

// =====================================================================
//  Framebuffer + базовые примитивы
// =====================================================================
static uint32_t* framebuffer = nullptr;
static BITMAPINFO bmi = { 0 };

// RGB() даёт 0x00BBGGRR, а 32-битный DIB с BI_RGB ждёт 0x00RRGGBB
static inline uint32_t ToDIB(uint32_t rgb) {
    return ((rgb & 0x000000FFu) << 16) |
        (rgb & 0x0000FF00u) |
        ((rgb & 0x00FF0000u) >> 16);
}

void FB_Init() {
    if (framebuffer) return;
    framebuffer = new uint32_t[WINDOW_WIDTH * WINDOW_HEIGHT];

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WINDOW_WIDTH;
    bmi.bmiHeader.biHeight = -WINDOW_HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
}

void FB_Present(HDC hdc) {
    static HDC     memDC = nullptr;
    static HBITMAP memBmp = nullptr;
    static HBITMAP oldBmp = nullptr;

    if (!memDC) {
        memDC = CreateCompatibleDC(hdc);
        memBmp = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
        oldBmp = (HBITMAP)SelectObject(memDC, memBmp);
    }

    SetDIBitsToDevice(memDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        0, 0, 0, WINDOW_HEIGHT, framebuffer, &bmi, DIB_RGB_COLORS);
    BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);
}

void SetPixelFast(int x, int y, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT) return;
    framebuffer[y * WINDOW_WIDTH + x] = ToDIB(color);
}

void DrawVerticalLine(int x, int y0, int y1, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= WINDOW_HEIGHT) y1 = WINDOW_HEIGHT - 1;
    if (y0 > y1) return;

    uint32_t dib = ToDIB(color);
    uint32_t* ptr = framebuffer + y0 * WINDOW_WIDTH + x;
    for (int y = y0; y <= y1; ++y) {
        *ptr = dib;
        ptr += WINDOW_WIDTH;
    }
}

void DrawHorizontalLine(int x0, int x1, int y, uint32_t color) {
    if (y < 0 || y >= WINDOW_HEIGHT) return;
    if (x0 > x1) { int t = x0; x0 = x1; x1 = t; }
    if (x0 < 0) x0 = 0;
    if (x1 >= WINDOW_WIDTH) x1 = WINDOW_WIDTH - 1;
    if (x0 > x1) return;

    uint32_t dib = ToDIB(color);
    uint32_t* ptr = framebuffer + y * WINDOW_WIDTH + x0;
    for (int x = x0; x <= x1; ++x) *ptr++ = dib;
}

void FillRect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    for (int j = 0; j < h; ++j) {
        int yy = y + j;
        if (yy < 0 || yy >= WINDOW_HEIGHT) continue;
        DrawHorizontalLine(x, x + w - 1, yy, color);
    }
}

void FillCircle(int cx, int cy, int r, uint32_t color) {
    if (r <= 0) return;
    for (int dy = -r; dy <= r; ++dy) {
        int dx = (int)sqrt((double)(r * r - dy * dy));
        DrawHorizontalLine(cx - dx, cx + dx, cy + dy, color);
    }
}

void FillConvexPoly(const POINT* pts, int n, uint32_t color) {
    if (n < 3) return;
    int ymin = pts[0].y, ymax = pts[0].y;
    for (int i = 1; i < n; ++i) {
        if (pts[i].y < ymin) ymin = pts[i].y;
        if (pts[i].y > ymax) ymax = pts[i].y;
    }
    if (ymin < 0) ymin = 0;
    if (ymax >= WINDOW_HEIGHT) ymax = WINDOW_HEIGHT - 1;
    if (ymin > ymax) return;

    for (int y = ymin; y <= ymax; ++y) {
        int hits[8], nhits = 0;
        for (int i = 0; i < n; ++i) {
            int j = (i + 1) % n;
            int y1 = pts[i].y, y2 = pts[j].y;
            if (y1 == y2) continue;
            bool crosses = (y >= y1 && y < y2) || (y >= y2 && y < y1);
            if (!crosses) continue;
            double t = (double)(y - y1) / (double)(y2 - y1);
            int x = (int)(pts[i].x + t * (pts[j].x - pts[i].x));
            if (nhits < 8) hits[nhits++] = x;
        }
        if (nhits >= 2) {
            int mn = hits[0], mx = hits[0];
            for (int i = 1; i < nhits; ++i) {
                if (hits[i] < mn) mn = hits[i];
                if (hits[i] > mx) mx = hits[i];
            }
            DrawHorizontalLine(mn, mx, y, color);
        }
    }
}

void DrawLine(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        SetPixelFast(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void DrawTexturedColumn(int x, int yTop, int yBottom,
    const uint32_t* tex, int texX,
    double texYStart, double texYStep,
    int shade)
{
    if (x < 0 || x >= WINDOW_WIDTH) return;

    if (yTop < 0) {
        texYStart += (-yTop) * texYStep;
        yTop = 0;
    }
    if (yBottom >= WINDOW_HEIGHT) yBottom = WINDOW_HEIGHT - 1;
    if (yTop > yBottom) return;

    uint32_t tinted[64];
    for (int i = 0; i < 64; ++i) {
        uint32_t c = tex[i * 64 + texX];
        int r = (GetRValue(c) * shade) >> 8;
        int g = (GetGValue(c) * shade) >> 8;
        int b = (GetBValue(c) * shade) >> 8;
        tinted[i] = ToDIB(RGB(r, g, b));
    }

    uint32_t* ptr = framebuffer + yTop * WINDOW_WIDTH + x;
    double ty = texYStart;
    for (int y = yTop; y <= yBottom; ++y) {
        int texY = ((int)ty) & 63;
        *ptr = tinted[texY];
        ptr += WINDOW_WIDTH;
        ty += texYStep;
    }
}

// =====================================================================
//  Alpha blending
// =====================================================================
void BlendPixelFast(int x, int y, uint32_t color, int alpha) {
    if (alpha <= 0) return;
    if (alpha > 255) alpha = 255;
    if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT) return;

    uint32_t* p = &framebuffer[y * WINDOW_WIDTH + x];
    uint32_t dst = *p;
    uint32_t src = ToDIB(color);

    int sr = (src >> 16) & 0xFF, sg = (src >> 8) & 0xFF, sb = src & 0xFF;
    int dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;

    int r = (sr * alpha + dr * (255 - alpha)) / 255;
    int g = (sg * alpha + dg * (255 - alpha)) / 255;
    int b = (sb * alpha + db * (255 - alpha)) / 255;

    *p = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

void BlendVignette(int x0, int x1, uint32_t color, int maxAlpha, int thickness) {
    if (x1 <= x0 || thickness <= 0 || maxAlpha <= 0) return;
    if (x0 < 0) x0 = 0;
    if (x1 > WINDOW_WIDTH) x1 = WINDOW_WIDTH;

    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        int dyEdge = (y < WINDOW_HEIGHT - 1 - y) ? y : (WINDOW_HEIGHT - 1 - y);
        for (int x = x0; x < x1; ++x) {
            int dxL = x - x0;
            int dxR = (x1 - 1) - x;
            int dxEdge = (dxL < dxR) ? dxL : dxR;
            int d = (dxEdge < dyEdge) ? dxEdge : dyEdge;
            if (d >= thickness) continue;
            int a = maxAlpha * (thickness - d) / thickness;
            BlendPixelFast(x, y, color, a);
        }
    }
}