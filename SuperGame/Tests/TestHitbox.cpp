#include "TestFramework.h"
#include "Hitbox.h"
#include "CharacterModel.h"
#include "Common.h"
#include <cmath>

// ---------------------------------------------------------------------
//  Помощник
// ---------------------------------------------------------------------
static HitInfo ShootFrom(double sx, double sy, double angle,
    double tx, double ty, double targetAngle)
{
    double dirx = std::sin(angle);
    double diry = std::cos(angle);
    return RaycastCharacter(sx, sy, dirx, diry, tx, ty, targetAngle);
}

// =====================================================================
//  Попадание в цель (центр)
//
//  ВАЖНО: при стрельбе по оси X у цели с углом 0 левое плечо (cx=-0.235)
//  стоит на пути раньше торса. Чтобы гарантированно попасть в торс,
//  поворачиваем цель на 90° — плечи уходят на ось Y, торс остаётся под лучом.
// =====================================================================

TEST_CASE(Hit_CenterAtClose) {
    HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 6.0, 5.0, PI / 2.0);
    CHECK_TRUE(h.hit);
    CHECK_TRUE(h.distance > 0.0);
    CHECK_TRUE(h.distance < 1.0);
}

TEST_CASE(Hit_CenterAt2m) {
    // Цель повёрнута на 90°, попадаем точно в торс (r=0.185)
    HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 7.0, 5.0, PI / 2.0);
    REQUIRE(h.hit);
    CHECK_EQ(h.partIdx, 1);   // PART_TORSO
    CHECK_NEAR(h.distance, 2.0 - 0.185, 0.05);
}

TEST_CASE(Hit_CenterAt5m) {
    HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 10.0, 5.0, PI / 2.0);
    REQUIRE(h.hit);
    CHECK_EQ(h.partIdx, 1);   // PART_TORSO
    CHECK_NEAR(h.distance, 5.0 - 0.185, 0.05);
}

// =====================================================================
//  Промах
// =====================================================================

TEST_CASE(Miss_BehindTarget) {
    HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 4.0, 5.0, 0.0);
    CHECK_FALSE(h.hit);
}

TEST_CASE(Miss_Sideways) {
    HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 7.0, 8.0, 0.0);
    CHECK_FALSE(h.hit);
}

TEST_CASE(Hit_VeryFar) {
    // На больших расстояниях модель всё ещё перехватывает луч.
    // Цель повёрнута, чтобы плечи не мешали.
    HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 105.0, 5.0, PI / 2.0);
    REQUIRE(h.hit);
    CHECK_EQ(h.partIdx, 1);
    CHECK_NEAR(h.distance, 100.0 - 0.185, 0.1);
}

// =====================================================================
//  Поворот модели
// =====================================================================

TEST_CASE(Hit_Rotated_TargetStillHit) {
    // При любом повороте цели центр тела остаётся на месте.
    for (int deg = 0; deg < 360; deg += 45) {
        double a = deg * PI / 180.0;
        HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 7.0, 5.0, a);
        CHECK_TRUE(h.hit);
    }
}

TEST_CASE(Hit_Rotated_ShoulderRotates) {
    // При повороте цели на 90° её "правое" плечо (локально cx=+0.235)
    // переезжает вперёд по мировой оси Y (py - 0.235).
    // Стреляем с севера (из (5, 3) на юг, угол = π) — попадём в это плечо.
    HitInfo h = ShootFrom(5.0, 3.0, PI, 5.0, 5.0, PI / 2.0);
    CHECK_TRUE(h.hit);
}

// =====================================================================
//  Части тела
//
//  ВАЖНО: голова и торс оба находятся в (cx=0, cy=0). Луч в XZ-плоскости
//  не различает их по высоте (у нас нет pitch). Значит попасть
//  "в голову" отдельно от "в торс" невозможно — это by design.
//
//  Проверяем то, что можем: попадание в конкретные XZ-смещённые части.
// =====================================================================

TEST_CASE(Hit_PartIdx_Torso_WhenNoObstacle) {
    HitInfo h = ShootFrom(5.0, 5.0, PI / 2.0, 7.0, 5.0, PI / 2.0);
    REQUIRE(h.hit);
    CHECK_EQ(h.partIdx, 1);   // PART_TORSO
}

TEST_CASE(Hit_PartIdx_Shoulder_WhenBlockingPath) {
    // Цель смотрит на север (угол 0). Стреляем с востока на запад
    // вдоль мировой оси X — на пути первым стоит левое плечо (cx=-0.235).
    HitInfo h = ShootFrom(7.0, 5.0, -PI / 2.0, 5.0, 5.0, 0.0);
    REQUIRE(h.hit);
    CHECK_TRUE(h.partIdx == 2 || h.partIdx == 3);   // плечо
}

// =====================================================================
//  maxT
// =====================================================================

TEST_CASE(MaxT_InsideRange) {
    HitInfo h = RaycastCharacter(5.0, 5.0, 1.0, 0.0, 7.0, 5.0, PI / 2.0, 5.0);
    CHECK_TRUE(h.hit);
}

TEST_CASE(MaxT_TooShort) {
    HitInfo h = RaycastCharacter(5.0, 5.0, 1.0, 0.0, 10.0, 5.0, PI / 2.0, 2.0);
    CHECK_FALSE(h.hit);
}

// =====================================================================
//  Регрессия на знак
// =====================================================================

TEST_CASE(Regression_SignBug) {
    // До фикса попадали только в упор. Проверим, что с 2 м попадаем.
    HitInfo h = ShootFrom(2.0, 2.0, 0.0, 2.0, 4.0, PI / 2.0);
    CHECK_TRUE(h.hit);
    CHECK_TRUE(h.distance > 1.5);
}