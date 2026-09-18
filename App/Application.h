#pragma once
#include "Common.h"
#include "Config.h"
#include "World.h"
#include "PlayerInput.h"
#include "Input.h"
#include "../Core/Settings.h"

enum class GameState {
    Playing,
};

struct Application {
    HWND         hwnd;
    HDC          hdc;
    GameState    state;
    World        world;
    GameSettings settings;
    InputSystem  input;

    DWORD        lastTimeMs;
    float        accumulator;
};

void App_Init(Application& app, HWND hwnd);
void App_Frame(Application& app);
void App_Shutdown(Application& app);

void App_OnKey(Application& app, int vk, bool isDown);