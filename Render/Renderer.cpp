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

// ---------------------------------------------------------------------
//  Вид одного игрока
// ---------------------------------------------------------------------
void RenderPlayerView(World& w, int viewerIdx,
    int screen_left, int screen_right)
{
    Player& player = w.players[viewerIdx];
    Player& other = w.players[1 - viewerIdx];

    const int view_width = screen_right - screen_left;
    const int fixed_half = WINDOW_HEIGHT / 2;

    // ---- Мёртвый игрок ----
    if (player.deathTimer > 0) {
        uint32_t deadColor = RGB(70, 8, 8);
        for (int y = 0; y < WINDOW_HEIGHT; ++y)
            for (int x = screen_left; x < screen_right; ++x)
                SetPixelFast(x, y, deadColor);
        return;
    }

    // ---- Камера (с тряской) ----
    double camX = player.renderX;
    double camY = player.renderY;
    double camA = player.renderAngle;
    int    half_height = fixed_half;

    if (w.settings.screenShake && player.shakeMag > 0.01f) {
        double s = player.shakeMag;
        camX += ((rand() % 201) - 100) / 100.0 * s * 0.015;
        camY += ((rand() % 201) - 100) / 100.0 * s * 0.015;
        half_height += (int)(((rand() % 21) - 10) / 10.0 * s);
        if (half_height < 20) half_height = 20;
        if (half_height > WINDOW_HEIGHT - 20) half_height = WINDOW_HEIGHT - 20;
    }

    double* zbuffer = new double[view_width];

    // ---- Небо ----
    for (int y = 0; y < half_height; ++y) {
        double t = (double)y / (double)half_height;
        int r = (int)(60 * (1 - t) + 22 * t);
        int g = (int)(60 * (1 - t) + 22 * t);
        int b = (int)(78 * (1 - t) + 32 * t);
        uint32_t sky = RGB(r, g, b);
        for (int x = screen_left; x < screen_right; ++x) SetPixelFast(x, y, sky);
    }
    // ---- Пол ----
    for (int y = half_height; y < WINDOW_HEIGHT; ++y) {
        double t = (double)(y - half_height) / (double)(WINDOW_HEIGHT - half_height);
        int r = (int)(35 * (1 - t) + 95 * t);
        int g = (int)(32 * (1 - t) + 80 * t);
        int b = (int)(30 * (1 - t) + 60 * t);
        uint32_t flr = RGB(r, g, b);
        for (int x = screen_left; x < screen_right; ++x) SetPixelFast(x, y, flr);
    }

    const double camSin = sin(camA);
    const double camCos = cos(camA);
    const double planeLen = tan(FOV * 0.5);

    // ---- Стены ----
    for (int x = 0; x < view_width; ++x) {
        double cameraX = (2.0 * (x + 0.5) / (double)view_width - 1.0) * planeLen;
        double dirX = camSin + cameraX * camCos;
        double dirY = camCos - cameraX * camSin;

        RayHit hit = CastRay(camX, camY, dirX, dirY);

        double perp = hit.dist;
        if (perp < 0.02) perp = 0.02;
        zbuffer[x] = perp;

        // Проекция: 1 мировая единица = WINDOW_HEIGHT / perp пикселей.
        double pixPerUnit = (double)WINDOW_HEIGHT / perp;

        // Потолок на WALL_HEIGHT, пол на 0, глаза на EYE_HEIGHT.
        // Пиксели выше горизонта:   (WALL_HEIGHT - EYE_HEIGHT) * pixPerUnit
        // Пиксели ниже горизонта:   EYE_HEIGHT * pixPerUnit
        int pixAbove = (int)((WALL_HEIGHT - EYE_HEIGHT) * pixPerUnit);
        int pixBelow = (int)(EYE_HEIGHT * pixPerUnit);

        int ceiling = half_height - pixAbove;
        int floor_ = half_height + pixBelow;

        // Отсекаем «бесконечно далёкое», чтобы не сломать int.
        if (ceiling < -WINDOW_HEIGHT * 32) ceiling = -WINDOW_HEIGHT * 32;
        if (floor_ > WINDOW_HEIGHT * 32) floor_ = WINDOW_HEIGHT * 32;

        double fog = 1.0 / (1.0 + perp * perp * 0.09);
        if (hit.side == 1) fog *= 0.72;
        int shade = clampi((int)(fog * 256.0), 0, 256);

        int texX = (int)(hit.wallX * TEX_SIZE);
        if (texX < 0) texX = 0;
        if (texX >= TEX_SIZE) texX = TEX_SIZE - 1;

        int rawHeight = (int)pixPerUnit;   // == WINDOW_HEIGHT / perp
        if (rawHeight < 1) rawHeight = 1;
        double texStep = (double)TEX_SIZE / (double)rawHeight;
        double texStart = 0.0;

        DrawTexturedColumn(screen_left + x, ceiling, floor_,
            &wallTex[hit.texId][0][0], texX,
            texStart, texStep, shade);

        SetPixelFast(screen_left + x, ceiling + 1, RGB(24, 20, 16));
        SetPixelFast(screen_left + x, ceiling + 2, RGB(38, 30, 24));
        SetPixelFast(screen_left + x, floor_ - 1, RGB(24, 20, 16));
        SetPixelFast(screen_left + x, floor_ - 2, RGB(38, 30, 24));
    }

    RenderOtherPlayer(player, other, screen_left, view_width, half_height, zbuffer);

    RenderCrosshair(screen_left + view_width / 2, fixed_half);

    delete[] zbuffer;
}

// ---------------------------------------------------------------------
//  Главная отрисовка
// ---------------------------------------------------------------------
void DrawGame(HDC hdc, World& w) {
    FB_Init();

    RenderPlayerView(w, 0, 0, WINDOW_WIDTH / 2);
    RenderPlayerView(w, 1, WINDOW_WIDTH / 2, WINDOW_WIDTH);

    // Разделительная линия
    for (int y = 0; y < WINDOW_HEIGHT; ++y) {
        SetPixelFast(WINDOW_WIDTH / 2, y, RGB(0, 0, 0));
        SetPixelFast(WINDOW_WIDTH / 2 + 1, y, RGB(0, 0, 0));
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
        int bandY = WINDOW_HEIGHT / 2 - bandH / 2;
        FillRect(0, bandY, WINDOW_WIDTH, bandH, RGB(10, 10, 14));
        FillRect(0, bandY, WINDOW_WIDTH, 2, RGB(255, 200, 60));
        FillRect(0, bandY + bandH - 2, WINDOW_WIDTH, 2, RGB(255, 200, 60));

        const char* title = "ROUND OVER";
        int tw = TextWidth(title, 2);
        DrawText((WINDOW_WIDTH - tw) / 2, bandY + 8, title, RGB(255, 220, 70), 2);

        const char* winner = (w.match.lastWinner == 0) ? "PLAYER 1 WINS" :
            (w.match.lastWinner == 1) ? "PLAYER 2 WINS" :
            "DRAW";
        int ww = TextWidth(winner, 1);
        DrawText((WINDOW_WIDTH - ww) / 2, bandY + 30, winner, RGB(230, 230, 230), 1);

        char buf[48];
        wsprintfA(buf, "NEXT ARENA IN %d...", (int)(w.match.roundEndTimer + 0.99f));
        int bw = TextWidth(buf, 1);
        DrawText((WINDOW_WIDTH - bw) / 2, bandY + 44, buf, RGB(180, 180, 180), 1);
    }

    FB_Present(hdc);
}