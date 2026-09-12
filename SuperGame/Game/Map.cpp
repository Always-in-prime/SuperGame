#include "Map.h"
#include <ctime>

char map[MAP_HEIGHT][MAP_WIDTH + 1];

static int s_spawns[16][2];
static int s_spawnCount = 0;

// ---------- PRNG (xorshift) ----------
static unsigned int s_rng = 1;

static unsigned int Rnd() {
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return s_rng;
}

static int RndRange(int lo, int hi) {   // inclusive
    if (hi <= lo) return lo;
    return lo + (int)(Rnd() % (unsigned int)(hi - lo + 1));
}

// ---------------------------------------------------------------------
//  Утилиты
// ---------------------------------------------------------------------

// Проверяет, что прямоугольник (x,y,w,h) полностью лежит на открытом поле ('.'),
// с дополнительным отступом margin вокруг — чтобы препятствия не липли
// вплотную друг к другу.
static bool IsRegionFree(int x, int y, int w, int h, int margin) {
    for (int j = -margin; j < h + margin; ++j) {
        for (int i = -margin; i < w + margin; ++i) {
            int nx = x + i;
            int ny = y + j;
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) return false;
            if (map[ny][nx] != '.') return false;
        }
    }
    return true;
}

static void FillRegion(int x, int y, int w, int h, char c) {
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i)
            map[y + j][x + i] = c;
}

// Flood-fill: все ли проходимые клетки достижимы от первой?
static bool FloodCheck() {
    static bool visited[MAP_HEIGHT][MAP_WIDTH];
    static int  stack[MAP_WIDTH * MAP_HEIGHT][2];

    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            visited[y][x] = false;

    int sx = -1, sy = -1, totalFloors = 0;
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            if (map[y][x] != '#') {
                if (sx < 0) { sx = x; sy = y; }
                ++totalFloors;
            }
    if (sx < 0) return false;

    int top = 0;
    stack[top][0] = sx; stack[top][1] = sy; ++top;
    visited[sy][sx] = true;
    int reached = 0;
    const int dx[4] = { 1,-1, 0, 0 };
    const int dy[4] = { 0, 0, 1,-1 };

    while (top > 0) {
        --top;
        int cx = stack[top][0], cy = stack[top][1];
        ++reached;
        for (int d = 0; d < 4; ++d) {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
            if (visited[ny][nx]) continue;
            if (map[ny][nx] == '#') continue;
            visited[ny][nx] = true;
            stack[top][0] = nx; stack[top][1] = ny; ++top;
        }
    }
    return reached == totalFloors;
}

// Расставляем точки спавна на полу, стараясь держать их далеко друг от друга
static void PlaceSpawns() {
    s_spawnCount = 0;

    int floors[MAP_WIDTH * MAP_HEIGHT][2];
    int nf = 0;
    for (int y = 1; y < MAP_HEIGHT - 1; ++y)
        for (int x = 1; x < MAP_WIDTH - 1; ++x)
            if (map[y][x] == '.') {
                floors[nf][0] = x;
                floors[nf][1] = y;
                ++nf;
            }
    if (nf < 4) return;

    const int TARGET = 8;
    const int MIN_DIST2 = 9;   // минимум 3 тайла между спавнами

    int attempts = 0;
    while (s_spawnCount < TARGET && s_spawnCount < nf && attempts < 500) {
        ++attempts;
        int fi = RndRange(0, nf - 1);
        int sx = floors[fi][0], sy = floors[fi][1];

        bool ok = true;
        for (int i = 0; i < s_spawnCount; ++i) {
            int ddx = s_spawns[i][0] - sx;
            int ddy = s_spawns[i][1] - sy;
            if (ddx * ddx + ddy * ddy < MIN_DIST2) { ok = false; break; }
        }
        if (!ok) continue;

        s_spawns[s_spawnCount][0] = sx;
        s_spawns[s_spawnCount][1] = sy;
        ++s_spawnCount;
    }

    if (s_spawnCount < 2) {
        for (int i = 0; i < nf && s_spawnCount < 2; ++i) {
            bool dup = false;
            for (int k = 0; k < s_spawnCount; ++k)
                if (s_spawns[k][0] == floors[i][0] &&
                    s_spawns[k][1] == floors[i][1]) {
                    dup = true; break;
                }
            if (dup) continue;
            s_spawns[s_spawnCount][0] = floors[i][0];
            s_spawns[s_spawnCount][1] = floors[i][1];
            ++s_spawnCount;
        }
    }
}

// ---------------------------------------------------------------------
//  Пробует разместить блок w×h со свободным отступом margin вокруг.
//  Возвращает true, если удалось.
// ---------------------------------------------------------------------
static bool TryPlaceBlock(int w, int h, int margin) {
    int maxX = MAP_WIDTH - w - margin;
    int maxY = MAP_HEIGHT - h - margin;
    if (maxX < margin) return false;
    if (maxY < margin) return false;

    for (int t = 0; t < 40; ++t) {
        int x = RndRange(margin, maxX);
        int y = RndRange(margin, maxY);
        if (!IsRegionFree(x, y, w, h, margin)) continue;
        FillRegion(x, y, w, h, '#');
        return true;
    }
    return false;
}

// Пробует разместить блок ровно по центру. Если занято — пробует случайно.
static bool TryPlaceCentral(int w, int h) {
    int cx = MAP_WIDTH / 2 - w / 2;
    int cy = MAP_HEIGHT / 2 - h / 2;
    if (IsRegionFree(cx, cy, w, h, 1)) {
        FillRegion(cx, cy, w, h, '#');
        return true;
    }
    return TryPlaceBlock(w, h, 1);
}

// =====================================================================
//  Генератор открытой Quake-style арены
// =====================================================================
void GenerateMap(unsigned int seed) {
    if (seed == 0) seed = (unsigned int)time(nullptr);
    s_rng = seed ? seed : 1;

    for (int attempt = 0; attempt < 40; ++attempt) {
        // ---- 1. Стены по краю + открытое поле внутри ----
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                bool border = (x == 0 || y == 0 ||
                    x == MAP_WIDTH - 1 ||
                    y == MAP_HEIGHT - 1);
                map[y][x] = border ? '#' : '.';
            }
            map[y][MAP_WIDTH] = '\0';
        }

        // ---- 2. Срезаем углы: арена становится восьмиугольной ----
        //        Это визуально превращает «квадратную комнату» в «зал»,
        //        ближе к арене из Quake.
        const int CUT = RndRange(3, 5);
        for (int y = 0; y < MAP_HEIGHT; ++y) {
            for (int x = 0; x < MAP_WIDTH; ++x) {
                int d1 = x + y;
                int d2 = (MAP_WIDTH - 1 - x) + y;
                int d3 = x + (MAP_HEIGHT - 1 - y);
                int d4 = (MAP_WIDTH - 1 - x) + (MAP_HEIGHT - 1 - y);
                if (d1 < CUT || d2 < CUT || d3 < CUT || d4 < CUT)
                    map[y][x] = '#';
            }
        }

        // ---- 3. Центральная структура — «ядро» арены ----
        //        30% — 3×2 (вытянутая платформа), иначе 2×2.
        if ((Rnd() % 100) < 30) TryPlaceCentral(3, 2);
        else                    TryPlaceCentral(2, 2);

        // ---- 4. Крупные колонны 2×2 (главные ориентиры) ----
        int bigCols = RndRange(2, 4);
        for (int i = 0; i < bigCols; ++i) TryPlaceBlock(2, 2, 1);

        // ---- 5. Одиночные столбы 1×1 (мелкая застройка) ----
        int pillars = RndRange(4, 7);
        for (int i = 0; i < pillars; ++i) TryPlaceBlock(1, 1, 1);

        // ---- 6. Короткие стены 1×3 / 3×1 — дают укрытия ----
        int walls = RndRange(1, 3);
        for (int i = 0; i < walls; ++i) {
            if (Rnd() & 1) TryPlaceBlock(3, 1, 1);
            else           TryPlaceBlock(1, 3, 1);
        }

        // ---- 7. Проверки ----
        if (!FloodCheck()) continue;
        PlaceSpawns();
        if (s_spawnCount < 2) continue;

        return;   // успех
    }

    // Fallback — простая открытая арена (на практике почти недостижимо)
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            bool border = (x == 0 || y == 0 ||
                x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1);
            map[y][x] = border ? '#' : '.';
        }
        map[y][MAP_WIDTH] = '\0';
    }
    s_spawns[0][0] = 2; s_spawns[0][1] = 2;
    s_spawns[1][0] = MAP_WIDTH - 3; s_spawns[1][1] = MAP_HEIGHT - 3;
    s_spawnCount = 2;
}

int GetSpawnCount() { return s_spawnCount; }

void GetSpawnPoint(int index, double& outX, double& outY) {
    if (index < 0 || index >= s_spawnCount) { outX = 1.5; outY = 1.5; return; }
    outX = s_spawns[index][0] + 0.5;
    outY = s_spawns[index][1] + 0.5;
}