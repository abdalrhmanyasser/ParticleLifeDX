#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include "imgui/imgui.h"
#include "Renderer.h"

// NEW: Forward declare the ImGui Win32 handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Renderer *g_renderer = nullptr;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // NEW: Let ImGui handle input first
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_SPACE && g_renderer != nullptr)
        {
            uint32_t randomTypes = 3 + (rand() % 13);
            g_renderer->RandomizeRules(randomTypes);
        }
        return 0;

    case WM_SIZE:
        if (g_renderer != nullptr)
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            g_renderer->Resize(width, height);
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    // Seed the basic random generator for initial particle placement
    srand((unsigned int)time(NULL));

    const wchar_t CLASS_NAME[] = L"ParticleLifeClass";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"Particle Life (Press SPACE to Mutate!)",
                               WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                               nullptr, nullptr, hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);

    Renderer renderer(hwnd);
    g_renderer = &renderer; // Assign the global pointer

    renderer.InitParticles(15000); // Only pass particle count now

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            renderer.UpdateAndRender();
        }
    }

    g_renderer = nullptr;
    return 0;
}