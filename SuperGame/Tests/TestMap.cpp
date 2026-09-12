#include "TestFramework.h"
#include "Map.h"

TEST_CASE(Map_Generates) {
    GenerateMap(12345);
    // Периметр должен быть стенами.
    for (int x = 0; x < MAP_WIDTH; ++x) {
        CHECK_EQ(map[0][x], '#');
        CHECK_EQ(map[MAP_HEIGHT - 1][x], '#');
    }
    for (int y = 0; y < MAP_HEIGHT; ++y) {
        CHECK_EQ(map[y][0], '#');
        CHECK_EQ(map[y][MAP_WIDTH - 1], '#');
    }
}

TEST_CASE(Map_HasAtLeastTwoSpawns) {
    for (unsigned seed = 1; seed <= 5; ++seed) {
        GenerateMap(seed);
        CHECK_TRUE(GetSpawnCount() >= 2);
    }
}

TEST_CASE(Map_SpawnsOnFloor) {
    for (unsigned seed = 1; seed <= 5; ++seed) {
        GenerateMap(seed);
        for (int i = 0; i < GetSpawnCount(); ++i) {
            double x, y;
            GetSpawnPoint(i, x, y);
            int cx = (int)x;
            int cy = (int)y;
            CHECK_TRUE(cx >= 0 && cx < MAP_WIDTH);
            CHECK_TRUE(cy >= 0 && cy < MAP_HEIGHT);
            CHECK_TRUE(map[cy][cx] == '.');
        }
    }
}

TEST_CASE(Map_FloodFillConnected) {
    // Проверим, что на сгенерированных картах всё связно.
    for (unsigned seed = 1; seed <= 10; ++seed) {
        GenerateMap(seed);

        static bool visited[MAP_HEIGHT][MAP_WIDTH];
        for (int y = 0; y < MAP_HEIGHT; ++y)
            for (int x = 0; x < MAP_WIDTH; ++x)
                visited[y][x] = false;

        int sx = -1, sy = -1;
        int total = 0;
        for (int y = 0; y < MAP_HEIGHT; ++y)
            for (int x = 0; x < MAP_WIDTH; ++x)
                if (map[y][x] == '.') {
                    if (sx < 0) { sx = x; sy = y; }
                    total++;
                }

        REQUIRE(total > 0);

        int stackX[MAP_WIDTH * MAP_HEIGHT];
        int stackY[MAP_WIDTH * MAP_HEIGHT];
        int top = 0;
        stackX[top] = sx; stackY[top] = sy; top++;
        visited[sy][sx] = true;
        int reached = 0;

        const int dx[4] = { 1,-1,0,0 };
        const int dy[4] = { 0,0,1,-1 };

        while (top > 0) {
            top--;
            int cx = stackX[top], cy = stackY[top];
            reached++;
            for (int d = 0; d < 4; ++d) {
                int nx = cx + dx[d], ny = cy + dy[d];
                if (nx < 0 || nx >= MAP_WIDTH || ny < 0 || ny >= MAP_HEIGHT) continue;
                if (visited[ny][nx]) continue;
                if (map[ny][nx] == '#') continue;
                visited[ny][nx] = true;
                stackX[top] = nx; stackY[top] = ny; top++;
            }
        }

        CHECK_EQ(reached, total);
    }
}

TEST_CASE(Map_DeterministicBySeed) {
    GenerateMap(777);
    char first[MAP_HEIGHT][MAP_WIDTH];
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            first[y][x] = map[y][x];

    GenerateMap(777);
    for (int y = 0; y < MAP_HEIGHT; ++y)
        for (int x = 0; x < MAP_WIDTH; ++x)
            CHECK_EQ(map[y][x], first[y][x]);
}