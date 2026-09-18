#pragma once
#include "Common.h"
#include "Config.h"
#include "Player.h"
#include "Match.h"
#include "PlayerInput.h"
#include "Settings.h"

// ============================================================
//  World — всё состояние игры в одном месте.
//  Не знает про рендер. Application владеет одним World.
// ============================================================

struct World {
    Player       players[2];
    Match        match;
    GameSettings settings;
};

// ----- Lifecycle -----
void World_Init(World& w, const GameSettings& s);
void World_Regenerate(World& w);

// ----- Fixed timestep -----
void World_SavePrev(World& w);
void World_Step(World& w, float dt, const PlayerInput inputs[2]);
void World_Interpolate(World& w, float alpha);