#include "Renderer.h"
#include "../Game/Map.h"
#include "../Game/Player.h"
#include "../Game/Texture.h"
#include "Framebuffer.h"
#include "Font.h"
#include "Raycast.h"
#include "Sprite.h"
#include "Weapon.h"
#include "Hud.h"
#include "Overlay.h"
#include <math.h>
#include <stdlib.h>
#include <cstdint>

// ---------------------------------------------------------------------
//  Внутренние утилиты Renderer.
//
//  ВАЖНО: FillRow и FB_RowPtr теперь в Framebuffer.h/.cpp —
//  локальных дубликатов тут быть не должно, иначе C2668 ambiguous.
// ---------------------------------------------------------------------
namespace {

    // Один статический буфер на процесс — без new/delete на кадр.
    double g_zbuffer[WINDOW_WIDTH];

    // Быстрый PRNG для screen shake. xorshift32.
    inline uint32_t FastRand(uint32_t& state) noexcept {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    inline int RandPM100(uint32_t& state) noexcept {
        return static_cast<int>(FastRand(state) % 201u) - 100;
    }

    inline int RandPM10(uint32_t& state) noexcept {
        return static_cast<int>(FastRand(state) % 21u) - 10;
    }

    // Заливка прямоугольника построчно через FillRow из Framebuffer.
    inline void FillRectRows(int x0, int y0, int x1, int y1,
        uint32_t color) noexcept
    {
        for (int y = y0; y < y1; ++y) FillRow(y, x0, x1, color);
    }

}  // namespace

void RenderPlayerView(World& w, int viewerIdx,
    int screen_left, int screen_right)
{
    Player& player = w.players[viewerIdx];
    Player& other = w.players[1 - viewerIdx];

    const int view_width = screen_right - screen_left;
    const int fixed_half = WINDOW_HEIGHT / 2;

    // ---- Мёртвый игрок ----
    if (player.deathTimer > 0) {
        FillRectRows(screen_left, 0, screen_right, WINDOW_HEIGHT,
            RGB(70, 8, 8));
        return;
    }

    // ---- Камера (с тряской) ----
    double camX = player.renderX;
    double camY = player.renderY;
    double camA = player.renderAngle;
    int    half_height = fixed_half;

    if (w.settings.screenShake && player.shakeMag > 0.01f) {
        static uint32_t rngState = 0x9E3779B9u;
        const double s = player.shakeMag;

        camX += RandPM100(rngState) / 100.0 * s * 0.015;
        camY += RandPM100(rngState) / 100.0 * s * 0.015;

        // Явный static_cast: int += double иначе даёт C4244.
        const double shakeY = RandPM10(rngState) / 10.0 * s;
        half_height += static_cast<int>(shakeY);

        if (half_height < 20) half_height = 20;
        if (half_height > WINDOW_HEIGHT - 20) half_height = WINDOW_HEIGHT - 20;
    }

    double* zbuffer = g_zbuffer;

    // ---- Небо ----
    {
        const int rows = half_height;
        int r = 60 << 16;
        int g = 60 << 16;
        int b = 78 << 16;
        const int dr = -((60 - 22) << 16) / (rows > 0 ? rows : 1);
        const int dg = -((60 - 22) << 16) / (rows > 0 ? rows : 1);
        const int db = -((78 - 32) << 16) / (rows > 0 ? rows : 1);

        for (int y = 0; y < rows; ++y, r += dr, g += dg, b += db) {
            const uint32_t sky = RGB((r >> 16), (g >> 16), (b >> 16));
            FillRow(y, screen_left, screen_right, sky);
        }
    }

    // ---- Пол ----
    {
        const int rows = WINDOW_HEIGHT - half_height;
        int r = 35 << 16;
        int g = 32 << 16;
        int b = 30 << 16;
        const int dr = ((95 - 35) << 16) / (rows > 0 ? rows : 1);
        const int dg = ((80 - 32) << 16) / (rows > 0 ? rows : 1);
        const int db = ((60 - 30) << 16) / (rows > 0 ? rows : 1);

        for (int y = half_height; y < WINDOW_HEIGHT; ++y, r += dr, g += dg, b += db) {
            const uint32_t flr = RGB((r >> 16), (g >> 16), (b >> 16));
            FillRow(y, screen_left, screen_right, flr);
        }
    }

    const double camSin = sin(camA);
    const double camCos = cos(camA);
    const double planeLen = tan(FOV * 0.5);

    // ---- Стены ----
    const double invViewW = 1.0 / static_cast<double>(view_width);
    const int    winH = WINDOW_HEIGHT;

    for (int x = 0; x < view_width; ++x) {
        const double cameraX = (2.0 * (x + 0.5) * invViewW - 1.0) * planeLen;
        const double dirX = camSin + cameraX * camCos;
        const double dirY = camCos - cameraX * camSin;

        RayHit hit = CastRay(camX, camY, dirX, dirY);

        double perp = hit.dist;
        if (perp < 0.02) perp = 0.02;
        zbuffer[x] = perp;

        const double pixPerUnit = static_cast<double>(winH) / perp;

        int pixAbove = static_cast<int>((WALL_HEIGHT - EYE_HEIGHT) * pixPerUnit);
        int pixBelow = static_cast<int>(EYE_HEIGHT * pixPerUnit);

        int ceiling = half_height - pixAbove;
        int floor_ = half_height + pixBelow;

        // Отсекаем «бесконечно далёкое», чтобы не сломать int.
        const int kBigLimit = winH * 32;
        if (ceiling < -kBigLimit) ceiling = -kBigLimit;
        if (floor_ > kBigLimit)   floor_ = kBigLimit;

        double fog = 1.0 / (1.0 + perp * perp * 0.09);
        if (hit.side == 1) fog *= 0.72;
        int shade = clampi(static_cast<int>(fog * 256.0), 0, 256);

        int texX = static_cast<int>(hit.wallX * TEX_SIZE);
        if (texX < 0) texX = 0;
        if (texX >= TEX_SIZE) texX = TEX_SIZE - 1;

        int rawHeight = static_cast<int>(pixPerUnit);
        if (rawHeight < 1) rawHeight = 1;
        const double texStep = static_cast<double>(TEX_SIZE) / static_cast<double>(rawHeight);

        DrawTexturedColumn(screen_left + x, ceiling, floor_,
            &wallTex[hit.texId][0][0], texX,
            0.0, texStep, shade);

        // Верхняя и нижняя «полки» стены — 4 пикселя на столбец.
        SetPixelFast(screen_left + x, ceiling + 1, RGB(24, 20, 16));
        SetPixelFast(screen_left + x, ceiling + 2, RGB(38, 30, 24));
        SetPixelFast(screen_left + x, floor_ - 1, RGB(24, 20, 16));
        SetPixelFast(screen_left + x, floor_ - 2, RGB(38, 30, 24));
    }

    RenderOtherPlayer(player, other, screen_left, view_width, half_height, zbuffer);

    RenderCrosshair(screen_left + view_width / 2, fixed_half);
}

// ---------------------------------------------------------------------
//  Главная отрисовка
// ---------------------------------------------------------------------
void DrawGame(HDC hdc, World& w) {
    // FB_Init больше не нужен: небо и пол заливают весь буфер,
    // стены и HUD перекрывают остальное. Если FB_Init делает что-то
    // ещё (например, сброс флага present) — верни его.
    // FB_Init();

    RenderPlayerView(w, 0, 0, WINDOW_WIDTH / 2);
    RenderPlayerView(w, 1, WINDOW_WIDTH / 2, WINDOW_WIDTH);

    // Разделительная линия — два столбца через FillRow.
    {
        const int midX0 = WINDOW_WIDTH / 2;
        const int midX1 = midX0 + 2;
        const uint32_t black = RGB(0, 0, 0);
        for (int y = 0; y < WINDOW_HEIGHT; ++y) {
            uint32_t* row = FB_RowPtr(y);
            row[midX0] = black;
            row[midX0 + 1] = black;
        }
    }

    // Оружие + HUD
    RenderWeapon(w.players[0], WINDOW_WIDTH / 2);
    RenderWeapon(w.players[1], WINDOW_WIDTH);
    RenderHUD(w.players[0], 0);
    RenderHUD(w.players[1], WINDOW_WIDTH / 2);

    // Feedback overlay
    RenderPlayerOverlay(w.players[0], 0, WINDOW_WIDTH / 2);
    RenderPlayerOverlay(w.players[1], WINDOW_WIDTH / 2, WINDOW_WIDTH);

    // Управление
    DrawText(10, 10, "P1: WASD + SPACE", RGB(230, 230, 230), 1);
    DrawText(WINDOW_WIDTH / 2 + 10, 10, "P2: ARROWS + R.Ctrl", RGB(230, 230, 230), 1);

    // ---- Оверлей конца раунда ----
    if (w.match.roundEnding) {
        const int bandH = 60;
        const int bandY = WINDOW_HEIGHT / 2 - bandH / 2;
        FillRect(0, bandY, WINDOW_WIDTH, bandH, RGB(10, 10, 14));
        FillRect(0, bandY, WINDOW_WIDTH, 2, RGB(255, 200, 60));
        FillRect(0, bandY + bandH - 2, WINDOW_WIDTH, 2, RGB(255, 200, 60));

        const char* title = "ROUND OVER";
        const int tw = TextWidth(title, 2);
        DrawText((WINDOW_WIDTH - tw) / 2, bandY + 8, title, RGB(255, 220, 70), 2);

        const char* winner = (w.match.lastWinner == 0) ? "PLAYER 1 WINS" :
            (w.match.lastWinner == 1) ? "PLAYER 2 WINS" :
            "DRAW";
        const int ww = TextWidth(winner, 1);
        DrawText((WINDOW_WIDTH - ww) / 2, bandY + 30, winner, RGB(230, 230, 230), 1);

        char buf[48];
        wsprintfA(buf, "NEXT ARENA IN %d...",
            static_cast<int>(w.match.roundEndTimer + 0.99f));
        const int bw = TextWidth(buf, 1);
        DrawText((WINDOW_WIDTH - bw) / 2, bandY + 44, buf, RGB(180, 180, 180), 1);
    }

    FB_Present(hdc);
}