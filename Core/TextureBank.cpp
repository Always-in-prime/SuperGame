#include "TextureBank.h"
#include <cstring>
#include <cmath>

// ---------------------------------------------------------------------
//  Хранилище
// ---------------------------------------------------------------------
struct TexEntry {
    char     name[32];
    uint32_t pixels[TEXBANK_SIZE][TEXBANK_SIZE];
    bool     used;
};

static TexEntry s_bank[TEXBANK_MAX];
static bool     s_ready = false;

// ---------------------------------------------------------------------
static void GenEmpty(TexEntry& e) {
    // Шахматная заглушка, если генератор вдруг вернул пустоту.
    for (int y = 0; y < TEXBANK_SIZE; ++y) {
        for (int x = 0; x < TEXBANK_SIZE; ++x) {
            bool odd = ((x >> 3) ^ (y >> 3)) & 1;
            e.pixels[y][x] = odd ? RGB(255, 0, 255) : RGB(30, 30, 30);
        }
    }
}

// ---------------------------------------------------------------------
void TexBank_Init() {
    if (s_ready) return;
    s_ready = true;

    for (int i = 0; i < TEXBANK_MAX; ++i) {
        s_bank[i].used = false;
        s_bank[i].name[0] = '\0';
        GenEmpty(s_bank[i]);
    }
}

// ---------------------------------------------------------------------
int TexBank_Find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < TEXBANK_MAX; ++i) {
        if (s_bank[i].used && strcmp(s_bank[i].name, name) == 0)
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------
int TexBank_Register(const char* name,
    uint32_t(*gen)(double u, double v))
{
    TexBank_Init();

    // Уже зарегистрирована?
    int existing = TexBank_Find(name);
    if (existing >= 0) return existing;

    // Ищем свободный слот.
    int slot = -1;
    for (int i = 0; i < TEXBANK_MAX; ++i) {
        if (!s_bank[i].used) { slot = i; break; }
    }
    if (slot < 0) return -1;   // банк переполнен

    // Заполняем.
    TexEntry& e = s_bank[slot];
    strncpy_s(e.name, sizeof(e.name), name, _TRUNCATE);
    e.used = true;

    if (gen) {
        for (int y = 0; y < TEXBANK_SIZE; ++y) {
            for (int x = 0; x < TEXBANK_SIZE; ++x) {
                double u = (double)x / TEXBANK_SIZE;
                double v = (double)y / TEXBANK_SIZE;
                e.pixels[y][x] = gen(u, v);
            }
        }
    }
    // иначе остаётся шахматная заглушка.

    return slot;
}

// ---------------------------------------------------------------------
uint32_t TexBank_Sample(int id, double u, double v) {
    if (id < 0 || id >= TEXBANK_MAX || !s_bank[id].used) {
        return RGB(255, 0, 255);   // магента — «нет текстуры»
    }

    // Нормализация u,v — оборачивание (tile).
    while (u < 0.0) u += 1.0;
    while (u >= 1.0) u -= 1.0;
    while (v < 0.0) v += 1.0;
    while (v >= 1.0) v -= 1.0;

    int x = (int)(u * TEXBANK_SIZE);
    int y = (int)(v * TEXBANK_SIZE);
    if (x < 0) x = 0; if (x >= TEXBANK_SIZE) x = TEXBANK_SIZE - 1;
    if (y < 0) y = 0; if (y >= TEXBANK_SIZE) y = TEXBANK_SIZE - 1;

    return s_bank[id].pixels[y][x];
}