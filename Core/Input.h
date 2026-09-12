#pragma once
#include "Common.h"

// ============================================================
//  Система ввода — очередь событий клавиатуры.
//
//  Идея:
//    - WndProc пишет WM_KEYDOWN/WM_KEYUP в очередь.
//    - Раз в кадр Input_PollState обновляет массив "зажато/нажато/отпущено".
//    - Player получает PlayerInput (намерения), а не читает клавиатуру.
//
//  Даёт:
//    - Никаких потерянных нажатий (быстрые тапы работают).
//    - Различие held/pressed/released.
//    - Тестируемость (можно подсунуть событие вручную).
// ============================================================

const int INPUT_MAX_KEYS = 256;

// ---- Состояние одной клавиши ----
struct InputKey {
    bool held = false;   // зажата сейчас
    bool pressed = false;   // была нажата именно в этом кадре
    bool released = false;   // была отпущена именно в этом кадре
};

// ---- Система ввода ----
struct InputSystem {
    // Состояние всех клавиш по VK-коду
    InputKey keys[INPUT_MAX_KEYS];

    // Очередь событий, накопленных между кадрами (из WndProc)
    // Каждый элемент — пара (VK-код, нажата ли).
    static const int QUEUE_MAX = 256;
    struct Event { int vk; bool isDown; };
    Event queue[QUEUE_MAX];
    int   queueCount = 0;
};

// ---- API ----
void Input_Init(InputSystem& in);

// Из WndProc: WM_KEYDOWN/WM_SYSKEYDOWN → Input_PushEvent(in, vk, true)
//             WM_KEYUP  /WM_SYSKEYUP  → Input_PushEvent(in, vk, false)
void Input_PushEvent(InputSystem& in, int vk, bool isDown);

// Раз в кадр перед апдейтом: разбирает очередь, обновляет held/pressed/released.
void Input_PollState(InputSystem& in);

// Проверки
bool Input_Held(const InputSystem& in, int vk);
bool Input_Pressed(const InputSystem& in, int vk);
bool Input_Released(const InputSystem& in, int vk);

// Утилита: перевести VK в диапазон 0..INPUT_MAX_KEYS-1
// (для VK_* с большими кодами всё равно не должно выйти за пределы,
// но на всякий случай — безопасный clamp)
inline int Input_ClampVK(int vk) {
    if (vk < 0 || vk >= INPUT_MAX_KEYS) return 0;
    return vk;
}