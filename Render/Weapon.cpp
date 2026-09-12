#include "Weapon.h"
#include "Framebuffer.h"
#include "Common.h"
#include "Player.h"
#include <math.h>
#include <stdlib.h>

// =====================================================================
//  Оружие — набор примитивов в камера-локальной системе координат.
//    x = вправо
//    y = вперёд (вдоль направления взгляда)   <- СТВОЛ ИДЁТ СЮДА
//    z = вверх
//    Глаз наблюдателя — в (0, 0, 0)
//
//  По каждому пикселю экрана пускаем луч (t*u, t, t*v) и ищем ближайшее
//  пересечение со всеми примитивами. Примитивы:
//    WK_SPHERE  — сфера
//    WK_CYL_Y   — цилиндр вдоль Y (ствол, цевьё, приклад, магазин)
//    WK_CYL_Z   — цилиндр вдоль Z (пистолетная рукоятка, идёт вниз)
// =====================================================================

enum { WK_SPHERE = 0, WK_CYL_Y = 1, WK_CYL_Z = 2 };

struct WP {
    int      kind;
    double   cx, cy, cz;
    double   r;
    double   aMin, aMax;
    uint32_t col;
};

static double s_ambientBoost = 0.0;

// ---------------------------------------------------------------------
//  Освещение: ключевой + контровой + ambient (поднимается при выстреле)
// ---------------------------------------------------------------------
static inline uint32_t Lit(uint32_t base, double nx, double ny, double nz) {
    // Ключевой свет: сверху-слева-спереди
    const double L1x = -0.35, L1y = 0.50, L1z = 0.80;
    double d1 = nx * L1x + ny * L1y + nz * L1z;
    if (d1 < 0) d1 = 0;

    // Контровой: снизу-справа-сзади (подсвечивает тени)
    const double L2x = 0.55, L2y = -0.20, L2z = -0.45;
    double d2 = nx * L2x + ny * L2y + nz * L2z;
    if (d2 < 0) d2 = 0;

    double light = (0.36 + s_ambientBoost) + 0.62 * d1 + 0.22 * d2;
    if (light > 1.25) light = 1.25;

    int r = (int)(GetRValue(base) * light);
    int g = (int)(GetGValue(base) * light);
    int b = (int)(GetBValue(base) * light);
    r = r < 0 ? 0 : (r > 255 ? 255 : r);
    g = g < 0 ? 0 : (g > 255 ? 255 : g);
    b = b < 0 ? 0 : (b > 255 ? 255 : b);
    return RGB(r, g, b);
}

// =====================================================================
void RenderWeapon(Player& p, int screen_right) {
    if (p.deathTimer > 0) return;

    const int screen_left = screen_right - WINDOW_WIDTH / 2;
    const int view_width = WINDOW_WIDTH / 2;
    const int half_h = WINDOW_HEIGHT / 2;

    // ---------- Отдача + покачивание при ходьбе ----------
    double recoil = 0.0;
    if (p.muzzleFlash > 0.0f) {
        recoil = (double)p.muzzleFlash / (double)MUZZLE_TIME;
        if (recoil > 1.0) recoil = 1.0;
        if (recoil < 0.0) recoil = 0.0;
    }

    double bobX = sin(p.walkPhase) * 0.009;
    double bobZ = fabs(cos(p.walkPhase)) * 0.007;
    double offX = bobX + 0.012 * recoil;   // вправо
    double offY = -0.035 * recoil;          // назад к глазу
    double offZ = bobZ + 0.030 * recoil;   // вверх

    s_ambientBoost = recoil * 0.65;

    // ---------- Палитра ----------
    const uint32_t C_METAL = RGB(82, 86, 100);
    const uint32_t C_METAL_D = RGB(38, 42, 54);
    const uint32_t C_METAL_H = RGB(190, 196, 215);
    const uint32_t C_WOOD = RGB(128, 80, 40);
    const uint32_t C_WOOD_D = RGB(58, 32, 14);
    const uint32_t C_WOOD_H = RGB(168, 112, 62);
    const uint32_t C_SKIN = RGB(220, 172, 138);
    const uint32_t C_SKIN_D = RGB(160, 118, 90);
    const uint32_t C_LEATH = RGB(48, 34, 26);
    const uint32_t C_TRIG = RGB(20, 22, 30);
    const uint32_t C_GOLD = RGB(184, 148, 72);

    // ---------- Сборка модели ----------
    WP parts[24];
    int n = 0;

#define ADD_SPH(x, y, z, radius, c) do { if (n < 24) { \
        parts[n].kind = WK_SPHERE; \
        parts[n].cx = (x) + offX; parts[n].cy = (y) + offY; parts[n].cz = (z) + offZ; \
        parts[n].r = (radius); parts[n].aMin = 0; parts[n].aMax = 0; parts[n].col = (c); \
        n++; } } while(0)

#define ADD_CY(x, y1, y2, z, radius, c) do { if (n < 24) { \
        parts[n].kind = WK_CYL_Y; \
        parts[n].cx = (x) + offX; parts[n].cy = 0; parts[n].cz = (z) + offZ; \
        parts[n].r = (radius); parts[n].aMin = (y1) + offY; parts[n].aMax = (y2) + offY; parts[n].col = (c); \
        n++; } } while(0)

#define ADD_CZ(x, y, z1, z2, radius, c) do { if (n < 24) { \
        parts[n].kind = WK_CYL_Z; \
        parts[n].cx = (x) + offX; parts[n].cy = (y) + offY; parts[n].cz = 0; \
        parts[n].r = (radius); parts[n].aMin = (z1) + offZ; parts[n].aMax = (z2) + offZ; parts[n].col = (c); \
        n++; } } while(0)

    // ===========================================================
    //  МОДЕЛЬ: приклад - ствольная коробка - цевьё - ствол
    //  Всё вдоль оси +Y. X-координата ≈ 0.11, Z ≈ -0.13.
    // ===========================================================

    // ---- Приклад (дерево), идёт назад-вниз ----
    ADD_CY(0.135, 0.30, 0.55, -0.155, 0.042, C_WOOD);
    // Тёмная вставка на прикладе сверху
    ADD_CY(0.135, 0.32, 0.52, -0.118, 0.012, C_WOOD_D);

    // ---- Затыльник приклада (торцевая пластина) ----
    ADD_CY(0.135, 0.28, 0.30, -0.155, 0.048, C_METAL_D);

    // ---- Ствольная коробка (толстый металл) ----
    ADD_CY(0.115, 0.55, 0.78, -0.135, 0.045, C_METAL_D);

    // ---- Верхняя планка на ствольной коробке ----
    ADD_CY(0.115, 0.55, 0.78, -0.092, 0.010, C_METAL);

    // ---- Цевьё (дерево, обёрнуто вокруг ствола) ----
    ADD_CY(0.11, 0.78, 1.02, -0.125, 0.038, C_WOOD);
    // Окантовка цевья спереди и сзади
    ADD_CY(0.11, 0.78, 0.80, -0.125, 0.041, C_WOOD_D);
    ADD_CY(0.11, 1.00, 1.02, -0.125, 0.041, C_WOOD_D);
    // Светлый блик сверху цевья
    ADD_CY(0.11, 0.80, 1.00, -0.090, 0.009, C_WOOD_H);

    // ---- Ствол (тонкий и длинный) ----
    ADD_CY(0.105, 1.02, 1.72, -0.120, 0.014, C_METAL);

    // ---- Блик на стволе сверху ----
    ADD_CY(0.105, 1.02, 1.72, -0.108, 0.004, C_METAL_H);

    // ---- Газовая трубка над стволом ----
    ADD_CY(0.105, 1.02, 1.45, -0.092, 0.006, C_METAL_H);

    // ---- Мушка ----
    ADD_CY(0.105, 1.66, 1.70, -0.095, 0.008, C_METAL_D);

    // ---- Дульный срез (тёмное отверстие) ----
    ADD_CY(0.105, 1.70, 1.72, -0.120, 0.017, C_METAL_D);

    // ---- Пистолетная рукоятка (вниз от ствольной коробки) ----
    ADD_CZ(0.115, 0.62, -0.30, -0.15, 0.030, C_LEATH);
    // Пята рукоятки
    ADD_CZ(0.115, 0.62, -0.32, -0.30, 0.032, C_METAL_D);

    // ---- Спусковая скоба ----
    ADD_CY(0.11, 0.58, 0.68, -0.21, 0.005, C_TRIG);

    // ---- Магазин под ствольной коробкой (наклонный, эмулируем прямым) ----
    ADD_CZ(0.115, 0.66, -0.26, -0.16, 0.022, C_METAL_D);
    // Золотистая окантовка магазина
    ADD_CZ(0.115, 0.66, -0.26, -0.24, 0.024, C_GOLD);

#undef ADD_SPH
#undef ADD_CY
#undef ADD_CZ

    // ---------- Проекция ----------
    const double planeLen = tan(FOV * 0.5);

    // Регион обработки — правая половина вида, чуть выше центра и до низа
    int sx0 = screen_left + view_width / 6;
    int sx1 = screen_right - 1;
    int sy0 = half_h - 60;
    int sy1 = WINDOW_HEIGHT - 1;

    // ---------- По-пиксельный рейкаст ----------
    for (int sy = sy0; sy <= sy1; ++sy) {
        double v = ((double)(half_h - sy) / (double)view_width) * planeLen;

        for (int sx = sx0; sx <= sx1; ++sx) {
            double u = ((2.0 * (sx - screen_left + 0.5) / (double)view_width) - 1.0) * planeLen;

            double bestT = 1e30;
            int    bestI = -1;
            double bnx = 0, bny = 0, bnz = 0;

            for (int i = 0; i < n; ++i) {
                const WP& pt = parts[i];
                double tt;
                double lnx, lny, lnz;

                if (pt.kind == WK_SPHERE) {
                    double ox = -pt.cx, oy = -pt.cy, oz = -pt.cz;
                    double A = u * u + 1.0 + v * v;
                    double B = 2.0 * (u * ox + oy + v * oz);
                    double Cc = ox * ox + oy * oy + oz * oz - pt.r * pt.r;
                    double disc = B * B - 4.0 * A * Cc;
                    if (disc < 0) continue;
                    tt = (-B - sqrt(disc)) / (2.0 * A);
                    if (tt < 0.05 || tt >= bestT) continue;

                    double hx = tt * u - pt.cx;
                    double hy = tt - pt.cy;
                    double hz = tt * v - pt.cz;
                    double nl = sqrt(hx * hx + hy * hy + hz * hz);
                    if (nl < 1e-6) continue;
                    lnx = hx / nl; lny = hy / nl; lnz = hz / nl;
                    bestT = tt; bestI = i;
                    bnx = lnx; bny = lny; bnz = lnz;
                }
                else if (pt.kind == WK_CYL_Y) {
                    // --- боковая поверхность ---
                    double A = u * u + v * v;
                    if (A > 1e-9) {
                        double B = -2.0 * (u * pt.cx + v * pt.cz);
                        double Cc = pt.cx * pt.cx + pt.cz * pt.cz - pt.r * pt.r;
                        double disc = B * B - 4.0 * A * Cc;
                        if (disc >= 0) {
                            double sq = sqrt(disc);
                            double t0 = (-B - sq) / (2.0 * A);
                            double t1 = (-B + sq) / (2.0 * A);
                            // Выбираем ближайшее положительное в диапазоне [aMin, aMax]
                            double cand[2] = { t0, t1 };
                            for (int k = 0; k < 2; ++k) {
                                double tk = cand[k];
                                if (tk < 0.05) continue;
                                if (tk < pt.aMin || tk > pt.aMax) continue;
                                if (tk >= bestT) continue;

                                double hx = tk * u - pt.cx;
                                double hz = tk * v - pt.cz;
                                double nl = sqrt(hx * hx + hz * hz);
                                if (nl < 1e-6) continue;
                                bestT = tk; bestI = i;
                                bnx = hx / nl; bny = 0; bnz = hz / nl;
                            }
                        }
                    }
                    // --- задняя крышка y = aMin ---
                    if (pt.aMin > 0.05 && pt.aMin < bestT) {
                        double tk = pt.aMin;
                        double hx = tk * u - pt.cx;
                        double hz = tk * v - pt.cz;
                        if (hx * hx + hz * hz <= pt.r * pt.r) {
                            bestT = tk; bestI = i;
                            bnx = 0; bny = -1; bnz = 0;
                        }
                    }
                    // --- передняя крышка y = aMax ---
                    if (pt.aMax > 0.05 && pt.aMax < bestT) {
                        double tk = pt.aMax;
                        double hx = tk * u - pt.cx;
                        double hz = tk * v - pt.cz;
                        if (hx * hx + hz * hz <= pt.r * pt.r) {
                            bestT = tk; bestI = i;
                            bnx = 0; bny = 1; bnz = 0;
                        }
                    }
                }
                else { // WK_CYL_Z
                    // --- боковая поверхность ---
                    double A = u * u + 1.0;
                    if (A > 1e-9) {
                        double B = -2.0 * (u * pt.cx + pt.cy);
                        double Cc = pt.cx * pt.cx + pt.cy * pt.cy - pt.r * pt.r;
                        double disc = B * B - 4.0 * A * Cc;
                        if (disc >= 0) {
                            double sq = sqrt(disc);
                            double t0 = (-B - sq) / (2.0 * A);
                            double t1 = (-B + sq) / (2.0 * A);
                            double cand[2] = { t0, t1 };
                            for (int k = 0; k < 2; ++k) {
                                double tk = cand[k];
                                if (tk < 0.05) continue;
                                double zp = tk * v;
                                if (zp < pt.aMin || zp > pt.aMax) continue;
                                if (tk >= bestT) continue;

                                double hx = tk * u - pt.cx;
                                double hy = tk - pt.cy;
                                double nl = sqrt(hx * hx + hy * hy);
                                if (nl < 1e-6) continue;
                                bestT = tk; bestI = i;
                                bnx = hx / nl; bny = hy / nl; bnz = 0;
                            }
                        }
                    }
                    // --- нижняя / верхняя крышки (z = aMin / aMax) ---
                    if (fabs(v) > 1e-6) {
                        double cand[2] = { pt.aMin / v, pt.aMax / v };
                        for (int k = 0; k < 2; ++k) {
                            double tk = cand[k];
                            if (tk < 0.05 || tk >= bestT) continue;
                            double hx = tk * u - pt.cx;
                            double hy = tk - pt.cy;
                            if (hx * hx + hy * hy <= pt.r * pt.r) {
                                bestT = tk; bestI = i;
                                bnx = 0; bny = 0;
                                bnz = (k == 0) ? -1.0 : 1.0;
                            }
                        }
                    }
                }
            }

            if (bestI >= 0) {
                uint32_t c = Lit(parts[bestI].col, bnx, bny, bnz);
                SetPixelFast(sx, sy, c);
            }
        }
    }

    // ---------- Дульная вспышка ----------
    if (p.muzzleFlash > 0.0f) {
        // Позиция дула в камера-локальных координатах
        double mx = 0.105 + offX;
        double my = 1.72 + offY;
        double mz = -0.120 + offZ;

        double inv = 1.0 / my;
        double nu = (mx * inv) / planeLen;
        double nv = (mz * inv) / planeLen;

        int fsx = screen_left + (int)((nu * 0.5 + 0.5) * view_width);
        int fsy = half_h - (int)(nv * view_width);

        // Большое свечение
        int rad = 16 + rand() % 6;
        for (int dy = -rad; dy <= rad; ++dy) {
            for (int dx = -rad; dx <= rad; ++dx) {
                int px = fsx + dx;
                int py = fsy + dy;
                if (px < 0 || px >= WINDOW_WIDTH) continue;
                if (py < 0 || py >= WINDOW_HEIGHT) continue;
                int d2 = dx * dx + dy * dy;
                if (d2 > rad * rad) continue;
                double t = (double)d2 / (double)(rad * rad);
                int alpha = (int)(220.0 * (1.0 - t));
                if (alpha < 0) alpha = 0;
                BlendPixelFast(px, py, RGB(255, 150, 50), alpha);
            }
        }
        // Яркое ядро
        int r2 = 6 + rand() % 3;
        for (int dy = -r2; dy <= r2; ++dy) {
            for (int dx = -r2; dx <= r2; ++dx) {
                int px = fsx + dx;
                int py = fsy + dy;
                if (px < 0 || px >= WINDOW_WIDTH) continue;
                if (py < 0 || py >= WINDOW_HEIGHT) continue;
                if (dx * dx + dy * dy <= r2 * r2) {
                    SetPixelFast(px, py, RGB(255, 250, 220));
                }
            }
        }
    }
}