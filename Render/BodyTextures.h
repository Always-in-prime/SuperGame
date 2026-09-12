#pragma once
#include "Common.h"
#include <cstdint>
#include "CharacterModel.h"

// =====================================================================
//  Процедурные текстуры частей тела.
//  Заполняются один раз при первом обращении.
//
//  Индексация: tex[id][i][j]
//    id — PartTexId (PART_HEAD, PART_TORSO, ...)
//    i  — угол (0..TEX_W-1, соответствует 0..2π)
//    j  — высота (0..TEX_H-1, 0 = верх)
//
//  Значения — обычные COLORREF-совместимые uint32_t от RGB().
// =====================================================================

#define BODYTEX_W 64
#define BODYTEX_H 64

extern uint32_t bodyTex[PART_TEX_COUNT][BODYTEX_W][BODYTEX_H];

void InitBodyTextures();

// Удобный сэмпл по (угол, высота). Угол сворачивается в [0, 2π),
// высота — в [0..1] (0 = низ, 1 = верх).
uint32_t SampleBodyTex(int texId, double angle, double vNorm);