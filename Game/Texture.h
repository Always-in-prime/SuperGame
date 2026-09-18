#pragma once
#include "../Core/Common.h"
#include <cstdint>

#define TEX_SIZE 64
#define NUM_WALL_TEXTURES 4

extern uint32_t wallTex[NUM_WALL_TEXTURES][TEX_SIZE][TEX_SIZE];

void InitTextures();
int  GetTileTexture(int cx, int cy);