#pragma once
#include "../Core/Common.h"
#include "../Core/Mesh.h"
#include "Player.h"

// ============================================================
//  Модель игрока — набор примитивов, единый для рендера и хитбоксов.
//
//  Индексы текстур берутся из TextureBank по именам:
//    "head", "torso", "leg", "arm", "shoulder"
// ============================================================

// Идентификаторы текстур (индексы для быстрого доступа после InitTextures)
enum PartTexId {
    PART_HEAD = 0,
    PART_TORSO,
    PART_LEG,
    PART_ARM,
    PART_SHOULDER,
    PART_TEX_COUNT
};

// Максимум частей тела.
#define PLAYER_MAX_PARTS 16

// Индексы примитивов в меше игрока (порядок добавления в BuildCharacterMesh).
enum PlayerPartIdx {
    PP_HEAD = 0,
    PP_TORSO,
    PP_SHOULDER_L,
    PP_SHOULDER_R,
    PP_LEG_L,
    PP_LEG_R,
    PP_ARM_L,
    PP_ARM_R,
    PP_COUNT
};

// Регистрирует текстуры игрока в TextureBank.
// Возвращает массив индексов текстур (в порядке PartTexId).
// Вызывается один раз при старте приложения.
void CharacterModel_InitTextures();

// Индексы текстур (заполняются после InitTextures).
extern int g_charTexIds[PART_TEX_COUNT];

// Строит меш игрока в МИРОВЫХ координатах.
// Позиция и угол берутся из p.x, p.y, p.angle.
// walkPhase используется для анимации.
void BuildCharacterMesh(Mesh& mesh, const Player& p);

// Строит меш в ЛОКАЛЬНЫХ координатах (относительно центра игрока в (0,0)).
// Используется для хитбоксов.
void BuildCharacterMeshLocal(Mesh& mesh, double walkPhase);