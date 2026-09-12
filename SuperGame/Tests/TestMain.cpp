#include "TestFramework.h"
#include <cstdio>

// ---- Hitbox ----
void Hit_CenterAtClose();
void Hit_CenterAt2m();
void Hit_CenterAt5m();
void Miss_BehindTarget();
void Miss_Sideways();
void Hit_VeryFar();
void Hit_Rotated_TargetStillHit();
void Hit_Rotated_ShoulderRotates();
void Hit_PartIdx_Torso_WhenNoObstacle();
void Hit_PartIdx_Shoulder_WhenBlockingPath();
void MaxT_InsideRange();
void MaxT_TooShort();
void Regression_SignBug();

// ---- CharacterModel ----
void Model_HasParts();
void Model_HeadIsHighest();
void Model_LegsAreLowest();
void Model_AllRadiiPositive();
void Model_AllDamageMultPositive();
void Model_HasTorso();

// ---- Map ----
void Map_Generates();
void Map_HasAtLeastTwoSpawns();
void Map_SpawnsOnFloor();
void Map_FloodFillConnected();
void Map_DeterministicBySeed();

// ---- Framebuffer ----
void FB_Init_NoCrash();
void FB_SetPixel_OutOfBounds_NoCrash();
void FB_DrawVerticalLine_OutOfBounds();
void FB_FillRect_ZeroSize();
void FB_BlendAlpha_TransparentIsNoop();

// ---- Player ----
void Player_LerpAngle_ShortestArc_Forward();
void Player_LerpAngle_ShortestArc_Back();
void Player_LerpAngle_Halfway();
void Player_StepPhysics_FixedSpeed();

int main() {
    std::printf("=========================================\n");
    std::printf("  SuperGame - Unit Tests\n");
    std::printf("=========================================\n\n");

    std::printf("[Hitbox]\n");
    RUN_TEST(Hit_CenterAtClose);
    RUN_TEST(Hit_CenterAt2m);
    RUN_TEST(Hit_CenterAt5m);
    RUN_TEST(Miss_BehindTarget);
    RUN_TEST(Miss_Sideways);
    RUN_TEST(Hit_VeryFar);
    RUN_TEST(Hit_Rotated_TargetStillHit);
    RUN_TEST(Hit_Rotated_ShoulderRotates);
    RUN_TEST(Hit_PartIdx_Torso_WhenNoObstacle);
    RUN_TEST(Hit_PartIdx_Shoulder_WhenBlockingPath);
    RUN_TEST(MaxT_InsideRange);
    RUN_TEST(MaxT_TooShort);
    RUN_TEST(Regression_SignBug);

    std::printf("\n[CharacterModel]\n");
    RUN_TEST(Model_HasParts);
    RUN_TEST(Model_HeadIsHighest);
    RUN_TEST(Model_LegsAreLowest);
    RUN_TEST(Model_AllRadiiPositive);
    RUN_TEST(Model_AllDamageMultPositive);
    RUN_TEST(Model_HasTorso);

    std::printf("\n[Map]\n");
    RUN_TEST(Map_Generates);
    RUN_TEST(Map_HasAtLeastTwoSpawns);
    RUN_TEST(Map_SpawnsOnFloor);
    RUN_TEST(Map_FloodFillConnected);
    RUN_TEST(Map_DeterministicBySeed);

    std::printf("\n[Framebuffer]\n");
    RUN_TEST(FB_Init_NoCrash);
    RUN_TEST(FB_SetPixel_OutOfBounds_NoCrash);
    RUN_TEST(FB_DrawVerticalLine_OutOfBounds);
    RUN_TEST(FB_FillRect_ZeroSize);
    RUN_TEST(FB_BlendAlpha_TransparentIsNoop);

    std::printf("\n[Player]\n");
    RUN_TEST(Player_LerpAngle_ShortestArc_Forward);
    RUN_TEST(Player_LerpAngle_ShortestArc_Back);
    RUN_TEST(Player_LerpAngle_Halfway);
    RUN_TEST(Player_StepPhysics_FixedSpeed);

    std::printf("\n=========================================\n");
    std::printf("  Passed: %d\n", TestFW::Passed());
    std::printf("  Failed: %d\n", TestFW::Failed());
    std::printf("=========================================\n");

    return TestFW::Failed() == 0 ? 0 : 1;
}