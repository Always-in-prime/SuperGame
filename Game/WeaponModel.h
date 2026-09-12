#pragma once
#include "../Core/Common.h"
#include "../Core/Mesh.h"
#include "Player.h"

// ============================================================
//  Модель оружия в системе камеры.
//
//  Использует текстуры из TextureBank:
//    "metal", "metal_dark", "metal_high", "wood", "wood_dark", "gold"
// ============================================================

enum WeaponTexId {
    WT_METAL = 0,
    WT_METAL_DARK,
    WT_METAL_HIGH,
    WT_WOOD,
    WT_WOOD_DARK,
    WT_GOLD,
    WT_COUNT
};

extern int g_weaponTexIds[WT_COUNT];

// Регистрирует текстуры оружия в TextureBank.
void WeaponModel_InitTextures();

// Строит меш оружия в системе камеры.
// recoil — 0..1 (отдача), bobX/bobZ — покачивание.
void BuildWeaponMesh(Mesh& mesh,
    double recoil,
    double bobX, double bobZ);