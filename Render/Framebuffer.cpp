#include "Framebuffer.h"
#include <cmath>
#include <cstring>

// =====================================================================
//  Framebuffer + базовые примитивы
//
//  Формат хранения: 32-битный DIB (0x00RRGGBB), как ждёт
//  SetDIBitsToDevice с BI_RGB. RGB() WinAPI даёт 0x00BBGGRR —
//  конвертируем один раз при записи через ToDIB.
// =====================================================================
static uint32_t* framebuffer = nullptr;
static BITMAPINFO bmi = { 0 };

// RGB() даёт 0x00BBGGRR, а 32-битный DIB с BI_RGB ждёт 0x00RRGGBB
static inline uint32_t ToDIB(uint32_t rgb) noexcept {
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

// ---------------------------------------------------------------------
//  Прямой доступ к строке и быстрая заливка
// ---------------------------------------------------------------------
uint32_t* FB_RowPtr(int y) noexcept {
    return framebuffer + static_cast<size_t>(y) * WINDOW_WIDTH;
}

void FillRow(int y, int x0, int x1, uint32_t color) noexcept {
    if (y < 0 || y >= WINDOW_HEIGHT) return;
    if (x0 < 0) x0 = 0;
    if (x1 > WINDOW_WIDTH) x1 = WINDOW_WIDTH;
    if (x0 >= x1) return;

    const uint32_t dib = ToDIB(color);
    uint32_t* ptr = framebuffer + static_cast<size_t>(y) * WINDOW_WIDTH + x0;
    const int n = x1 - x0;

    // Компилятор векторизует этот цикл в SSE/AVX.
    for (int i = 0; i < n; ++i) ptr[i] = dib;
}

// ---------------------------------------------------------------------
//  Одиночный пиксель
// ---------------------------------------------------------------------
void SetPixelFast(int x, int y, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT) return;
    framebuffer[static_cast<size_t>(y) * WINDOW_WIDTH + x] = ToDIB(color);
}

void DrawVerticalLine(int x, int y0, int y1, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= WINDOW_HEIGHT) y1 = WINDOW_HEIGHT - 1;
    if (y0 > y1) return;

    const uint32_t dib = ToDIB(color);
    uint32_t* ptr = framebuffer + static_cast<size_t>(y0) * WINDOW_WIDTH + x;
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

    const uint32_t dib = ToDIB(color);
    uint32_t* ptr = framebuffer + static_cast<size_t>(y) * WINDOW_WIDTH + x0;
    for (int x = x0; x <= x1; ++x) *ptr++ = dib;
}

void FillRect(int x, int y, int w, int h, uint32_t color) {
    if (w <= 0 || h <= 0) return;
    const int x1 = x + w;   // полуоткрытый интервал — совместимо с FillRow
    for (int j = 0; j < h; ++j) {
        const int yy = y + j;
        if (yy < 0 || yy >= WINDOW_HEIGHT) continue;
        FillRow(yy, x, x1, color);
    }
}

void FillCircle(int cx, int cy, int r, uint32_t color) {
    if (r <= 0) return;
    for (int dy = -r; dy <= r; ++dy) {
        const int dx = static_cast<int>(std::sqrt(static_cast<double>(r * r - dy * dy)));
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
            const int j = (i + 1) % n;
            const int y1 = pts[i].y, y2 = pts[j].y;
            if (y1 == y2) continue;
            const bool crosses = (y >= y1 && y < y2) || (y >= y2 && y < y1);
            if (!crosses) continue;
            const double t = static_cast<double>(y - y1) / static_cast<double>(y2 - y1);
            const int x = static_cast<int>(pts[i].x + t * (pts[j].x - pts[i].x));
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
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// ---------------------------------------------------------------------
//  Текстурированный столбец.
//
//  Горячий путь: ~640 вызовов на кадр × ~480 строк.
//
//  Оптимизации:
//    - tinted[] строится сразу в DIB-формате: никаких RGB()/GetRValue
//      в горячем цикле.
//    - Если shade == 256 — текстура используется как есть, без tinting.
//    - texStep и texStart предпосчитаны вызывающим.
// ---------------------------------------------------------------------
void DrawTexturedColumn(int x, int yTop, int yBottom,
    const uint32_t* tex, int texX,
    double texYStart, double texYStep,
    int shade)
{
    if (x < 0 || x >= WINDOW_WIDTH) return;

    if (yTop < 0) {
        texYStart += static_cast<double>(-yTop) * texYStep;
        yTop = 0;
    }
    if (yBottom >= WINDOW_HEIGHT) yBottom = WINDOW_HEIGHT - 1;
    if (yTop > yBottom) return;

    // Текстура лежит в DIB-формате. tinted — тоже DIB.
    // Извлекаем каналы и обратно упаковываем без вызовов RGB().
    uint32_t tinted[64];

    if (shade >= 256) {
        // Без затемнения — берём текстуру как есть.
        // Убирает 64 итерации tinting на каждый вызов.
        for (int i = 0; i < 64; ++i) {
            tinted[i] = tex[i * 64 + texX];
        }
    }
    else {
        const uint32_t sh = static_cast<uint32_t>(shade);
        for (int i = 0; i < 64; ++i) {
            const uint32_t c = tex[i * 64 + texX];
            // DIB: 0x00RRGGBB
            const uint32_t r = ((c >> 16) & 0xFFu) * sh >> 8;
            const uint32_t g = ((c >> 8) & 0xFFu) * sh >> 8;
            const uint32_t b = (c & 0xFFu) * sh >> 8;
            tinted[i] = (r << 16) | (g << 8) | b;
        }
    }

    uint32_t* ptr = framebuffer + static_cast<size_t>(yTop) * WINDOW_WIDTH + x;
    double ty = texYStart;
    for (int y = yTop; y <= yBottom; ++y) {
        const int texY = static_cast<int>(ty) & 63;
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

    uint32_t* p = &framebuffer[static_cast<size_t>(y) * WINDOW_WIDTH + x];
    const uint32_t dst = *p;
    const uint32_t src = ToDIB(color);

    const int sr = (src >> 16) & 0xFF, sg = (src >> 8) & 0xFF, sb = src & 0xFF;
    const int dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;

    const int ia = 255 - alpha;
    // Div255 — точное деление на 255 без деления.
    const int r = Div255(sr * alpha + dr * ia);
    const int g = Div255(sg * alpha + dg * ia);
    const int b = Div255(sb * alpha + db * ia);

    *p = (static_cast<uint32_t>(r) << 16) |
        (static_cast<uint32_t>(g) << 8) |
        static_cast<uint32_t>(b);
}

// ---------------------------------------------------------------------
//  Виньетка.
//
//  a(x,y) = maxAlpha * (thickness - min(dxEdge, dyEdge)) / thickness
//
//  dxEdge зависит только от x, dyEdge — только от y.
//  Предпосчитываем alphaX[x] и alphaY[y], берём min.
//  Деление — один раз на столбец/строку, не на пиксель.
// ---------------------------------------------------------------------
void BlendVignette(int x0, int x1, uint32_t color, int maxAlpha, int thickness) {
    if (x1 <= x0 || thickness <= 0 || maxAlpha <= 0) return;
    if (x0 < 0) x0 = 0;
    if (x1 > WINDOW_WIDTH) x1 = WINDOW_WIDTH;

    const int w = x1 - x0;
    if (w <= 0) return;

    // alphaX[x - x0] для x ∈ [x0, x1)
    // alphaY[y] для y ∈ [0, WINDOW_HEIGHT)
    static thread_local int alphaX[WINDOW_WIDTH];
    static thread_local int alphaY[WINDOW_HEIGHT];

    for (int i = 0; i < w; ++i) {
        const int xL = i;
        const int xR = (w - 1) - i;
        const int dx = (xL < xR) ? xL : xR;
        alphaX[i] = (dx >= thickness) ? 0
            : maxAlpha * (thickness - dx) / thickness;
    }
    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        const int dyL = y;
        const int dyR = (WINDOW_HEIGHT - 1) - y;
        const int dy = (dyL < dyR) ? dyL : dyR;
        alphaY[y] = (dy >= thickness) ? 0
            : maxAlpha * (thickness - dy) / thickness;
    }

    const uint32_t src = ToDIB(color);
    const int sr = (src >> 16) & 0xFF, sg = (src >> 8) & 0xFF, sb = src & 0xFF;

    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        const int ay = alphaY[y];
        if (ay == 0) continue;

        uint32_t* ptr = framebuffer + static_cast<size_t>(y) * WINDOW_WIDTH + x0;

        for (int i = 0; i < w; ++i, ++ptr) {
            const int ax = alphaX[i];
            const int a = (ax < ay) ? ax : ay;
            if (a == 0) continue;

            const uint32_t dst = *ptr;
            const int dr = (dst >> 16) & 0xFF, dg = (dst >> 8) & 0xFF, db = dst & 0xFF;

            const int ia = 255 - a;
            const int r = Div255(sr * a + dr * ia);
            const int g = Div255(sg * a + dg * ia);
            const int b = Div255(sb * a + db * ia);

            *ptr = (static_cast<uint32_t>(r) << 16) |
                (static_cast<uint32_t>(g) << 8) |
                static_cast<uint32_t>(b);
        }
    }
}