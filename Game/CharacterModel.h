#pragma once
#include "Common.h"

// =====================================================================
//  ≈дина€ модель игрока. »спользуетс€ и дл€ попаданий (Hitbox.cpp),
//  и дл€ рендера (Sprite.cpp). ѕомен€ешь здесь Ч помен€етс€ везде.
// =====================================================================

// »дентификаторы дл€ выбора процедурной текстуры.
// (»ндексы должны совпадать с массивом текстур в BodyTextures.cpp.)
enum PartTexId {
    PART_HEAD = 0,
    PART_TORSO,
    PART_LEG,
    PART_ARM,
    PART_SHOULDER,
    PART_TEX_COUNT
};

// “ип примитива Ч определ€ет, как капсула рисуетс€ и во что попадают пули.
enum PartKind {
    PART_SPHERE = 0,   // голова, плечи Ч сфера (zMin == zMax)
    PART_CYL_XZ = 1    // торс, ноги, руки Ч вертикальный цилиндр
};

// ќдна часть тела.
struct BodyPart {
    int      kind;       // PART_SPHERE / PART_CYL_XZ
    double   cx, cy;     // XZ-центр в локальных координатах игрока (м)
    double   r;          // радиус (м)
    double   zMin, zMax; // вертикальный диапазон (м). ƒл€ сферы zMin == zMax == cz
    int      texId;      // кака€ текстура нат€гиваетс€
    double   damageMult; // множитель урона (дл€ будущих хедшотов)
};

// ћаксимум частей тела Ч задаЄт размер локальных буферов в рендере
// и хитбоксе. ƒолжен быть >= реальному числу элементов в PLAYER_PARTS.
#define PLAYER_MAX_PARTS 16

extern const BodyPart PLAYER_PARTS[];
extern const int      PLAYER_PART_COUNT;   // runtime-число, всегда <= PLAYER_MAX_PARTS
extern const double   PLAYER_HEIGHT;