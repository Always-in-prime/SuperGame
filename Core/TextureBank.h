#pragma once
#include "Common.h"
#include <cstdint>

// ============================================================
//  TextureBank — единое хранилище всех текстур объектов.
//  (комментарий сохранён)
// ============================================================

constexpr int kTexBankSize = 64;
constexpr int kTexBankMax = 16;

// Backwards-compatible макросы (постепенно выводим из кода).
#define TEXBANK_SIZE kTexBankSize
#define TEXBANK_MAX  kTexBankMax

void TexBank_Init();

int  TexBank_Register(const char* name,
    uint32_t(*gen)(double u, double v));

int  TexBank_Find(const char* name);

// Сэмпл текстуры по индексу. u,v ∈ [0,1).
// Если id < 0 — возвращает fallback-цвет (magenta, чтобы баг был виден).
//
// Горячий путь: без циклов wrap, прямая индексация, noexcept.
// Для u,v ВНЕ [0,1) используйте TexBank_SampleWrap.
uint32_t TexBank_Sample(int id, double u, double v) noexcept;

// Обёртка с wrap-around. Для случаев, когда u,v могут выйти за [0,1).
uint32_t TexBank_SampleWrap(int id, double u, double v) noexcept;