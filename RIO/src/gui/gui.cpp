#include "gui.h"
#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace rio {
    PresentFn present = nullptr;
    ResizeBuffersFn resize_buffers = nullptr;

    WNDPROC wndProc = nullptr;
    HWND hwnd = nullptr;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* device_context = nullptr;
    ID3D11RenderTargetView* render_target_view = nullptr;
    bool showMenu = true;

    void toggle() noexcept
    {
        showMenu = !showMenu;
    }

    bool isvisible() noexcept
    {
        return showMenu;
    }

    LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept {
        if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
            return true;
        return CallWindowProc(wndProc, hwnd, msg, wParam, lParam);
    }

    void init(IDXGISwapChain* swapchain) noexcept {
        DXGI_SWAP_CHAIN_DESC sd;
        swapchain->GetDesc(&sd);
        hwnd = sd.OutputWindow;

        if (!device) {
            swapchain->GetDevice(__uuidof(ID3D11Device), (void**)&device);
            device->GetImmediateContext(&device_context);

            ID3D11Texture2D* backBuffer = nullptr;
            swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
            device->CreateRenderTargetView(backBuffer, NULL, &render_target_view);
            backBuffer->Release();

            ImGui::CreateContext();
            ImGui_ImplWin32_Init(hwnd);
            wndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
            ImGui_ImplDX11_Init(device, device_context);
        }
    }

    HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags) noexcept {
        static bool initialized = false;

        if (!initialized) {
            init(swapChain);
            initialized = true;
        }

        if (GetAsyncKeyState(VK_INSERT) & 1) {
            toggle();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (showMenu) {
            ImGui::Begin("Hello from RIO!");
            ImGui::End();
        }

        ImGui::Render();
        device_context->OMSetRenderTargets(1, &render_target_view, NULL);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        return present(swapChain, SyncInterval, Flags);
    }

    HRESULT __stdcall hkResize(IDXGISwapChain* swapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) noexcept {
        if (render_target_view) {
            render_target_view->Release();
            render_target_view = nullptr;
        }

        HRESULT hr = resize_buffers(swapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

        if (SUCCEEDED(hr)) {
            ID3D11Texture2D* pBackBuffer = nullptr;
            swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
            if (pBackBuffer) {
                device->CreateRenderTargetView(pBackBuffer, NULL, &render_target_view);
                pBackBuffer->Release();
            }
        }

        return hr;
    }

    bool setup_overlay() noexcept {
        if (MH_Initialize() != MH_OK)
            return false;

        WNDCLASSEXA wc = {
            sizeof(WNDCLASSEXA), CS_CLASSDC, DefWindowProcA, 0L, 0L,
            GetModuleHandleA(0), 0, 0, 0, 0, "Dummy", 0
        };

        RegisterClassExA(&wc);
        HWND Hwnd = CreateWindowA("Dummy", 0, WS_OVERLAPPEDWINDOW, 100, 100, 300, 300, 0, 0, wc.hInstance, 0);

        D3D_FEATURE_LEVEL featureLevel;
        const D3D_FEATURE_LEVEL flvl[] = { D3D_FEATURE_LEVEL_11_0 };

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = Hwnd;
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        IDXGISwapChain* swapChain;
        ID3D11Device* Device;
        ID3D11DeviceContext* Context;

        if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
            flvl, 1, D3D11_SDK_VERSION, &sd, &swapChain, &Device, &featureLevel, &Context) != S_OK)
            return false;

        void** vtable = *(void***)(swapChain);
        void* present_target = vtable[8];
        void* resize_target = vtable[13];

        if (MH_CreateHook(present_target, &hkPresent, reinterpret_cast<void**>(&present)) != MH_OK)
            return false;

        if (MH_EnableHook(present_target) != MH_OK)
            return false;

        if (MH_CreateHook(resize_target, &hkResize, reinterpret_cast<void**>(&resize_buffers)) == MH_OK)
            MH_EnableHook(resize_target);

        swapChain->Release();
        Device->Release();
        Context->Release();
        DestroyWindow(Hwnd);
        UnregisterClassA("Dummy", wc.hInstance);

        return true;
    }

    void shutdown() noexcept {
        if (hwnd && wndProc)
            SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)wndProc);

        MH_Uninitialize();
    }
}
