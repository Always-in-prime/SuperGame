#include "Application.h"
#include "Renderer.h"
#include "Framebuffer.h"
#include "TextureBank.h"
#include "CharacterModel.h"
#include "WeaponModel.h"
#include "Texture.h"
#include "MathUtils.h"
#include <mmsystem.h>

static_assert(MAX_PHYSICS_STEPS > 0, "MAX_PHYSICS_STEPS must be > 0");

// ---------------------------------------------------------------------
//  Маппинг клавиатуры в PlayerInput.
// ---------------------------------------------------------------------
static PlayerInput BuildP1Input(const InputSystem& in) noexcept {
    PlayerInput pi;

    if (Input_Held(in, 'W')) pi.forward += 1.0;
    if (Input_Held(in, 'S')) pi.forward -= 1.0;

    if (Input_Held(in, 'A')) pi.turn -= 1.0;
    if (Input_Held(in, 'D')) pi.turn += 1.0;

    pi.shoot = Input_Held(in, VK_SPACE);
    pi.shootPressed = Input_Pressed(in, VK_SPACE);

    return pi;
}

static PlayerInput BuildP2Input(const InputSystem& in) noexcept {
    PlayerInput pi;

    if (Input_Held(in, VK_UP))   pi.forward += 1.0;
    if (Input_Held(in, VK_DOWN)) pi.forward -= 1.0;

    if (Input_Held(in, VK_LEFT))  pi.turn -= 1.0;
    if (Input_Held(in, VK_RIGHT)) pi.turn += 1.0;

    pi.shoot = Input_Held(in, VK_RCONTROL);
    pi.shootPressed = Input_Pressed(in, VK_RCONTROL);

    return pi;
}

// ---------------------------------------------------------------------
void App_Init(Application& app, HWND hwnd) {
    app.hwnd = hwnd;
    app.state = GameState::Playing;
    app.lastTimeMs = timeGetTime();
    app.accumulator = 0.0f;

    // Кешируем HDC — GetDC/ReleaseDC каждый кадр не нужен.
    app.hdc = GetDC(hwnd);

    // Настройки: если файла нет — берутся дефолты из структуры.
    Settings_Load(app.settings);

    Input_Init(app.input);

    FB_Init();
    InitTextures();          // текстуры стен (не в банке, отдельно)
    TexBank_Init();          // банк текстур для объектов
    CharacterModel_InitTextures();
    WeaponModel_InitTextures();

    World_Init(app.world, app.settings);
}

// ---------------------------------------------------------------------
void App_OnKey(Application& app, int vk, bool isDown) {
    Input_PushEvent(app.input, vk, isDown);
}

// ---------------------------------------------------------------------
void App_Frame(Application& app) {
    DWORD now = timeGetTime();
    float frameTime = (now - app.lastTimeMs) / 1000.0f;
    app.lastTimeMs = now;
    if (frameTime > MAX_FRAME_TIME) frameTime = MAX_FRAME_TIME;
    app.accumulator += frameTime;

    Input_PollState(app.input);

    PlayerInput inputs[2];
    inputs[0] = BuildP1Input(app.input);
    inputs[1] = BuildP2Input(app.input);

    // ---- Фиксированные шаги физики ----
    int steps = 0;
    while (app.accumulator >= FIXED_DT && steps < MAX_PHYSICS_STEPS) {
        World_SavePrev(app.world);
        World_Step(app.world, FIXED_DT, inputs);
        app.accumulator -= FIXED_DT;
        ++steps;
    }
    if (steps == MAX_PHYSICS_STEPS) app.accumulator = 0.0f;

    // ---- Интерполяция рендера ----
    float alpha = app.accumulator / FIXED_DT;
    World_Interpolate(app.world, alpha);

    // ---- Отрисовка (HDC уже кеширован) ----
    DrawGame(app.hdc, app.world);
}

// ---------------------------------------------------------------------
void App_Shutdown(Application& app) {
    Settings_Save(app.settings);

    if (app.hdc) {
        ReleaseDC(app.hwnd, app.hdc);
        app.hdc = nullptr;
    }
}