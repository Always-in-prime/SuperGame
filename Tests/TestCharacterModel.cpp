#include "TestFramework.h"
#include "CharacterModel.h"

TEST_CASE(Model_HasParts) {
    CHECK_TRUE(PLAYER_PART_COUNT > 0);
    CHECK_TRUE(PLAYER_PART_COUNT <= PLAYER_MAX_PARTS);
}

TEST_CASE(Model_HeadIsHighest) {
    double maxZ = -1e9;
    int    headIdx = -1;
    for (int i = 0; i < PLAYER_PART_COUNT; ++i) {
        if (PLAYER_PARTS[i].zMax > maxZ) {
            maxZ = PLAYER_PARTS[i].zMax;
            headIdx = i;
        }
    }
    CHECK_EQ(PLAYER_PARTS[headIdx].texId, PART_HEAD);
}

TEST_CASE(Model_LegsAreLowest) {
    bool legsAtGround = false;
    for (int i = 0; i < PLAYER_PART_COUNT; ++i) {
        if (PLAYER_PARTS[i].texId == PART_LEG && PLAYER_PARTS[i].zMin <= 0.001) {
            legsAtGround = true;
            break;
        }
    }
    CHECK_TRUE(legsAtGround);
}

TEST_CASE(Model_AllRadiiPositive) {
    for (int i = 0; i < PLAYER_PART_COUNT; ++i) {
        CHECK_TRUE(PLAYER_PARTS[i].r > 0.0);
        CHECK_TRUE(PLAYER_PARTS[i].zMax >= PLAYER_PARTS[i].zMin);
    }
}

TEST_CASE(Model_AllDamageMultPositive) {
    for (int i = 0; i < PLAYER_PART_COUNT; ++i) {
        CHECK_TRUE(PLAYER_PARTS[i].damageMult > 0.0);
    }
}

TEST_CASE(Model_HasTorso) {
    bool found = false;
    for (int i = 0; i < PLAYER_PART_COUNT; ++i) {
        if (PLAYER_PARTS[i].texId == PART_TORSO) { found = true; break; }
    }
    CHECK_TRUE(found);
}