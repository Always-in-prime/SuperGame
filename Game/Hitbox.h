#pragma once
#include "Common.h"
#include "CharacterModel.h"

// =====================================================================
//  ’итбоксы используют ту же модель, что и рендер (PLAYER_PARTS).
//  ѕопадание = луч пересекает хоть одну капсулу/сферу игрока.
//
//  ¬озвращаем не только hit, но и:
//    - distance Ч на каком рассто€нии произошло попадание (в метрах)
//    - partIdx  Ч индекс части в PLAYER_PARTS (дл€ множител€ урона)
// =====================================================================

struct HitInfo {
    bool   hit;
    double distance;   // t вдоль луча
    int    partIdx;    // индекс в PLAYER_PARTS (дл€ хедшота и пр.)
};

// Ћуч (ox,oy) + t*(dx,dy)  против модели игрока:
//   position: (px, py)   Ч мирова€ позици€ центра игрока
//   angle:    a          Ч поворот игрока в мире (рад)
//   maxT > 0 Ч считать попаданием только то, что ближе maxT
//
// ¬нутри переводит луч в локальную систему игрока и провер€ет
// пересечение со всеми част€ми PLAYER_PARTS.
HitInfo RaycastCharacter(double ox, double oy,
    double dx, double dy,
    double px, double py, double a,
    double maxT = 0.0);