#include "Common.h"
#include "Config.h"
#include "Application.h"
#include "Renderer.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

static Application* g_app = nullptr;

// ---------------------------------------------------------------------
//  Windows шлЄт VK_CONTROL/VK_MENU дл€ обоих Ctrl/Alt Ч различаем
//  по extended-биту (bit 24) в lParam. Ёто официальный способ,
//  который используют SDL/GLFW.
// ---------------------------------------------------------------------
static WPARAM NormalizeExtendedVK(WPARAM vk, LPARAM lParam) {
    if (vk == VK_CONTROL)
        return (lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
    if (vk == VK_MENU)
        return (lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
    return vk;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (g_app) App_OnKey(*g_app, (int)NormalizeExtendedVK(wParam, lParam), true);
        return 0;

    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (g_app) App_OnKey(*g_app, (int)NormalizeExtendedVK(wParam, lParam), false);
        return 0;

    case WM_KILLFOCUS:
        if (g_app) Input_Init(g_app->input);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        if (g_app) DrawGame(hdc, g_app->world);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"RaycastWindowClass";
    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);

    RECT wr = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"WinAPI Doom-Quake Split-Screen",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
        NULL, NULL, hInstance, NULL);
    if (hwnd == NULL) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    Application app;
    g_app = &app;
    App_Init(app, hwnd);

    MSG msg = { };
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            App_Frame(app);
        }
    }

    App_Shutdown(app);
    g_app = nullptr;
    return 0;
}