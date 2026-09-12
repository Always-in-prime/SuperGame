#include "Common.h"
#include "Config.h"
#include "Player.h"
#include "Renderer.h"
#include "Framebuffer.h"
#include "Texture.h"
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawGame(hdc);
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

    InitGame();
    InitTextures();

    DWORD lastTime = timeGetTime();
    float accumulator = 0.0f;
    MSG msg = { };

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            DWORD now = timeGetTime();
            float frameTime = (now - lastTime) / 1000.0f;
            lastTime = now;
            if (frameTime > MAX_FRAME_TIME) frameTime = MAX_FRAME_TIME;
            accumulator += frameTime;

            int steps = 0;
            while (accumulator >= FIXED_DT && steps < 8) {
                SavePrevStates();
                StepPhysics(FIXED_DT);
                accumulator -= FIXED_DT;
                ++steps;
            }
            if (steps == 8) accumulator = 0.0f;

            float alpha = accumulator / FIXED_DT;
            InterpolateRenderStates(alpha);

            HDC hdc = GetDC(hwnd);
            DrawGame(hdc);
            ReleaseDC(hwnd, hdc);
        }
    }
    return 0;
}