#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace rio {
    using PresentFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT);
    using ResizeBuffersFn = HRESULT(__stdcall*)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);

    extern PresentFn present;
    extern ResizeBuffersFn resize_buffers;

    extern WNDPROC wndProc;
    extern HWND hwnd;
    extern ID3D11Device* device;
    extern ID3D11DeviceContext* device_context;
    extern ID3D11RenderTargetView* render_target_view;
    extern bool showMenu;

    LRESULT CALLBACK hkWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept;
    HRESULT __stdcall hkPresent(IDXGISwapChain* swapChain, UINT SyncInterval, UINT Flags) noexcept;
    HRESULT __stdcall hkResize(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) noexcept;

    void init(IDXGISwapChain* swapchain) noexcept;
    bool setup_overlay() noexcept;
    void shutdown() noexcept;

    void toggle() noexcept;
    bool isvisible() noexcept;
}
