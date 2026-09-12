#include "Input.h"
#include <cstring>

// ---------------------------------------------------------------------
void Input_Init(InputSystem& in) {
    memset(in.keys, 0, sizeof(in.keys));
    in.queueCount = 0;
}

// ---------------------------------------------------------------------
void Input_PushEvent(InputSystem& in, int vk, bool isDown) {
    vk = Input_ClampVK(vk);
    if (vk == 0) return;
    if (in.queueCount >= InputSystem::QUEUE_MAX) return;

    in.queue[in.queueCount].vk = vk;
    in.queue[in.queueCount].isDown = isDown;
    in.queueCount++;
}

// ---------------------------------------------------------------------
//  Полное обновление состояния.
//
//  Алгоритм:
//    1. Сбросить pressed/released у всех клавиш (они живут ровно один кадр).
//    2. Пройти по очереди событий:
//       - keydown → held = true, pressed = true
//       - keyup   → held = false, released = true
//    3. Очистить очередь.
//
//  Если в очереди был keydown сразу за keyup — оба флага попадут
//  в один кадр. Это позволяет поймать быстрые тапы.
// ---------------------------------------------------------------------
void Input_PollState(InputSystem& in) {
    for (int i = 0; i < INPUT_MAX_KEYS; ++i) {
        in.keys[i].pressed = false;
        in.keys[i].released = false;
    }

    for (int i = 0; i < in.queueCount; ++i) {
        int vk = in.queue[i].vk;
        bool isDown = in.queue[i].isDown;

        if (isDown) {
            // pressed — только при переходе из не-нажатого.
            if (!in.keys[vk].held) {
                in.keys[vk].pressed = true;
            }
            in.keys[vk].held = true;
        }
        else {
            in.keys[vk].held = false;
            in.keys[vk].released = true;
        }
    }

    in.queueCount = 0;
}

// ---------------------------------------------------------------------
bool Input_Held(const InputSystem& in, int vk) {
    vk = Input_ClampVK(vk);
    return in.keys[vk].held;
}

bool Input_Pressed(const InputSystem& in, int vk) {
    vk = Input_ClampVK(vk);
    return in.keys[vk].pressed;
}

bool Input_Released(const InputSystem& in, int vk) {
    vk = Input_ClampVK(vk);
    return in.keys[vk].released;
}