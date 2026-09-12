#include "TestFramework.h"
#include "Framebuffer.h"
#include "Common.h"

TEST_CASE(FB_Init_NoCrash) {
    FB_Init();
    CHECK_TRUE(true);
}

TEST_CASE(FB_SetPixel_OutOfBounds_NoCrash) {
    SetPixelFast(-1, -1, RGB(255, 0, 0));
    SetPixelFast(WINDOW_WIDTH, WINDOW_HEIGHT, RGB(255, 0, 0));
    SetPixelFast(WINDOW_WIDTH + 1000, 0, RGB(255, 0, 0));
    CHECK_TRUE(true);
}

TEST_CASE(FB_DrawVerticalLine_OutOfBounds) {
    DrawVerticalLine(-5, 0, 100, RGB(255, 0, 0));
    DrawVerticalLine(WINDOW_WIDTH + 5, 0, 100, RGB(255, 0, 0));
    // y0 > y1 — не должно ничего ломать
    DrawVerticalLine(100, 50, 10, RGB(255, 0, 0));
    CHECK_TRUE(true);
}

TEST_CASE(FB_FillRect_ZeroSize) {
    FillRect(100, 100, 0, 0, RGB(0, 255, 0));
    FillRect(100, 100, -5, 10, RGB(0, 255, 0));
    CHECK_TRUE(true);
}

TEST_CASE(FB_BlendAlpha_TransparentIsNoop) {
    // alpha = 0 — ничего не должно меняться. Не можем проверить
    // содержимое (framebuffer внутри), но проверим что не крешится.
    BlendPixelFast(100, 100, RGB(255, 0, 0), 0);
    BlendPixelFast(100, 100, RGB(255, 0, 0), 255);
    CHECK_TRUE(true);
}