#include "Overlay.h"
#include "Framebuffer.h"
#include "Font.h"

void RenderPlayerOverlay(Player& p, int screen_left, int screen_right) {
    const int view_w = screen_right - screen_left;
    const int cx = screen_left + view_w / 2;
    const int cy = WINDOW_HEIGHT / 2;

    // ---- Damage flash Ч красна€ виньетка ----
    if (p.damageFlashTimer > 0.0f) {
        float t = p.damageFlashTimer / DAMAGE_FLASH_TIME;
        if (t > 1.0f) t = 1.0f;
        int maxAlpha = (int)(200.0f * t);
        if (maxAlpha > 200) maxAlpha = 200;
        BlendVignette(screen_left, screen_right, RGB(220, 20, 20), maxAlpha, 130);
    }

    // ---- Hit marker ----
    if (p.hitMarkerTimer > 0.0f) {
        const int inner = 5;
        const int outer = 12;
        uint32_t white = RGB(255, 255, 255);
        uint32_t black = RGB(0, 0, 0);

        DrawLine(cx - outer + 1, cy - outer + 1, cx - inner + 1, cy - inner + 1, black);
        DrawLine(cx + outer + 1, cy - outer + 1, cx + inner + 1, cy - inner + 1, black);
        DrawLine(cx - outer + 1, cy + outer + 1, cx - inner + 1, cy + inner + 1, black);
        DrawLine(cx + outer + 1, cy + outer + 1, cx + inner + 1, cy + inner + 1, black);

        DrawLine(cx - outer, cy - outer, cx - inner, cy - inner, white);
        DrawLine(cx + outer, cy - outer, cx + inner, cy - inner, white);
        DrawLine(cx - outer, cy + outer, cx - inner, cy + inner, white);
        DrawLine(cx + outer, cy + outer, cx + inner, cy + inner, white);
    }

    // ---- Kill confirm ----
    if (p.killConfirmTimer > 0.0f) {
        const char* txt = "FRAG!";
        int tw = TextWidth(txt, 3);
        int tx = cx - tw / 2;
        int ty = cy - 60;
        DrawText(tx + 2, ty + 2, txt, RGB(30, 0, 0), 3);
        DrawText(tx, ty, txt, RGB(255, 90, 90), 3);
    }

    // ---- Death notify ----
    if (p.deathNotifyTimer > 0.0f) {
        const char* txt = "YOU DIED";
        int tw = TextWidth(txt, 2);
        int tx = cx - tw / 2;
        int ty = cy + 30;
        DrawText(tx + 2, ty + 2, txt, RGB(0, 0, 0), 2);
        DrawText(tx, ty, txt, RGB(230, 50, 50), 2);
    }
}