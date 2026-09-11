#include "Renderer.h"
#include "Map.h"
#include "Player.h"
#include <math.h>
#include <stdlib.h>

// =====================================================================
//  Внутренние утилиты
// =====================================================================
static inline int clampi(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}
static inline double clampd(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ---------------------------------------------------------------------
//  Кэш перьев для стен (64 оттенка). Создаём один раз на процесс.
// ---------------------------------------------------------------------
static HPEN g_wallPens[64] = { 0 };
static void InitWallPens() {
    if (g_wallPens[0]) return;
    for (int i = 0; i < 64; ++i) {
        int t = i * 255 / 63;
        g_wallPens[i] = CreatePen(PS_SOLID, 1, RGB(t, t * 100 / 255, t * 55 / 255));
    }
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
//  Спрайт второго игрока (билборд с учётом поворота тела)
// =====================================================================
static void RenderOtherPlayer(HDC memDC, Player& viewer, Player& target,
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

    // -------- ориентация тела относительно зрителя --------
    double dir_to_viewer = atan2(viewer.x - target.x, viewer.y - target.y);
    double facing = target.angle - dir_to_viewer;
    while (facing > PI) facing -= 2 * PI;
    while (facing < -PI) facing += 2 * PI;

    double cosF = cos(facing);
    double sinF = sin(facing);
    double mix = (1.0 - cosF) * 0.5;

    // -------- цвета с затенением по дистанции --------
    double shade = (perp_dist > 4.0) ? 4.0 / perp_dist : 1.0;
    if (shade < 0.30) shade = 0.30;

    auto shade3 = [&](int r, int g, int b) -> COLORREF {
        return RGB((int)(r * shade), (int)(g * shade), (int)(b * shade));
    };
    auto mix3 = [](COLORREF a, COLORREF b, double t) -> COLORREF {
        return RGB(
            (int)(GetRValue(a) * (1 - t) + GetRValue(b) * t),
            (int)(GetGValue(a) * (1 - t) + GetGValue(b) * t),
            (int)(GetBValue(a) * (1 - t) + GetBValue(b) * t));
    };

    COLORREF headColor = mix3(shade3(255, 215, 175), shade3(60, 40, 25), mix);
    COLORREF torsoColor = mix3(shade3(70, 110, 220), shade3(30, 50, 120), mix);
    COLORREF armColor = mix3(shade3(55, 90, 195), shade3(22, 38, 95), mix);
    COLORREF legsColor = shade3(45, 45, 60);
    COLORREF shoeColor = shade3(25, 20, 15);
    COLORREF eyeColor = shade3(15, 15, 15);

    HPEN headPen = CreatePen(PS_SOLID, 1, headColor);
    HPEN torsoPen = CreatePen(PS_SOLID, 1, torsoColor);
    HPEN armPen = CreatePen(PS_SOLID, 1, armColor);
    HPEN legsPen = CreatePen(PS_SOLID, 1, legsColor);
    HPEN shoePen = CreatePen(PS_SOLID, 1, shoeColor);
    HPEN oldPen = (HPEN)SelectObject(memDC, headPen);

    // -------- пропорции --------
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

        // Голова — эллипс
        if (au < headHW) {
            double t = au / headHW;
            double bulge = sqrt(1.0 - t * t);
            int hTop = (int)spriteTopF;
            int hBot = yHeadBot;
            int shrink = (int)((1.0 - bulge) * (hBot - hTop) * 0.5);

            SelectObject(memDC, headPen);
            MoveToEx(memDC, screen_left + x, hTop + shrink, NULL);
            LineTo(memDC, screen_left + x, hBot - shrink);
        }

        // Торс + руки
        if (au < torsoHW) {
            bool isArm = (au > torsoHW * 0.72);
            SelectObject(memDC, isArm ? armPen : torsoPen);
            MoveToEx(memDC, screen_left + x, yHeadBot, NULL);
            LineTo(memDC, screen_left + x, yTorsoBot);
        }

        // Ноги + ступни
        if (au < legsHW && au > legsHW * 0.12) {
            SelectObject(memDC, legsPen);
            MoveToEx(memDC, screen_left + x, yTorsoBot, NULL);
            LineTo(memDC, screen_left + x, yLegsBot);

            SelectObject(memDC, shoePen);
            MoveToEx(memDC, screen_left + x, yLegsBot, NULL);
            LineTo(memDC, screen_left + x, yFeetBot);
        }
    }

    // -------- глаза --------
    if (cosF > 0.0) {
        HPEN eyePen = CreatePen(PS_SOLID, 1, eyeColor);
        SelectObject(memDC, eyePen);

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
                MoveToEx(memDC, screen_left + px, ey1, NULL);
                LineTo(memDC, screen_left + px, ey2);
            }
        }
        DeleteObject(eyePen);
    }

    SelectObject(memDC, oldPen);
    DeleteObject(headPen);
    DeleteObject(torsoPen);
    DeleteObject(armPen);
    DeleteObject(legsPen);
    DeleteObject(shoePen);
}

// =====================================================================
//  Прицел в центре вида
// =====================================================================
static void RenderCrosshair(HDC memDC, int cx, int cy) {
    const int gap = 4;
    const int len = 10;

    HPEN outline = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN inner = CreatePen(PS_SOLID, 1, RGB(255, 220, 70));

    // Чёрная обводка (рисуем линии в 3 строки/столбца — жирный контур)
    HPEN old = (HPEN)SelectObject(memDC, outline);
    for (int o = -1; o <= 1; ++o) {
        MoveToEx(memDC, cx - gap - len, cy + o, NULL); LineTo(memDC, cx - gap, cy + o);
        MoveToEx(memDC, cx + gap, cy + o, NULL); LineTo(memDC, cx + gap + len, cy + o);
        MoveToEx(memDC, cx + o, cy - gap - len, NULL); LineTo(memDC, cx + o, cy - gap);
        MoveToEx(memDC, cx + o, cy + gap, NULL); LineTo(memDC, cx + o, cy + gap + len);
    }

    // Жёлтая внутренняя часть
    SelectObject(memDC, inner);
    MoveToEx(memDC, cx - gap - len + 1, cy, NULL); LineTo(memDC, cx - gap - 1, cy);
    MoveToEx(memDC, cx + gap + 1, cy, NULL); LineTo(memDC, cx + gap + len - 1, cy);
    MoveToEx(memDC, cx, cy - gap - len + 1, NULL); LineTo(memDC, cx, cy - gap - 1);
    MoveToEx(memDC, cx, cy + gap + 1, NULL); LineTo(memDC, cx, cy + gap + len - 1);

    // Красная точка в центре
    //for (int dx = -1; dx <= 1; ++dx)
    //    for (int dy = -1; dy <= 1; ++dy)
    //        SetPixel(memDC, cx + dx, cy + dy, RGB(255, 60, 60));

    SelectObject(memDC, old);
    DeleteObject(outline);
    DeleteObject(inner);
}

// =====================================================================
//  Оружие (дробовик справа внизу)
// =====================================================================
static void RenderWeapon(HDC memDC, Player& p, int screen_left, int screen_right) {
    if (p.deathTimer > 0) return;
    (void)screen_left;
    const int bottom = WINDOW_HEIGHT;

    auto quad = [&](int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4,
        COLORREF f, COLORREF l, int lw = 1) {
            POINT q[4] = { {x1,y1},{x2,y2},{x3,y3},{x4,y4} };
            HBRUSH b = CreateSolidBrush(f); HPEN pn = CreatePen(PS_SOLID, lw, l);
            HBRUSH ob = (HBRUSH)SelectObject(memDC, b); HPEN op = (HPEN)SelectObject(memDC, pn);
            Polygon(memDC, q, 4);
            SelectObject(memDC, ob); SelectObject(memDC, op);
            DeleteObject(b); DeleteObject(pn);
    };
    auto disc = [&](int l, int t, int r, int b, COLORREF f, COLORREF lc, int lw = 1) {
        HBRUSH br = CreateSolidBrush(f); HPEN pn = CreatePen(PS_SOLID, lw, lc);
        HBRUSH ob = (HBRUSH)SelectObject(memDC, br); HPEN op = (HPEN)SelectObject(memDC, pn);
        Ellipse(memDC, l, t, r, b);
        SelectObject(memDC, ob); SelectObject(memDC, op);
        DeleteObject(br); DeleteObject(pn);
    };

    const int kick = (p.muzzleFlash > 0) ? (p.muzzleFlash / 4) : 0;

    /* ===== якоря: казённик справа-снизу, дуло слева-чуть-выше ===== */
    const int recvX = screen_right - 110;      // ось ствола у казны
    const int recvY = bottom - 160 + kick;     // верх казённой части
    const int recvBot = bottom - 40 + kick;     // низ коробки
    const int muzX = recvX - 65;             // дуло — вперёд-влево
    const int muzY = recvY - 100 + kick;      // дуло чуть выше (≈20°)

    const int recvHalfW = 22;
    const int muzHalfW = 12;

    auto tAt = [&](int y) { double t = double(recvY - y) / double(recvY - muzY);
    return t < 0 ? 0.0 : (t > 1 ? 1.0 : t); };
    auto axisX = [&](int y) { return recvX + (int)((muzX - recvX) * tAt(y)); };
    auto halfW = [&](int y) { return recvHalfW + (int)((muzHalfW - recvHalfW) * tAt(y)); };

    const COLORREF steelDrk = RGB(14, 14, 20);
    const COLORREF steelMid = RGB(48, 48, 60);
    const COLORREF steelHi = RGB(150, 154, 170);
    const COLORREF woodMid = RGB(108, 64, 30);
    const COLORREF woodDrk = RGB(52, 26, 8);
    const COLORREF skinMid = RGB(226, 178, 140);
    const COLORREF skinEdge = RGB(82, 48, 26);

    /* 1. ствол */
    {
        int xN = axisX(recvY), wN = halfW(recvY);
        int xF = axisX(muzY), wF = halfW(muzY);
        quad(xN - wN, recvY, xN + wN, recvY,
            xF + wF, muzY, xF - wF, muzY,
            steelMid, steelDrk, 2);
        // верхний блик
        quad(xN - wN + wN * 25 / 100, recvY, xN - wN + wN * 75 / 100, recvY,
            xF - wF + wF * 75 / 100, muzY, xF - wF + wF * 25 / 100, muzY,
            steelHi, steelHi, 1);
    }

    /* 2. дульный срез */
    {
        int mx = axisX(muzY), mw = halfW(muzY);
        disc(mx - mw + 2, muzY - 3, mx + mw - 2, muzY + 3, RGB(4, 4, 6), RGB(0, 0, 0), 1);
    }

    /* 3. цевьё (деревянный блок на стволе) */
    {
        int botY = recvY - 20;
        int topY = recvY - 80;
        int x1 = axisX(botY), w1 = halfW(botY) + 7;
        int x2 = axisX(topY), w2 = halfW(topY) + 7;
        quad(x1 - w1, botY, x1 + w1, botY, x2 + w2, topY, x2 - w2, topY,
            woodMid, woodDrk, 2);
        quad(x1 - w1 + 3, botY, x1 + w1 - 3, botY,
            x2 + w2 - 3, topY, x2 - w2 + 3, topY,
            RGB(168, 116, 64), RGB(168, 116, 64), 1);
    }

    /* 4. ствольная коробка */
    {
        int cxT = axisX(recvY);
        POINT r[4] = {
            { cxT - 26,   recvY + 4 },
            { cxT + 26,   recvY + 4 },
            { recvX + 34, recvBot },
            { recvX - 34, recvBot }
        };
        HBRUSH b = CreateSolidBrush(steelMid);
        HPEN   pn = CreatePen(PS_SOLID, 2, steelDrk);
        HBRUSH ob = (HBRUSH)SelectObject(memDC, b);
        HPEN   op = (HPEN)SelectObject(memDC, pn);
        Polygon(memDC, r, 4);
        SelectObject(memDC, ob); SelectObject(memDC, op);
        DeleteObject(b); DeleteObject(pn);
    }

    /* 5. приклад (уходит за нижний край) */
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
        SelectObject(memDC, ob); SelectObject(memDC, op);
        DeleteObject(b); DeleteObject(pn);
    }

    /* 6. правая рука на рукояти */
    {
        int cx = recvX;
        quad(cx + 4, recvBot - 30,
            cx + 32, recvBot - 28,
            cx + 40, recvBot + 40,
            cx + 4, recvBot + 38,
            skinMid, skinEdge, 2);
    }

    /* 7. вспышка */
    if (p.muzzleFlash > 0) {
        int tx = axisX(muzY), ty = muzY - 4;
        int g = 14 + rand() % 5;
        disc(tx - g, ty - g, tx + g, ty + g, RGB(255, 130, 24), RGB(255, 160, 60), 1);
        int c = 5 + rand() % 2;
        disc(tx - c, ty - c, tx + c, ty + c, RGB(255, 252, 214), RGB(255, 255, 246), 1);
    }
}


// =====================================================================
//  HUD
// =====================================================================
static void RenderHUD(HDC memDC, Player& p, int screen_left, int screen_right) {
    const int barW = 180;
    const int barH = 18;
    const int barX = screen_left + 12;
    const int barY = WINDOW_HEIGHT - 32;

    // Фон
    HBRUSH bgBrush = CreateSolidBrush(RGB(20, 20, 20));
    RECT bgR = { barX - 2, barY - 2, barX + barW + 2, barY + barH + 2 };
    FillRect(memDC, &bgR, bgBrush);
    DeleteObject(bgBrush);

    // Заполнение
    int hp = clampi(p.hp, 0, 100);
    int fillW = barW * hp / 100;
    COLORREF hpColor = RGB(60, 200, 60);
    if (hp <= 30)      hpColor = RGB(210, 40, 40);
    else if (hp <= 60) hpColor = RGB(220, 180, 40);

    HBRUSH hpBrush = CreateSolidBrush(hpColor);
    RECT hpR = { barX, barY, barX + fillW, barY + barH };
    FillRect(memDC, &hpR, hpBrush);
    DeleteObject(hpBrush);

    // Рамка
    HPEN framePen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    HPEN oldPen = (HPEN)SelectObject(memDC, framePen);
    HBRUSH oldBr = (HBRUSH)SelectObject(memDC, GetStockObject(NULL_BRUSH));
    Rectangle(memDC, barX, barY, barX + barW, barY + barH);
    SelectObject(memDC, oldPen);
    SelectObject(memDC, oldBr);
    DeleteObject(framePen);

    // Текст
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
void RenderPlayerView(HDC memDC, Player& player, Player& other,
    int screen_left, int screen_right)
{
    const int view_width = screen_right - screen_left;
    const int half_height = WINDOW_HEIGHT / 2;

    // -------- Экран смерти --------
    if (player.deathTimer > 0) {
        HBRUSH deadBrush = CreateSolidBrush(RGB(70, 8, 8));
        RECT fullR = { screen_left, 0, screen_right, WINDOW_HEIGHT };
        FillRect(memDC, &fullR, deadBrush);
        DeleteObject(deadBrush);

        SetBkMode(memDC, TRANSPARENT);
        SetTextColor(memDC, RGB(255, 90, 90));
        const wchar_t* txt = L"ВЫ МЕРТВЫ";
        int tw = (int)wcslen(txt) * 8;
        TextOutW(memDC, screen_left + view_width / 2 - tw / 2,
            WINDOW_HEIGHT / 2 - 10, txt, (int)wcslen(txt));
        return;
    }

    double* zbuffer = new double[view_width];

    // ================= Небо и пол с градиентом =================
    const int BAND = 4;

    for (int y = 0; y < half_height; y += BAND) {
        double t = (double)y / half_height;                 // 0 — сверху, 1 — у горизонта
        int r = (int)(60 * (1 - t) + 22 * t);
        int g = (int)(60 * (1 - t) + 22 * t);
        int b = (int)(78 * (1 - t) + 32 * t);
        HBRUSH br = CreateSolidBrush(RGB(r, g, b));
        RECT rr = { screen_left, y, screen_right, y + BAND };
        FillRect(memDC, &rr, br);
        DeleteObject(br);
    }
    for (int y = half_height; y < WINDOW_HEIGHT; y += BAND) {
        double t = (double)(y - half_height) / (WINDOW_HEIGHT - half_height);
        int r = (int)(35 * (1 - t) + 95 * t);
        int g = (int)(32 * (1 - t) + 80 * t);
        int b = (int)(30 * (1 - t) + 60 * t);
        HBRUSH br = CreateSolidBrush(RGB(r, g, b));
        RECT rr = { screen_left, y, screen_right, y + BAND };
        FillRect(memDC, &rr, br);
        DeleteObject(br);
    }

    // ================= Рейкастинг (DDA) =================
    InitWallPens();

    for (int x = 0; x < view_width; ++x) {
        double ray_angle = (player.angle - FOV / 2.0)
            + ((double)x / (double)view_width) * FOV;

        double dirX = sin(ray_angle);
        double dirY = cos(ray_angle);

        RayHit hit = CastRay(player.x, player.y, dirX, dirY);

        // Fisheye-коррекция
        double perp = hit.dist * cos(ray_angle - player.angle);
        if (perp < 0.05) perp = 0.05;
        zbuffer[x] = perp;

        int wall_height = (int)((double)WINDOW_HEIGHT / perp);
        if (wall_height > WINDOW_HEIGHT * 4) wall_height = WINDOW_HEIGHT * 4;

        int ceiling = half_height - wall_height / 2;
        int floor_ = half_height + wall_height / 2;

        // Затенение: дальше — темнее; боковые грани — темнее
        double fog = 1.0 / (1.0 + perp * perp * 0.09);
        if (hit.side == 1) fog *= 0.72;
        int shade = clampi((int)(fog * 255.0), 0, 255);
        int idx = shade * 63 / 255;

        HPEN oldPen = (HPEN)SelectObject(memDC, g_wallPens[idx]);
        MoveToEx(memDC, screen_left + x, ceiling, NULL);
        LineTo(memDC, screen_left + x, floor_);
        SelectObject(memDC, oldPen);
    }

    // ================= Спрайт второго игрока =================
    RenderOtherPlayer(memDC, player, other, screen_left, view_width, half_height, zbuffer);

    // ================= Прицел =================
    RenderCrosshair(memDC, screen_left + view_width / 2, half_height);

    // ================= Оружие и HUD =================
    RenderWeapon(memDC, player, screen_left, screen_right);
    RenderHUD(memDC, player, screen_left, screen_right);

    delete[] zbuffer;
}

// =====================================================================
//  Главная отрисовка
// =====================================================================
void DrawGame(HDC hdc) {
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, WINDOW_WIDTH, WINDOW_HEIGHT);
    HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBitmap);

    RenderPlayerView(memDC, p1, p2, 0, WINDOW_WIDTH / 2);
    RenderPlayerView(memDC, p2, p1, WINDOW_WIDTH / 2, WINDOW_WIDTH);

    // Разделитель
    HPEN linePen = CreatePen(PS_SOLID, 4, RGB(0, 0, 0));
    HPEN oldPen = (HPEN)SelectObject(memDC, linePen);
    MoveToEx(memDC, WINDOW_WIDTH / 2, 0, NULL);
    LineTo(memDC, WINDOW_WIDTH / 2, WINDOW_HEIGHT);
    SelectObject(memDC, oldPen);
    DeleteObject(linePen);

    // Подсказки
    SetBkMode(memDC, TRANSPARENT);
    SetTextColor(memDC, RGB(230, 230, 230));
    TextOutW(memDC, 10, 10, L"Игрок 1: WASD + ПРОБЕЛ", 22);
    TextOutW(memDC, WINDOW_WIDTH / 2 + 10, 10, L"Игрок 2: СТРЕЛКИ + R.CTRL", 25);

    BitBlt(hdc, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, memDC, 0, 0, SRCCOPY);

    SelectObject(memDC, oldBmp);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}