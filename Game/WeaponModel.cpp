#include "WeaponModel.h"
#include "../Core/TextureBank.h"
#include <cmath>

int g_weaponTexIds[WT_COUNT] = { -1,-1,-1,-1,-1,-1 };

// ---------------------------------------------------------------------
//  Простые генераторы текстур.
// ---------------------------------------------------------------------
static inline uint32_t C(int r, int g, int b) { return RGB(r, g, b); }

// --- МЕТАЛЛ: серая поверхность с тёмными полосами ---
static uint32_t MetalTex(double u, double v) {
    int n = (int)(u * 100) ^ (int)(v * 100);
    n = (n * 374761393) & 0xFF;
    int base = 82 + (n & 31) - 15;

    // Тёмные линии (швы)
    if (fmod(v * 8.0, 1.0) < 0.08) base -= 30;
    if (fmod(u * 12.0, 1.0) < 0.05) base -= 20;

    if (base < 20) base = 20;
    if (base > 160) base = 160;
    return C(base, base + 4, base + 16);
}

// --- МЕТАЛЛ ТЁМНЫЙ ---
static uint32_t MetalDarkTex(double u, double v) {
    int n = (int)(u * 100) ^ (int)(v * 100);
    n = (n * 374761393) & 0xFF;
    int base = 38 + (n & 15) - 8;

    if (fmod(v * 8.0, 1.0) < 0.08) base -= 12;
    if (base < 12) base = 12;

    return C(base, base + 4, base + 16);
}

// --- МЕТАЛЛ СВЕТЛЫЙ (блики) ---
static uint32_t MetalHighTex(double u, double v) {
    int n = (int)(u * 100) ^ (int)(v * 100);
    n = (n * 374761393) & 0xFF;
    int base = 180 + (n & 35) - 15;
    if (base > 230) base = 230;

    // Полоски блика
    if (fmod(v * 6.0, 1.0) < 0.15) base += 20;
    if (base > 240) base = 240;

    return C(base, base + 6, base + 22);
}

// --- ДЕРЕВО: вертикальные доски ---
static uint32_t WoodTex(double u, double v) {
    int plank = (int)(u * 4);
    int xl = (int)(fmod(u * 4.0, 1.0) * 64);

    if (xl < 2 || xl > 61) return C(34, 20, 10);

    int base = 112 + (plank * 37) % 31;
    int grain = ((int)(v * 100) + plank * 13) % 12;
    if (grain < 2) base += 18;
    if (grain > 9) base -= 12;

    int r = base;
    int g = base * 60 / 112;
    int b = base * 32 / 112;
    return C(r, g, b);
}

// --- ДЕРЕВО ТЁМНОЕ ---
static uint32_t WoodDarkTex(double u, double v) {
    int base = 58 + ((int)(u * 7 + v * 3)) % 12;
    return C(base, base * 32 / 58, base * 14 / 58);
}

// --- ЗОЛОТО ---
static uint32_t GoldTex(double u, double v) {
    int n = (int)(u * 100) ^ (int)(v * 100);
    n = (n * 374761393) & 0xFF;
    int base = 184 + (n & 31) - 15;
    if (base < 140) base = 140;
    if (base > 220) base = 220;
    return C(base, base * 148 / 184, base * 72 / 184);
}

// ---------------------------------------------------------------------
void WeaponModel_InitTextures() {
    g_weaponTexIds[WT_METAL] = TexBank_Register("w_metal", MetalTex);
    g_weaponTexIds[WT_METAL_DARK] = TexBank_Register("w_metal_dark", MetalDarkTex);
    g_weaponTexIds[WT_METAL_HIGH] = TexBank_Register("w_metal_high", MetalHighTex);
    g_weaponTexIds[WT_WOOD] = TexBank_Register("w_wood", WoodTex);
    g_weaponTexIds[WT_WOOD_DARK] = TexBank_Register("w_wood_dark", WoodDarkTex);
    g_weaponTexIds[WT_GOLD] = TexBank_Register("w_gold", GoldTex);
}

// ---------------------------------------------------------------------
//  Хелперы для локальных координат
// ---------------------------------------------------------------------
// Мы работаем в camera-space: X — вправо, Y — вверх, Z — вперёд.
// Старая модель оружия была в системе (X вперёд, Z вверх).
// Переводим:
//   старое (X, Y, Z) -> новое (Y, Z, X)
//   старое cx -> новое cz
//   старое cy (вперёд) -> новое cz... не так.
// Проще: новое (X = +вправо, Y = +вверх, Z = +вперёд).
// Старое оружие: cy был "вдоль ствола" (то есть +Z), cz — вверх (то есть +Y).
// Значит: newX = oldCx, newY = oldCz, newZ = oldCy.

void BuildWeaponMesh(Mesh& mesh, double recoil, double bobX, double bobZ) {
    // Смещение от отдачи
    double offX = bobX + 0.012 * recoil;
    double offY = bobZ + 0.030 * recoil;
    double offZ = -0.035 * recoil;

    Mesh_Init(mesh, 0.0, 0.0, 0.0, 0.0, 1.0);
    mesh.offX = offX;
    mesh.offY = offY;
    mesh.offZ = offZ;

    // --- Приклад (дерево) ---
    // Идёт вдоль Z, смещён вправо и вниз.
    // Старый ADD_CY(0.135, 0.30, 0.55, -0.155) -> новый (X=0.135, Y=-0.155, Z от 0.30 до 0.55)
    Mesh_Add(mesh, MakeCylZ(0.135, -0.155, 0.30, 0.55, 0.042,
        g_weaponTexIds[WT_WOOD], RGB(128, 80, 40)));

    // Тёмная вставка на прикладе
    Mesh_Add(mesh, MakeCylZ(0.135, -0.118, 0.32, 0.52, 0.012,
        g_weaponTexIds[WT_WOOD_DARK], RGB(58, 32, 14)));

    // Затыльник (торцевая пластина)
    Mesh_Add(mesh, MakeCylZ(0.135, -0.155, 0.28, 0.30, 0.048,
        g_weaponTexIds[WT_METAL_DARK], RGB(38, 42, 54)));

    // --- Ствольная коробка ---
    Mesh_Add(mesh, MakeCylZ(0.115, -0.135, 0.55, 0.78, 0.045,
        g_weaponTexIds[WT_METAL_DARK], RGB(38, 42, 54)));

    // Верхняя планка
    Mesh_Add(mesh, MakeCylZ(0.115, -0.092, 0.55, 0.78, 0.010,
        g_weaponTexIds[WT_METAL], RGB(82, 86, 100)));

    // --- Цевьё ---
    Mesh_Add(mesh, MakeCylZ(0.110, -0.125, 0.78, 1.02, 0.038,
        g_weaponTexIds[WT_WOOD], RGB(128, 80, 40)));
    Mesh_Add(mesh, MakeCylZ(0.110, -0.125, 0.78, 0.80, 0.041,
        g_weaponTexIds[WT_WOOD_DARK], RGB(58, 32, 14)));
    Mesh_Add(mesh, MakeCylZ(0.110, -0.125, 1.00, 1.02, 0.041,
        g_weaponTexIds[WT_WOOD_DARK], RGB(58, 32, 14)));

    // --- Ствол ---
    Mesh_Add(mesh, MakeCylZ(0.105, -0.120, 1.02, 1.72, 0.014,
        g_weaponTexIds[WT_METAL], RGB(82, 86, 100)));

    // Блик на стволе (тонкая полоска металла сверху)
    Mesh_Add(mesh, MakeCylZ(0.105, -0.108, 1.02, 1.72, 0.004,
        g_weaponTexIds[WT_METAL_HIGH], RGB(190, 196, 215)));

    // Газовая трубка
    Mesh_Add(mesh, MakeCylZ(0.105, -0.092, 1.02, 1.45, 0.006,
        g_weaponTexIds[WT_METAL_HIGH], RGB(190, 196, 215)));

    // Мушка
    Mesh_Add(mesh, MakeCylZ(0.105, -0.095, 1.66, 1.70, 0.008,
        g_weaponTexIds[WT_METAL_DARK], RGB(38, 42, 54)));

    // Дульный срез
    Mesh_Add(mesh, MakeCylZ(0.105, -0.120, 1.70, 1.72, 0.017,
        g_weaponTexIds[WT_METAL_DARK], RGB(38, 42, 54)));

    // --- Пистолетная рукоятка (вдоль Y, вниз) ---
    Mesh_Add(mesh, MakeCylY(0.115, -0.30, -0.15, 0.62, 0.030,
        g_weaponTexIds[WT_WOOD_DARK], RGB(48, 34, 26)));

    // Пята рукоятки
    Mesh_Add(mesh, MakeCylY(0.115, -0.32, -0.30, 0.62, 0.032,
        g_weaponTexIds[WT_METAL_DARK], RGB(38, 42, 54)));

    // --- Спусковая скоба ---
    Mesh_Add(mesh, MakeCylZ(0.110, -0.210, 0.58, 0.68, 0.005,
        g_weaponTexIds[WT_METAL_DARK], RGB(20, 22, 30)));

    // --- Магазин (вниз от ствольной коробки) ---
    Mesh_Add(mesh, MakeCylY(0.115, -0.26, -0.16, 0.66, 0.022,
        g_weaponTexIds[WT_METAL_DARK], RGB(38, 42, 54)));

    // Золотая окантовка магазина
    Mesh_Add(mesh, MakeCylY(0.115, -0.26, -0.24, 0.66, 0.024,
        g_weaponTexIds[WT_GOLD], RGB(184, 148, 72)));
}