#include "Hud.h"
#include "Framebuffer.h"
#include "Font.h"

void RenderCrosshair(int cx, int cy) {
    const int gap = 4;
    const int len = 10;
    uint32_t outline = RGB(0, 0, 0);
    uint32_t inner = RGB(255, 220, 70);

    for (int o = -1; o <= 1; ++o) {
        DrawHorizontalLine(cx - gap - len, cx - gap, cy + o, outline);
        DrawHorizontalLine(cx + gap, cx + gap + len, cy + o, outline);
        DrawVerticalLine(cx + o, cy - gap - len, cy - gap, outline);
        DrawVerticalLine(cx + o, cy + gap, cy + gap + len, outline);
    }
    DrawHorizontalLine(cx - gap - len + 1, cx - gap - 1, cy, inner);
    DrawHorizontalLine(cx + gap + 1, cx + gap + len - 1, cy, inner);
    DrawVerticalLine(cx, cy - gap - len + 1, cy - gap - 1, inner);
    DrawVerticalLine(cx, cy + gap + 1, cy + gap + len - 1, inner);
}

void RenderHUD(Player& p, int screen_left) {
    const int barW = 180;
    const int barH = 18;
    const int barX = screen_left + 12;
    const int barY = WINDOW_HEIGHT - 32;

    FillRect(barX - 2, barY - 2, barW + 4, barH + 4, RGB(20, 20, 20));

    int hp = clampi(p.hp, 0, 100);
    int fillW = barW * hp / 100;

    uint32_t hpColor = RGB(60, 200, 60);
    if (hp <= 30)      hpColor = RGB(210, 40, 40);
    else if (hp <= 60) hpColor = RGB(220, 180, 40);

    if (fillW > 0) FillRect(barX, barY, fillW, barH, hpColor);
    FillRect(barX, barY, barW, 1, RGB(200, 200, 200));
    FillRect(barX, barY + barH - 1, barW, 1, RGB(200, 200, 200));
    FillRect(barX, barY, 1, barH, RGB(200, 200, 200));
    FillRect(barX + barW - 1, barY, 1, barH, RGB(200, 200, 200));

    char buf[32];
    wsprintfA(buf, "HP %d", p.hp);
    DrawText(barX + barW + 10, barY + 5, buf, RGB(240, 240, 240), 1);

    wsprintfA(buf, "SCORE %d", p.score);
    DrawText(barX + barW + 10, barY - 15, buf, RGB(240, 240, 240), 1);
}