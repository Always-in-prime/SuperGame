#include "TextureBank.h"
#include <cstring>
#include <cmath>

namespace {

    struct TexEntry {
        char     name[32];
        uint32_t pixels[kTexBankSize][kTexBankSize];
        bool     used;
    };

    TexEntry s_bank[kTexBankMax];
    bool     s_ready = false;

    // Шахматная заглушка — используется, пока текстура не зарегистрирована.
    void GenEmpty(TexEntry& e) noexcept {
        for (int y = 0; y < kTexBankSize; ++y) {
            for (int x = 0; x < kTexBankSize; ++x) {
                const bool odd = ((x >> 3) ^ (y >> 3)) & 1;
                e.pixels[y][x] = odd ? RGB(255, 0, 255) : RGB(30, 30, 30);
            }
        }
    }

    // Быстрый wrap без циклов. Работает корректно для любого u,v,
    // включая отрицательные.
    inline double Wrap01(double t) noexcept {
        t -= std::floor(t);      // t ∈ [0,1)
        // floor(-0.1) = -1, -0.1 - (-1) = 0.9 — корректно.
        return t;
    }

}  // namespace

// ---------------------------------------------------------------------
void TexBank_Init() {
    if (s_ready) return;
    s_ready = true;

    for (int i = 0; i < kTexBankMax; ++i) {
        s_bank[i].used = false;
        s_bank[i].name[0] = '\0';
        GenEmpty(s_bank[i]);
    }
}

// ---------------------------------------------------------------------
int TexBank_Find(const char* name) {
    if (!name) return -1;
    for (int i = 0; i < kTexBankMax; ++i) {
        if (s_bank[i].used && std::strcmp(s_bank[i].name, name) == 0)
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------
int TexBank_Register(const char* name,
    uint32_t(*gen)(double u, double v))
{
    TexBank_Init();

    const int existing = TexBank_Find(name);
    if (existing >= 0) return existing;

    int slot = -1;
    for (int i = 0; i < kTexBankMax; ++i) {
        if (!s_bank[i].used) { slot = i; break; }
    }
    if (slot < 0) return -1;   // банк переполнен

    TexEntry& e = s_bank[slot];
    strncpy_s(e.name, sizeof(e.name), name, _TRUNCATE);
    e.used = true;

    if (gen) {
        constexpr double kInv = 1.0 / kTexBankSize;
        for (int y = 0; y < kTexBankSize; ++y) {
            for (int x = 0; x < kTexBankSize; ++x) {
                e.pixels[y][x] = gen(x * kInv, y * kInv);
            }
        }
    }

    return slot;
}

// ---------------------------------------------------------------------
uint32_t TexBank_Sample(int id, double u, double v) noexcept {
    // Fallback: невалидный id или незарегистрированная текстура.
    // Единый возврат без ветвлений по флагам.
    if (static_cast<unsigned>(id) >= static_cast<unsigned>(kTexBankMax) ||
        !s_bank[id].used)
    {
        return RGB(255, 0, 255);
    }

    // Горячий путь: u,v ∈ [0,1). Никаких while/fmod.
    // Если это не так — упадём на clamp ниже (безопасно).
    int x = static_cast<int>(u * kTexBankSize);
    int y = static_cast<int>(v * kTexBankSize);
    if (static_cast<unsigned>(x) >= static_cast<unsigned>(kTexBankSize)) {
        x = (x < 0) ? 0 : (kTexBankSize - 1);
    }
    if (static_cast<unsigned>(y) >= static_cast<unsigned>(kTexBankSize)) {
        y = (y < 0) ? 0 : (kTexBankSize - 1);
    }

    return s_bank[id].pixels[y][x];
}

// ---------------------------------------------------------------------
uint32_t TexBank_SampleWrap(int id, double u, double v) noexcept {
    return TexBank_Sample(id, Wrap01(u), Wrap01(v));
}