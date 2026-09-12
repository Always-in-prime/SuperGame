#pragma once
#include "Common.h"
#include <cstdint>

void DrawChar(int x, int y, char c, uint32_t color, int scale = 1);
void DrawText(int x, int y, const char* text, uint32_t color, int scale = 1);
int  TextWidth(const char* text, int scale = 1);