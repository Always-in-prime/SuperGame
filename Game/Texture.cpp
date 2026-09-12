#pragma once
#include "Texture.h"

uint32_t wallTex[NUM_WALL_TEXTURES][TEX_SIZE][TEX_SIZE];

static inline int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

// Быстрый детерминированный хеш для шума
static inline int Hash(int x, int y) {
    uint32_t h = (uint32_t)(x * 374761393) + (uint32_t)(y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (int)(h & 0xFF);
}

// ---------------------------------------------------------------------
//  0: Кирпич — тёплый красно-коричневый, шахматная кладка
// ---------------------------------------------------------------------
static void GenBrick() {
    uint32_t* T = &wallTex[0][0][0];
    const int BRICK_H = 16;      // высота ряда кирпича
    const int BRICK_W = 32;      // длина кирпича
    const int MORTAR = 2;       // толщина шва

    for (int y = 0; y < TEX_SIZE; ++y) {
        int row = y / BRICK_H;
        int yLocal = y % BRICK_H;
        int xoff = (row & 1) ? (BRICK_W / 2) : 0;
        for (int x = 0; x < TEX_SIZE; ++x) {
            int xx = (x + xoff) & (TEX_SIZE - 1);
            int col = xx / BRICK_W;
            int xLocal = xx % BRICK_W;

            if (yLocal < MORTAR || xLocal < MORTAR) {
                // Шов — тёмно-серый с шумом
                int n = Hash(x, y) & 15;
                T[y * TEX_SIZE + x] = RGB(52 + n, 44 + n, 36 + n);
            }
            else {
                // Кирпич — базовый цвет варьируется от ряда/колонны
                int base = Hash(col * 13, row * 7);
                int r = 128 + (base & 31);
                int g = 68 + (base & 15);
                int b = 52 + (base & 11);

                // Грубый шум по "пятнам" 2x2
                int n = (Hash(x >> 1, y >> 1) & 15) - 8;

                // Лёгкий блик сверху кирпича и тень снизу
                if (yLocal < 4) { r += 22; g += 10; b += 8; }
                if (yLocal > BRICK_H - 4) { r -= 18; g -= 10; b -= 8; }

                T[y * TEX_SIZE + x] = RGB(
                    clamp255(r + n), clamp255(g + n), clamp255(b + n));
            }
        }
    }
}

// ---------------------------------------------------------------------
//  1: Металл — серые панели с швами и заклёпками
// ---------------------------------------------------------------------
static void GenMetal() {
    uint32_t* T = &wallTex[1][0][0];
    const int PANEL = 32;

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            int xl = x % PANEL;
            int yl = y % PANEL;
            int n = (Hash(x >> 1, y >> 1) & 15) - 8;

            int r, g, b;
            if (xl < 1 || yl < 1) {
                // Шов панели
                r = 38; g = 42; b = 52;
            }
            else if (xl == 1 || yl == 1) {
                // Блик по краю панели
                r = 138; g = 146; b = 165;
            }
            else {
                // Основная поверхность — вертикальный градиент
                int shade = 92 + (yl * 34 / PANEL);
                r = shade + 2;
                g = shade + 6;
                b = shade + 20;
            }

            // Заклёпки в 4 углах панели
            int rvx, rvy, d2;
            rvx = xl - 5; rvy = yl - 5; d2 = rvx * rvx + rvy * rvy;
            if (d2 < 6) { r = 165; g = 172; b = 190; }
            rvx = xl - (PANEL - 6); rvy = yl - 5; d2 = rvx * rvx + rvy * rvy;
            if (d2 < 6) { r = 165; g = 172; b = 190; }
            rvx = xl - 5; rvy = yl - (PANEL - 6); d2 = rvx * rvx + rvy * rvy;
            if (d2 < 6) { r = 165; g = 172; b = 190; }
            rvx = xl - (PANEL - 6); rvy = yl - (PANEL - 6); d2 = rvx * rvx + rvy * rvy;
            if (d2 < 6) { r = 165; g = 172; b = 190; }

            T[y * TEX_SIZE + x] = RGB(
                clamp255(r + n), clamp255(g + n), clamp255(b + n));
        }
    }
}

// ---------------------------------------------------------------------
//  2: Дерево — вертикальные доски с горизонтальным зерном
// ---------------------------------------------------------------------
static void GenWood() {
    uint32_t* T = &wallTex[2][0][0];
    const int PLANK_W = 16;

    for (int y = 0; y < TEX_SIZE; ++y) {
        for (int x = 0; x < TEX_SIZE; ++x) {
            int plank = x / PLANK_W;
            int xl = x % PLANK_W;

            if (xl < 1 || xl == PLANK_W - 1) {
                // Тёмная щель между досками
                T[y * TEX_SIZE + x] = RGB(34, 20, 10);
                continue;
            }

            int base = Hash(plank * 91, 0);
            int r = 112 + (base & 31);
            int g = 66 + (base & 15);
            int b = 32 + (base & 11);

            // Горизонтальное зерно — вертикальные волны
            int grain = (int)(Hash(x, y * 3) & 31) - 16;
            int wave = (y + plank * 7) % 12;
            if (wave < 2) r += 18;
            if (wave > 9) r -= 12;

            T[y * TEX_SIZE + x] = RGB(
                clamp255(r + grain),
                clamp255(g + grain / 2),
                clamp255(b + grain / 3));
        }
    }
}

// ---------------------------------------------------------------------
//  3: Камень — серые блоки с шахматной кладкой
// ---------------------------------------------------------------------
static void GenStone() {
    uint32_t* T = &wallTex[3][0][0];
    const int BLOCK = 16;

    for (int y = 0; y < TEX_SIZE; ++y) {
        int brow = y / BLOCK;
        int yl = y % BLOCK;
        int xoff = (brow & 1) ? (BLOCK / 2) : 0;
        for (int x = 0; x < TEX_SIZE; ++x) {
            int xx = (x + xoff) & (TEX_SIZE - 1);
            int bcol = xx / BLOCK;
            int xl = xx % BLOCK;

            if (yl < 1 || xl < 1) {
                int n = Hash(x, y) & 15;
                T[y * TEX_SIZE + x] = RGB(60 + n, 58 + n, 56 + n);
            }
            else {
                int base = Hash(bcol * 19, brow * 23);
                int v = 132 + (base & 51);
                int n = (Hash(x >> 1, y >> 1) & 15) - 8;
                T[y * TEX_SIZE + x] = RGB(
                    clamp255(v + n),
                    clamp255(v + n - 4),
                    clamp255(v + n - 8));
            }
        }
    }
}

void InitTextures() {
    GenBrick();
    GenMetal();
    GenWood();
    GenStone();
}

// ---------------------------------------------------------------------
//  2x2 шахматка: любой квадрат 2×2 клеток содержит все 4 текстуры.
//  Это даёт мгновенное разнообразие и делает участки стены узнаваемыми.
// ---------------------------------------------------------------------
int GetTileTexture(int cx, int cy) {
    return ((cx & 1) << 1) | (cy & 1);
}