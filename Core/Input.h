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
//
//  Оптимизация:
//    - Автоповтор WM_KEYDOWN игнорируется (held уже покрывает удержание).
//    - Сброс pressed/released идёт только по "грязным" клавишам
//      прошлого кадра, а не по всем 256.
// ============================================================

constexpr int INPUT_MAX_KEYS = 256;

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

    // Очередь событий, накопленных между кадрами (из WndProc).
    // Каждый элемент — пара (VK-код, нажата ли).
    static constexpr int QUEUE_MAX = 512;
    struct Event { int vk; bool isDown; };
    Event queue[QUEUE_MAX];
    int   queueCount = 0;

    // "Грязные" клавиши: те, у которых в прошлом кадре был выставлен
    // pressed/released. Сбрасываем только их — вместо полного прохода
    // по 256 клавишам.
    static constexpr int DIRTY_MAX = QUEUE_MAX;
    int   dirtyVK[DIRTY_MAX];
    int   dirtyCount = 0;
};

// ---- API ----
void Input_Init(InputSystem& in) noexcept;

// Из WndProc: WM_KEYDOWN/WM_SYSKEYDOWN → Input_PushEvent(in, vk, true)
//             WM_KEYUP  /WM_SYSKEYUP  → Input_PushEvent(in, vk, false)
void Input_PushEvent(InputSystem& in, int vk, bool isDown) noexcept;

// Раз в кадр перед апдейтом: разбирает очередь, обновляет held/pressed/released.
void Input_PollState(InputSystem& in) noexcept;

// Проверки
bool Input_Held(const InputSystem& in, int vk) noexcept;
bool Input_Pressed(const InputSystem& in, int vk) noexcept;
bool Input_Released(const InputSystem& in, int vk) noexcept;

// Утилита: перевести VK в диапазон 0..INPUT_MAX_KEYS-1.
// VK == 0 считается невалидным (не используется WinAPI).
inline int Input_ClampVK(int vk) noexcept {
    if (vk <= 0 || vk >= INPUT_MAX_KEYS) return 0;
    return vk;
}