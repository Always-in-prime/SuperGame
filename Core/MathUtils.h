#pragma once
#include "Common.h"
#include <cmath>

// ============================================================
//  Мелкие математические утилиты.
//  Не зависят от игровой логики, можно использовать везде.
// ============================================================

inline double Clamp(double v, double lo, double hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline int ClampI(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

inline float ClampF(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// Свернуть угол в диапазон [-PI, PI)
inline double WrapAngle(double a) {
    while (a > PI) a -= 2.0 * PI;
    while (a < -PI) a += 2.0 * PI;
    return a;
}

// Линейная интерполяция
inline double Lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

// Интерполяция углов по короткой дуге
inline double LerpAngle(double a, double b, double t) {
    double d = WrapAngle(b - a);
    return a + d * t;
}

// Угловое расстояние (по модулю, всегда >= 0)
inline double AngleDiff(double a, double b) {
    return std::fabs(WrapAngle(a - b));
}

inline bool NearZero(double v, double eps = 1e-9) {
    return v > -eps && v < eps;
}

// Плавное сглаживание (экспоненциальное)
// value → target со скоростью, зависящей от rate и dt
inline double Damp(double value, double target, double rate, double dt) {
    double t = 1.0 - std::exp(-rate * dt);
    return value + (target - value) * t;
}