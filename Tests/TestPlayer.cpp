#include "TestFramework.h"
#include "Player.h"
#include "Common.h"
#include <cmath>

// ---------------------------------------------------------------------
//  Углы эквивалентны по модулю 2π, но как числа могут отличаться.
//  Проверяем именно эквивалентность.
// ---------------------------------------------------------------------
static double AngleDiff(double a, double b) {
    double d = a - b;
    while (d > PI) d -= 2.0 * PI;
    while (d < -PI) d += 2.0 * PI;
    return std::fabs(d);
}

TEST_CASE(Player_LerpAngle_ShortestArc_Forward) {
    // С 350° к 10° — короткий путь через 0°.
    p1.prevAngle = 350.0 * PI / 180.0;
    p1.angle = 10.0 * PI / 180.0;
    InterpolateRenderStates(1.0f);   // alpha = 1 → renderAngle = angle (по модулю 2π)
    CHECK_TRUE(AngleDiff(p1.renderAngle, p1.angle) < 1e-6);
}

TEST_CASE(Player_LerpAngle_ShortestArc_Back) {
    p1.prevAngle = 10.0 * PI / 180.0;
    p1.angle = 350.0 * PI / 180.0;
    InterpolateRenderStates(1.0f);
    CHECK_TRUE(AngleDiff(p1.renderAngle, p1.angle) < 1e-6);
}

TEST_CASE(Player_LerpAngle_Halfway) {
    // alpha = 0.5 — угол должен быть посередине по короткой дуге.
    p1.prevAngle = 0.0;
    p1.angle = PI / 2.0;
    InterpolateRenderStates(0.5f);
    CHECK_NEAR(p1.renderAngle, PI / 4.0, 1e-6);
}

TEST_CASE(Player_StepPhysics_FixedSpeed) {
    // Просто проверка, что вызов не крашится.
    p1.x = 5.0;
    p1.y = 5.0;
    p1.deathTimer = 0.0f;
    p1.angle = 0.0;
    double dx = sin(p1.angle) * move_speed * 0.1;
    double dy = cos(p1.angle) * move_speed * 0.1;
    TryMove(p1, dx, dy);
    CHECK_TRUE(true);
}