#include "Settings.h"
#include "Config.h"
#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------
//  Определяем путь рядом с exe.
// ---------------------------------------------------------------------
const char* Settings_GetDefaultPath(char* outBuf, int bufSize) {
    char exePath[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    // Отрезаем имя exe по последнему '\'
    char* slash = strrchr(exePath, '\\');
    if (slash) *(slash + 1) = '\0';
    else       exePath[0] = '\0';

    snprintf(outBuf, bufSize, "%ssettings.ini", exePath);
    return outBuf;
}

// ---------------------------------------------------------------------
//  Хелперы для чтения/записи с дефолтами.
// ---------------------------------------------------------------------
static int ReadInt(const char* path, const char* section,
    const char* key, int def)
{
    return (int)GetPrivateProfileIntA(section, key, def, path);
}

static bool ReadBool(const char* path, const char* section,
    const char* key, bool def)
{
    return GetPrivateProfileIntA(section, key, def ? 1 : 0, path) != 0;
}

static float ReadFloat(const char* path, const char* section,
    const char* key, float def)
{
    char defStr[64];
    snprintf(defStr, sizeof(defStr), "%.6f", def);

    char buf[64] = { 0 };
    GetPrivateProfileStringA(section, key, defStr, buf, sizeof(buf), path);
    return (float)atof(buf);
}

static void WriteInt(const char* path, const char* section,
    const char* key, int v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", v);
    WritePrivateProfileStringA(section, key, buf, path);
}

static void WriteBool(const char* path, const char* section,
    const char* key, bool v)
{
    WritePrivateProfileStringA(section, key, v ? "1" : "0", path);
}

static void WriteFloat(const char* path, const char* section,
    const char* key, float v)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%.6f", v);
    WritePrivateProfileStringA(section, key, buf, path);
}

// ---------------------------------------------------------------------
//  Load / Save
// ---------------------------------------------------------------------
void Settings_Load(GameSettings& s, const char* iniPath) {
    char fullPath[MAX_PATH];
    if (!iniPath || !*iniPath) {
        Settings_GetDefaultPath(fullPath, MAX_PATH);
        iniPath = fullPath;
    }

    // Если файла нет — Load вернёт дефолты, дальше Save не вызываем:
    // пользовательские значения не перезаписываем.

    s.rotSpeed = ReadFloat(iniPath, "Controls", "rotSpeed", s.rotSpeed);
    s.moveSpeed = ReadFloat(iniPath, "Controls", "moveSpeed", s.moveSpeed);

    s.screenShake = ReadBool(iniPath, "Visual", "screenShake", s.screenShake);
    s.headBob = ReadBool(iniPath, "Visual", "headBob", s.headBob);

    s.matchScore = ReadInt(iniPath, "Match", "matchScore", s.matchScore);
    s.showFps = ReadBool(iniPath, "Debug", "showFps", s.showFps);
}

void Settings_Save(const GameSettings& s, const char* iniPath) {
    char fullPath[MAX_PATH];
    if (!iniPath || !*iniPath) {
        Settings_GetDefaultPath(fullPath, MAX_PATH);
        iniPath = fullPath;
    }

    WriteFloat(iniPath, "Controls", "rotSpeed", s.rotSpeed);
    WriteFloat(iniPath, "Controls", "moveSpeed", s.moveSpeed);

    WriteBool(iniPath, "Visual", "screenShake", s.screenShake);
    WriteBool(iniPath, "Visual", "headBob", s.headBob);

    WriteInt(iniPath, "Match", "matchScore", s.matchScore);
    WriteBool(iniPath, "Debug", "showFps", s.showFps);

    // Принудительно сбросить кэш WinAPI — на случай, если файл кэшируется.
    WritePrivateProfileStringA(NULL, NULL, NULL, iniPath);
}