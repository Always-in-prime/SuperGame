#include "Renderer.h"
#include "Map.h"
#include "Player.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#pragma comment(lib, "msimg32.lib")

// =====================================================================
//  Быстрый буфер пикселей
// =====================================================================
static uint32_t* framebuffer = nullptr;
static BITMAPINFO bmi = { 0 };

// RGB() в Windows даёт 0x00BBGGRR, а 32-битный DIB с BI_RGB ждёт 0x00RRGGBB
static inline uint32_t ToDIB(uint32_t rgb) {
    return ((rgb & 0x000000FFu) << 16) |
        (rgb & 0x0000FF00u) |
        ((rgb & 0x00FF0000u) >> 16);
}

static void InitFastRender() {
    if (framebuffer) return;
    framebuffer = new uint32_t[WINDOW_WIDTH * WINDOW_HEIGHT];

    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WINDOW_WIDTH;
    bmi.bmiHeader.biHeight = -WINDOW_HEIGHT;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
}

static inline void SetPixelFast(int x, int y, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH || y < 0 || y >= WINDOW_HEIGHT) return;
    framebuffer[y * WINDOW_WIDTH + x] = ToDIB(color);
}

static inline void DrawVerticalLine(int x, int y0, int y1, uint32_t color) {
    if (x < 0 || x >= WINDOW_WIDTH) return;
    if (y0 < 0) y0 = 0;
    if (y1 >= WINDOW_HEIGHT) y1 = WINDOW_HEIGHT - 1;

    uint32_t dib = ToDIB(color);
    uint32_t* ptr = framebuffer + y0 * WINDOW_WIDTH + x;
    for (int y = y0; y <= y1; ++y) {
        *ptr = dib;
        ptr += WINDOW_WIDTH;
    }
}

// =====================================================================
//  Внутренние утилиты
// =====================================================================
static inline int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ---------------------------------------------------------------------
//  DDA-рейкаст: возвращает перпендикулярное расстояние и сторону стены.
// ---------------------------------------------------------------------
struct RayHit { double dist; int side; };

static RayHit CastRay(double px, double py, double dirX, double dirY) {
    int mapX = (int)floor(px);
    int mapY = (int)floor(py);

    double ddx = (dirX == 0.0) ? 1e30 : fabs(1.0 / dirX);
    double ddy = (dirY == 0.0) ? 1e30 : fabs(1.0 / dirY);

    int stepX, stepY;
    double sdx, sdy;

    if (dirX < 0) { stepX = -1; sdx = (px - mapX) * ddx; }
    else { stepX = 1; sdx = (mapX + 1.0 - px) * ddx; }
    if (dirY < 0) { stepY = -1; sdy = (py - mapY) * ddy; }
    else { stepY = 1; sdy = (mapY + 1.0 - py) * ddy; }

    int side = 0;
    for (int i = 0; i < 128; ++i) {
        if (sdx < sdy) { sdx += ddx; mapX += stepX; side = 0; }
        else { sdy += ddy; mapY += stepY; side = 1; }

        if (mapX < 0 || mapX >= MAP_WIDTH || mapY < 0 || mapY >= MAP_HEIGHT) break;
        if (map[mapY][mapX] == '#') break;
    }

    double dist = (side == 0) ? (sdx - ddx) : (sdy - ddy);
    if (dist < 0.0001) dist = 0.0001;

    RayHit h; h.dist = dist; h.side = side;
    return h;
}

// =====================================================================
//  Спрайт второго игрока (попиксельная отрисовка в буфер)
// =====================================================================
static void RenderOtherPlayer(Player& viewer, Player& target,
    int screen_left, int view_width, int half_height, double* zbuffer)
{
    if (target.deathTimer > 0) return;

    double dx = target.x - viewer.x;
    double dy = target.y - viewer.y;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 0.3) return;

    double angle_to = atan2(dx, dy);
    double angle_diff = angle_to - viewer.angle;
    while (angle_diff > PI) angle_diff -= 2 * PI;
    while (angle_diff < -PI) angle_diff += 2 * PI;

    const double halfFov = FOV * 0.5;
    if (fabs(angle_diff) > halfFov + 0.5) return;

    double perp_dist = dist * cos(angle_diff);
    if (perp_dist < 0.1) perp_dist = 0.1;

    double tanHalfFov = tan(halfFov);
    double screen_center = (0.5 + 0.5 * tan(angle_diff) / tanHalfFov) * view_width;

    double sprite_h = (double)WINDOW_HEIGHT / perp_dist;
    if (sprite_h > WINDOW_HEIGHT * 6.0) sprite_h = WINDOW_HEIGHT * 6.0;
    double sprite_w = sprite_h * 0.50;
    double spriteTopF = half_height - sprite_h * 0.5;

    double dir_to_viewer = atan2(viewer.x - target.x, viewer.y - target.y);
    double facing = target.angle - dir_to_viewer;
    while (facing > PI) facing -= 2 * PI;
    while (facing < -PI) facing += 2 * PI;

    double cosF = cos(facing);
    double sinF = sin(facing);
    double mix = (1.0 - cosF) * 0.5;

    double shade = (perp_dist > 4.0) ? 4.0 / perp_dist : 1.0;
    if (shade < 0.30) shade = 0.30;

    auto shade3 = [&](int r, int g, int b) -> uint32_t {
        return RGB((int)(r * shade), (int)(g * shade), (int)(b * shade));
    };
    auto mix3 = [](uint32_t a, uint32_t b, double t) -> uint32_t {
        return RGB(
            (int)(GetRValue(a) * (1 - t) + GetRValue(b) * t),
            (int)(GetGValue(a) * (1 - t) + GetGValue(b) * t),
            (int)(GetBValue(a) * (1 - t) + GetBValue(b) * t));
    };

    uint32_t headColor = mix3(shade3(255, 215, 175), shade3(60, 40, 25), mix);
    uint32_t torsoColor = mix3(shade3(70, 110, 220), shade3(30, 50, 120), mix);
    uint32_t armColor = mix3(shade3(55, 90, 195), shade3(22, 38, 95), mix);
    uint32_t legsColor = shade3(45, 45, 60);
    uint32_t shoeColor = shade3(25, 20, 15);
    uint32_t eyeColor = shade3(15, 15, 15);

    const double HEAD_FRAC = 0.22;
    const double TORSO_FRAC = 0.60;
    const double LEGS_FRAC = 0.94;

    double headW = sprite_w * (0.42 + 0.30 * fabs(cosF));
    double torsoW = sprite_w * (0.62 + 0.38 * fabs(cosF));
    double legsW = torsoW * 0.80;

    double headHW = headW * 0.5;
    double torsoHW = torsoW * 0.5;
    double legsHW = legsW * 0.5;

    int yHeadBot = (int)(spriteTopF + sprite_h * HEAD_FRAC);
    int yTorsoBot = (int)(spriteTopF + sprite_h * TORSO_FRAC);
    int yLegsBot = (int)(spriteTopF + sprite_h * LEGS_FRAC);
    int yFeetBot = (int)(spriteTopF + sprite_h);

    int x_start = (int)floor(screen_center - sprite_w * 0.5) - 1;
    int x_end = (int)ceil(screen_center + sprite_w * 0.5) + 1;

    for (int x = x_start; x <= x_end; ++x) {
        if (x < 0 || x >= view_width) continue;
        if (perp_dist > zbuffer[x]) continue;

        double u = (x + 0.5) - screen_center;
        double au = fabs(u);

        if (au < headHW) {
            double t = au / headHW;
            double bulge = sqrt(1.0 - t * t);
            int hTop = (int)spriteTopF;
            int hBot = yHeadBot;
            int shrink = (int)((1.0 - bulge) * (hBot - hTop) * 0.5);
            DrawVerticalLine(screen_left + x, hTop + shrink, hBot - shrink, headColor);
        }
        if (au < torsoHW) {
            uint32_t col = (au > torsoHW * 0.72) ? armColor : torsoColor;
            DrawVerticalLine(screen_left + x, yHeadBot, yTorsoBot, col);
        }
        if (au < legsHW && au > legsHW * 0.12) {
            DrawVerticalLine(screen_left + x, yTorsoBot, yLegsBot, legsColor);
            DrawVerticalLine(screen_left + x, yLegsBot, yFeetBot, shoeColor);
        }
    }

    if (cosF > 0.0) {
        double eyeY = spriteTopF + sprite_h * HEAD_FRAC * 0.55;
        double eyeH = sprite_h * HEAD_FRAC * 0.14;
        double eyeW = headW * 0.10;
        double eyeSep = headW * 0.20 * cosF;
        double eyeShift = sinF * headW * 0.30;

        double eyesX[2] = {
            screen_center + eyeShift - eyeSep,
            screen_center + eyeShift + eyeSep
        };
        int ey1 = (int)(eyeY - eyeH * 0.5);
        int ey2 = (int)(eyeY + eyeH * 0.5);

        for (int e = 0; e < 2; ++e) {
            if (fabs(eyesX[e] - screen_center) > headHW * 0.90) continue;
            int ex1 = (int)(eyesX[e] - eyeW * 0.5);
            int ex2 = (int)(eyesX[e] + eyeW * 0.5);
            for (int px = ex1; px <= ex2; ++px) {
                if (px < 0 || px >= view_width) continue;
                if (perp_dist > zbuffer[px]) continue;
                DrawVerticalLine(screen_left + px, ey1, ey2, eyeColor);
            }
        }
    }
}

// =====================================================================
//  Прицел в центре вида
// =====================================================================
static void RenderCrosshair(int cx, int cy) {
    const int gap = 4;
    const int len = 10;
    uint32_t outline = RGB(0, 0, 0);
    uint32_t inner = RGB(255, 220, 70);

    for (int o = -1; o <= 1; ++o) {
        DrawVerticalLine(cx - gap - len, cy + o, cy + o, outline);
        DrawVerticalLine(cx - gap, cy + o, cy + o, outline);
        DrawVerticalLine(cx + gap, cy + o, cy + o, outline);
        DrawVerticalLine(cx + gap + len, cy + o, cy + o, outline);
        DrawVerticalLine(cx + o, cy - gap - len, cy - gap, outline);
        DrawVerticalLine(cx + o, cy + gap, cy + gap + len, outline);
    }
    DrawVerticalLine(cx - gap - len + 1, cy, cy, inner);
    DrawVerticalLine(cx + gap + 1, cy, cy, inner);
    DrawVerticalLine(cx, cy - gap - len + 1, cy - gap - 1, inner);
    DrawVerticalLine(cx, cy + gap + 1, cy + gap + len - 1, inner);
}

// =====================================================================
//  Оружие (рисуем через GDI в отдельный memDC)
// =====================================================================
static void RenderWeapon(HDC memDC, Player& p, int screen_left, int screen_right) {
    if (p.deathTimer > 0) return;
    (void)screen_left;

    const int bottom = WINDOW_HEIGHT;

    auto quad = [&](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
        COLORREF f, COLORREF l, int lw = 1) {
            POINT q[4] = { {x1,y1},{x2,y2},{x3,y3},{x4,y4} };
            HBRUSH b = CreateSolidBrush(f);
            HPEN   pn = CreatePen(PS_SOLID, lw, l);
            HBRUSH ob = (HBRUSH)SelectObject(memDC, b);
            HPEN   op = (HPEN)SelectObject(memDC, pn);
            Polygon(memDC, q, 4);
            SelectObject(memDC, ob);
            SelectObject(memDC, op);
            DeleteObject(b);
            DeleteObject(pn);
    };

    auto disc = [&](int l, int t, int r, int b, COLORREF f, COLORREF lc, int lw = 1) {
        HBRUSH br = CreateSolidBrush(f);
        HPEN   pn = CreatePen(PS_SOLID, lw, lc);
        HBRUSH ob = (HBRUSH)SelectObject(memDC, br);
        HPEN   op = (HPEN)SelectObject(memDC, pn);
        Ellipse(memDC, l, t, r, b);
        SelectObject(memDC, ob);
        SelectObject(memDC, op);
        DeleteObject(br);
        DeleteObject(pn);
    };

    const int kick = (p.muzzleFlash > 0) ? (int)(p.muzzleFlash * 8.0f) : 0;

    const int recvX = screen_right - 110;
    const int recvY = bottom - 160 + kick;
    const int recvBot = bottom - 40 + kick;
    const int muzX = recvX - 65;
    const int muzY = recvY - 100 + kick;

    const int recvHalfW = 22;
    const int muzHalfW = 12;

    auto tAt = [&](int y) {
        double t = double(recvY - y) / double(recvY - muzY);
        return t < 0 ? 0.0 : (t > 1 ? 1.0 : t);
    };
    auto axisX = [&](int y) { return recvX + (int)((muzX - recvX) * tAt(y)); };
    auto halfW = [&](int y) { return recvHalfW + (int)((muzHalfW - recvHalfW) * tAt(y)); };

    const COLORREF steelDrk = RGB(14, 14, 20);
    const COLORREF steelMid = RGB(48, 48, 60);
    const COLORREF steelHi = RGB(150, 154, 170);
    const COLORREF woodMid = RGB(108, 64, 30);
    const COLORREF woodDrk = RGB(52, 26, 8);
    const COLORREF skinMid = RGB(226, 178, 140);
    const COLORREF skinEdge = RGB(82, 48, 26);

    {
        int xN = axisX(recvY), wN = halfW(recvY);
        int xF = axisX(muzY), wF = halfW(muzY);
        quad(xN - wN, recvY, xN + wN, recvY,
            xF + wF, muzY, xF - wF, muzY,
            steelMid, steelDrk, 2);
        quad(xN - wN + wN * 25 / 100, recvY, xN - wN + wN * 75 / 100, recvY,
            xF - wF + wF * 75 / 100, muzY, xF - wF + wF * 25 / 100, muzY,
            steelHi, steelHi, 1);
    }
    {
        int mx = axisX(muzY), mw = halfW(muzY);
        disc(mx - mw + 2, muzY - 3, mx + mw - 2, muzY + 3,
            RGB(4, 4, 6), RGB(0, 0, 0), 1);
    }
    {
        int botY = recvY - 20;
        int topY = recvY - 80;
        int x1 = axisX(botY), w1 = halfW(botY) + 7;
        int x2 = axisX(topY), w2 = halfW(topY) + 7;
        quad(x1 - w1, botY, x1 + w1, botY,
            x2 + w2, topY, x2 - w2, topY,
            woodMid, woodDrk, 2);
        quad(x1 - w1 + 3, botY, x1 + w1 - 3, botY,
            x2 + w2 - 3, topY, x2 - w2 + 3, topY,
            RGB(168, 116, 64), RGB(168, 116, 64), 1);
    }
    {
        int cxT = axisX(recvY);
        POINT r[4] = {
            { cxT - 26,   recvY + 4 },
            { cxT + 26,   recvY + 4 },
            { recvX + 34, recvBot   },
            { recvX - 34, recvBot   }
        };
        HBRUSH b = CreateSolidBrush(steelMid);
        HPEN   pn = CreatePen(PS_SOLID, 2, steelDrk);
        HBRUSH ob = (HBRUSH)SelectObject(memDC, b);
        HPEN   op = (HPEN)SelectObject(memDC, pn);
        Polygon(memDC, r, 4);
        SelectObject(memDC, ob);
        SelectObject(memDC, op);
        DeleteObject(b);
        DeleteObject(pn);
    }
    {
        int cx = recvX;
        POINT s[4] = {
            { cx + 18,  recvBot - 4 },
            { cx + 62,  recvBot - 4 },
            { cx + 110, bottom + 40 },
            { cx + 30,  bottom + 40 }
        };
        HBRUSH b = CreateSolidBrush(woodMid);
        HPEN   pn = CreatePen(PS_SOLID, 2, woodDrk);
        HBRUSH ob = (HBRUSH)SelectObject(memDC, b);
        HPEN   op = (HPEN)SelectObject(memDC, pn);
        Polygon(memDC, s, 4);
        SelectObject(memDC, ob);
        SelectObject(memDC, op);
        DeleteObject(b);
        DeleteObject(pn);
    }
    {
        int cx = recvX;
        quad(cx + 4, recvBot - 30,
            cx + 32, recvBot - 28,
            cx + 40, recvBot + 40,
            cx + 4, recvBot + 38,
            skinMid, skinEdge, 2);
    }

    if (p.muzzleFlash > 0) {
        int tx = axisX(muzY), ty = muzY - 4;
        int g = 14 + rand() % 5;
        disc(tx - g, ty - g, tx + g, ty + g,
            RGB(255, 130, 24), RGB(255, 160, 60), 1);
        int c = 5 + rand() % 2;
        disc(tx - c, ty - c, tx + c, ty + c,
            RGB(255, 252, 214), RGB(255, 255, 246), 1);
    }
}

// =====================================================================
//  HUD (тоже через GDI)
// =====================================================================
static void RenderHUD(HDC memDC, Player& p, int screen_left, int screen_right) {
    (void)screen_right;

    const int barW = 180;
    const int barH = 18;
    const int barX = screen_left + 12;
    const int barY = WINDOW_HEIGHT - 32;

    HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 20));
    RECT bgR = { barX - 2, barY - 2, barX + barW + 2, barY + barH + 2 };
    FillRect(memDC, &bgR, bgBrush);
    DeleteObject(bgBrush);

    int hp = clampi(p.hp, 0, 100);
    int fillW = barW * hp / 100;

    COLORREF hpColor = RGB(60, 200, 60);
    if (hp <= 30)      hpColor = RGB(210, 40, 40);
    else if (hp <= 60) hpColor = RGB(220, 180, 40);

    HBRUSH hpBrush = CreateSolidBrush(hpColor);
    RECT hpR = { barX, barY, barX + fillW, barY + barH };
    FillRect(memDC, &hpR, hpBrush);
    DeleteObject(hpBrush);

    HPEN framePen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(memDC, framePen);
    HBRUSH oldBr = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
    Rectangle(memDC, barX, barY, barX + barW, barY + barH);
    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBr);
    DeleteObject(framePen);

    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(240, 240, 240));

    wchar_t buf[64];
    swprintf_s(buf, L"HP %d", p.hp);
    TextOutW(memDC, barX + barW + 10, barY, buf, (int)wcslen(buf));

    swprintf_s(buf, L"Счёт: %d", p.score);
    TextOutW(memDC, barX + barW + 10, barY - 22, buf, (int)wcslen(buf));
}

// =====================================================================
//  Вид одного игрока
// =====================================================================
void RenderPlayerView(Player& player, Player& other,
    int screen_left, int screen_right)
{
    const int view_width = screen_right - screen_left;
    const int half_height = WINDOW_HEIGHT / 2;

    if (player.deathTimer > 0) {
        uint32_t deadColor = RGB(70, 8, 8);
        for (int y = 0; y < WINDOW_HEIGHT; ++y) {
            for (int x = screen_left; x < screen_right; ++x) {
                SetPixelFast(x, y, deadColor);
            }
        }
        return;
    }

    double* zbuffer = new double[view_width];

    // ----- Небо -----
    for (int y = 0; y < half_height; ++y) {
        double t = (double)y / half_height;
        int r = (int)(60 * (1 - t) + 22 * t);
        int g = (int)(60 * (1 - t) + 22 * t);
        int b = (int)(78 * (1 - t) + 32 * t);
        uint32_t skyColor = RGB(r, g, b);
        for (int x = screen_left; x < screen_right; ++x) {
            SetPixelFast(x, y, skyColor);
        }
    }

    // ----- Пол -----
    for (int y = half_height; y < WINDOW_HEIGHT; ++y) {
        double t = (double)(y - half_height) / (WINDOW_HEIGHT - half_height);
        int r = (int)(35 * (1 - t) + 95 * t);
        int g = (int)(32 * (1 - t) + 80 * t);
        int b = (int)(30 * (1 - t) + 60 * t);
        uint32_t floorColor = RGB(r, g, b);
        for (int x = screen_left; x < screen_right; ++x) {
            SetPixelFast(x, y, floorColor);
        }
    }

    // ----- Стены (рейкаст) -----
    for (int x = 0; x < view_width; ++x) {
        double ray_angle = (player.angle - FOV / 2.0)
            + ((double)x / (double)view_width) * FOV;
        double dirX = sin(ray_angle);
        double dirY = cos(ray_angle);

        RayHit hit = CastRay(player.x, player.y, dirX, dirY);
        double perp = hit.dist * cos(ray_angle - player.angle);
        if (perp < 0.05) perp = 0.05;
        zbuffer[x] = perp;

        int wall_height = (int)((double)WINDOW_HEIGHT / perp);
        if (wall_height > WINDOW_HEIGHT * 4) wall_height = WINDOW_HEIGHT * 4;

        int ceiling = half_height - wall_height / 2;
        int floor_ = half_height + wall_height / 2;

        double fog = 1.0 / (1.0 + perp * perp * 0.09);
        if (hit.side == 1) fog *= 0.72;
        int shade = clampi((int)(fog * 255.0), 0, 255);
        int t = shade;
        uint32_t wallColor = RGB(t, t * 100 / 255, t * 55 / 255);
        DrawVerticalLine(screen_left + x, ceiling, floor_, wallColor);
    }

    RenderOtherPlayer(player, other, screen_left, view_width, half_height, zbuffer);
    RenderCrosshair(screen_left + view_width / 2, half_height);

    delete[] zbuffer;
}

// =====================================================================
//  Главная отрисовка
// =====================================================================
void DrawGame(HDC hdc) {
    InitFastRender();

    // ---- 1. 3D-виды в framebuffer ----
    RenderPlayerView(p1, p2, 0, WINDOW_WIDTH / 2);
    RenderPlayerView(p2, p1, WINDOW_WIDTH / 2, WINDOW_WIDTH);

    // Разделительная линия между видами
    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        SetPixelFast(WINDOW_WIDTH / 2, y, RGB(0, 0, 0));
        SetPixelFast(WINDOW_WIDTH / 2 + 1, y, RGB(0, 0, 0));
    }

    // ---- 2. Постоянный back-буфер (создаётся один раз) ----
    static HDC     backDC = nullptr;
    static HBITMAP backBmp = nullptr;
    static HBITMAP oldBmp = nullptr;

    if (!backDC) {
        backDC = CreateCompatibleDC(hdc);
        backBmp = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
        oldBmp = (HBITMAP)SelectObject(backDC, backBmp);
    }

    // ---- 3. framebuffer → back-буфер ----
    SetDIBitsToDevice(backDC, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
        0, 0, 0, WINDOW_HEIGHT, framebuffer, &bmi, DIB_RGB_COLORS);

    // ---- 4. Оружие и HUD рисуем В ТОТ ЖЕ back-буфер ----
    RenderWeapon(backDC, p1, 0, WINDOW_WIDTH / 2);
    RenderWeapon(backDC, p2, WINDOW_WIDTH / 2, WINDOW_WIDTH);
    RenderHUD(backDC, p1, 0, WINDOW_WIDTH / 2);
    RenderHUD(backDC, p2, WINDOW_WIDTH / 2, WINDOW_WIDTH);

    SetBkMode(backDC, TRANSPARENT);
    SetTextColor(backDC, RGB(230, 230, 230));
    TextOutW(backDC, 10, 10, L"Игрок 1: WASD + ПРОБЕЛ", 22);
    TextOutW(backDC, WINDOW_WIDTH / 2 + 10, 10,
        L"Игрок 2: СТРЕЛКИ + R.CTRL", 25);

    // ---- 5. Один атомарный блит на экран ----
    BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, backDC, 0, 0, SRCCOPY);
}