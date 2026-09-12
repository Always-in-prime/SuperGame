#pragma once
#include "Common.h"

extern char map[MAP_HEIGHT][MAP_WIDTH + 1];

void GenerateMap(unsigned int seed = 0);
int  GetSpawnCount();
void GetSpawnPoint(int index, double& outX, double& outY);