#include <Windows.h>
#include "gui/gui.h"
#include <thread>

int __stdcall Hook() {
    if (rio::setup_overlay()) {
        MessageBoxA(0, "Overlay Initialized", "Success", MB_OK);
    }
    else {
        goto UNLOAD;
    }
    return 0;

UNLOAD:
    rio::shutdown();
}

int __stdcall DllMain(const HMODULE instance, const uintptr_t reason, const void* reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        const auto thread = CreateThread(
            nullptr,
            0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(Hook),
            nullptr,
            0,
            nullptr
        );

        if (thread)
            CloseHandle(thread);
    }

    return TRUE;
}
