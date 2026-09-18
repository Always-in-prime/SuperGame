#pragma once
#include "Common.h"
#include <cmath>
#include <cstdint>

// ============================================================
//  Мелкие математические утилиты.
//  Не зависят от игровой логики, можно использовать везде.
//
//  Все функции — noexcept и inline-friendly.
//  WrapAngle и LerpAngle работают за O(1), без циклов.
// ============================================================

inline double Clamp(double v, double lo, double hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline int ClampI(int v, int lo, int hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float ClampF(float v, float lo, float hi) noexcept {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Свернуть угол в диапазон [-PI, PI).
//
// Реализация через std::remainder: одна трансцендентная операция
// вместо while-цикла. Для a ∈ [-PI, PI) — identity.
// На границах: remainder(π, 2π) = π (не -π), это допустимо —
// диапазон полуоткрытый, оба конца эквивалентны по смыслу.
inline double WrapAngle(double a) noexcept {
    // std::remainder(x, y) возвращает x - n*y, где n — ближайшее целое.
    // Результат в [-y/2, y/2] = [-π, π].
    return std::remainder(a, 2.0 * PI);
}

// Линейная интерполяция
inline double Lerp(double a, double b, double t) noexcept {
    return a + (b - a) * t;
}

// Интерполяция углов по короткой дуге
inline double LerpAngle(double a, double b, double t) noexcept {
    const double d = WrapAngle(b - a);
    return a + d * t;
}

// Угловое расстояние (по модулю, всегда >= 0)
inline double AngleDiff(double a, double b) noexcept {
    return std::fabs(WrapAngle(a - b));
}

inline bool NearZero(double v, double eps = 1e-9) noexcept {
    return v > -eps && v < eps;
}

// Плавное сглаживание (экспоненциальное).
// value → target со скоростью, зависящей от rate и dt.
//
// Для dt → 0 ведёт себя линейно: value + (target-value)*rate*dt.
// Для больших dt насыщается к target (экспонента гасит разницу).
inline double Damp(double value, double target, double rate, double dt) noexcept {
    const double t = 1.0 - std::exp(-rate * dt);
    return value + (target - value) * t;
}

// ---- Bit-level утилиты для hot path ----
// Достать канал из упакованного RGB (WinAPI: 0x00BBGGRR).
inline constexpr uint8_t GetR(uint32_t rgb) noexcept {
    return static_cast<uint8_t>(rgb & 0xFFu);
}
inline constexpr uint8_t GetG(uint32_t rgb) noexcept {
    return static_cast<uint8_t>((rgb >> 8) & 0xFFu);
}
inline constexpr uint8_t GetB(uint32_t rgb) noexcept {
    return static_cast<uint8_t>((rgb >> 16) & 0xFFu);
}