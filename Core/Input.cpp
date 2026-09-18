#include "Input.h"
#include <cstring>

// ---------------------------------------------------------------------
void Input_Init(InputSystem& in) noexcept {
    memset(in.keys, 0, sizeof(in.keys));
    in.queueCount = 0;
    in.dirtyCount = 0;
}

// ---------------------------------------------------------------------
//  Автоповтор WM_KEYDOWN игнорируем: если клавиша уже held, повторный
//  keydown не несёт новой информации, но забивает очередь.
//
//  WM_KEYUP для не-held клавиши тоже игнорируем — иначе released
//  выставится для клавиши, которую никто не нажимал.
// ---------------------------------------------------------------------
void Input_PushEvent(InputSystem& in, int vk, bool isDown) noexcept {
    vk = Input_ClampVK(vk);
    if (vk == 0) return;

    const InputKey& k = in.keys[vk];

    // Автоповтор: keydown при уже зажатой клавише — пропускаем.
    if (isDown && k.held) return;

    // keyup для не-нажатой клавиши — пропускаем.
    if (!isDown && !k.held) return;

    if (in.queueCount >= InputSystem::QUEUE_MAX) return;

    in.queue[in.queueCount].vk = vk;
    in.queue[in.queueCount].isDown = isDown;
    in.queueCount++;
}

// ---------------------------------------------------------------------
//  Полное обновление состояния.
//
//  Алгоритм:
//    1. Сбросить pressed/released у клавиш, которые были "грязными"
//       в прошлом кадре (а не у всех 256).
//    2. Пройти по очереди событий:
//       - keydown → held = true, pressed = true (если не был held)
//       - keyup   → held = false, released = true
//    3. Запомнить новые "грязные" клавиши.
//    4. Очистить очередь.
//
//  Если в очереди был keydown сразу за keyup — оба флага попадут
//  в один кадр. Это позволяет поймать быстрые тапы.
// ---------------------------------------------------------------------
void Input_PollState(InputSystem& in) noexcept {
    // 1. Сброс только тех клавиш, что были грязными в прошлом кадре.
    for (int i = 0; i < in.dirtyCount; ++i) {
        const int vk = in.dirtyVK[i];
        in.keys[vk].pressed = false;
        in.keys[vk].released = false;
    }
    in.dirtyCount = 0;

    // 2. Разбор очереди.
    for (int i = 0; i < in.queueCount; ++i) {
        const int vk = in.queue[i].vk;
        const bool isDown = in.queue[i].isDown;

        InputKey& k = in.keys[vk];

        if (isDown) {
            if (!k.held) {
                k.pressed = true;
                k.held = true;

                if (in.dirtyCount < InputSystem::DIRTY_MAX) {
                    in.dirtyVK[in.dirtyCount++] = vk;
                }
            }
            // Автоповтор сюда уже не доходит — отфильтрован в PushEvent.
        }
        else {
            k.held = false;
            k.released = true;

            if (in.dirtyCount < InputSystem::DIRTY_MAX) {
                in.dirtyVK[in.dirtyCount++] = vk;
            }
        }
    }

    // 3. Очистка очереди.
    in.queueCount = 0;
}

// ---------------------------------------------------------------------
bool Input_Held(const InputSystem& in, int vk) noexcept {
    vk = Input_ClampVK(vk);
    return in.keys[vk].held;
}

bool Input_Pressed(const InputSystem& in, int vk) noexcept {
    vk = Input_ClampVK(vk);
    return in.keys[vk].pressed;
}

bool Input_Released(const InputSystem& in, int vk) noexcept {
    vk = Input_ClampVK(vk);
    return in.keys[vk].released;
}