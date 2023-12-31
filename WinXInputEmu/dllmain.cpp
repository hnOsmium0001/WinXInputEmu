#include "pch.h"

#include <format>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "dll.h"
#include "export.h"
#include "inputdevice.h"
#include "inputsrc.h"
#include "shadowed.h"
#include "userdevice.h"
#include "utils.h"

// Declared in dll.h
// Assigned in DllMain() -> DLL_PROCESS_ATTACH
HMODULE gHModule;

// Definitions for stuff in shadowed.h
static HMODULE xinput_dll;
//Pfn_XInputEnable pfn_XInputEnable = nullptr;
Pfn_XInputGetAudioDeviceIds pfn_XInputGetAudioDeviceIds = nullptr;
Pfn_XInputGetBatteryInformation pfn_XInputGetBatteryInformation = nullptr;
Pfn_XInputGetCapabilities pfn_XInputGetCapabilities = nullptr;
//Pfn_XInputGetDSoundAudioDeviceGuids pfn_XInputGetDSoundAudioDeviceGuids = nullptr;
Pfn_XInputGetKeystroke pfn_XInputGetKeystroke = nullptr;
Pfn_XInputGetState pfn_XInputGetState = nullptr;
Pfn_XInputSetState pfn_XInputSetState = nullptr;

static void InitializeShadowedPfns() {
    // Using LoadLibraryExW() with LOAD_LIBRARY_SEARCH_SYSTEM32 only doesn't seem to work -- it still found our XInput1_4.dll (if our name is indeed this)
    // On 32-bit process, "System32" is automatically redirected to SysWOW64
    // On 64-bit process, "System32" will work in place
    xinput_dll = LoadLibraryW(L"C:/Windows/System32/XInput1_4.dll");
    if (!xinput_dll) {
        LOG_DEBUG(L"Error opening XInput1_4.dll: {}", GetLastErrorStr());
        return;
    }

    //pfn_XInputEnable = (Pfn_XInputEnable)GetProcAddress(xinput_dll, "XInputEnable");
    pfn_XInputGetAudioDeviceIds = (Pfn_XInputGetAudioDeviceIds)GetProcAddress(xinput_dll, "XInputGetAudioDeviceIds");
    pfn_XInputGetBatteryInformation = (Pfn_XInputGetBatteryInformation)GetProcAddress(xinput_dll, "XInputGetBatteryInformation");
    pfn_XInputGetCapabilities = (Pfn_XInputGetCapabilities)GetProcAddress(xinput_dll, "XInputGetCapabilities");
    //pfn_XInputGetDSoundAudioDeviceGuids = (Pfn_XInputGetDSoundAudioDeviceGuids)GetProcAddress(xinput_dll, "XInputGetDSoundAudioDeviceGuids");
    pfn_XInputGetKeystroke = (Pfn_XInputGetKeystroke)GetProcAddress(xinput_dll, "XInputGetKeystroke");
    pfn_XInputGetState = (Pfn_XInputGetState)GetProcAddress(xinput_dll, "XInputGetState");
    pfn_XInputSetState = (Pfn_XInputSetState)GetProcAddress(xinput_dll, "XInputSetState");
}

static HANDLE gWorkingThread;
static DWORD gWorkingThreadId;
static DWORD WINAPI WorkingThreadFunction(LPVOID lpParam) {
    RunInputSource();
    return 0;
}

static void StartWorkingThread() {
    gWorkingThread = CreateThread(nullptr, 0, WorkingThreadFunction, nullptr, 0, &gWorkingThreadId);
    if (gWorkingThread == nullptr) {
        LOG_DEBUG(L"Failed to launch working thread");
    }

    // TODO gracefully exit the thread when dll unloads
    //WaitForSingleObject(gWorkingThread, INFINITE);
    //CloseHandle(gWorkingThread);
}

static INIT_ONCE gDllInitGuard = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK DllInitHandler(PINIT_ONCE initOnce, PVOID parameter, PVOID* lpContext) {
    InitializeShadowedPfns();
    StartWorkingThread();
    return TRUE;
}
static void EnsureDllInit() {
    PVOID ctx = nullptr;
    BOOL status = InitOnceExecuteOnce(&gDllInitGuard, DllInitHandler, nullptr, &ctx);
    if (!status) {
        LOG_DEBUG(L"Failed to execute INIT_ONCE");
    }
}

// This function is deprecated, but we still provide it in case the game uses it
XI_API_FUNC void WINAPI XInputEnable(
    BOOL enable
) WIN_NOEXCEPT {
    EnsureDllInit();
}

XI_API_FUNC DWORD WINAPI XInputGetAudioDeviceIds(
    _In_ DWORD dwUserIndex,
    _Out_writes_opt_(*pRenderCount) LPWSTR pRenderDeviceId,
    _Inout_opt_ UINT* pRenderCount,
    _Out_writes_opt_(*pCaptureCount) LPWSTR pCaptureDeviceId,
    _Inout_opt_ UINT* pCaptureCount
) WIN_NOEXCEPT {
    EnsureDllInit();

    SrwSharedLock lock(gXiGamepadsLock);

    //LOG_DEBUG(L"audio device ids {}", dwUserIndex);
    if (!gXiGamepadsEnabled[dwUserIndex])
        return pfn_XInputGetAudioDeviceIds(dwUserIndex, pRenderDeviceId, pRenderCount, pCaptureDeviceId, pCaptureCount);

    // We pretend that a headset is not connected to this emulated gamepad

    if (pRenderDeviceId) *pRenderDeviceId = '\0';
    if (pRenderCount) *pRenderCount = 0;
    if (pCaptureDeviceId) *pCaptureDeviceId = '\0';
    if (pCaptureCount) *pCaptureCount = 0;

    return ERROR_SUCCESS;
}

XI_API_FUNC DWORD WINAPI XInputGetBatteryInformation(
    _In_ DWORD dwUserIndex,
    _In_ BYTE devType,
    _Out_ XINPUT_BATTERY_INFORMATION* pBatteryInformation
) WIN_NOEXCEPT {
    EnsureDllInit();

    SrwSharedLock lock(gXiGamepadsLock);

    //LOG_DEBUG(L"battery info {}", dwUserIndex);
    if (!gXiGamepadsEnabled[dwUserIndex])
        return pfn_XInputGetBatteryInformation(dwUserIndex, devType, pBatteryInformation);

    *pBatteryInformation = {};

    switch (devType) {
    case BATTERY_DEVTYPE_GAMEPAD:
        pBatteryInformation->BatteryType = BATTERY_TYPE_WIRED;
        pBatteryInformation->BatteryLevel = BATTERY_LEVEL_FULL;
        break;
    case BATTERY_DEVTYPE_HEADSET:
        // We pretend that a headset is not connected to this emulated gamepad
        pBatteryInformation->BatteryType = BATTERY_TYPE_DISCONNECTED;
        pBatteryInformation->BatteryType = BATTERY_LEVEL_EMPTY;
        break;
    }

    return ERROR_SUCCESS;
}

XI_API_FUNC DWORD WINAPI XInputGetCapabilities(
    _In_ DWORD dwUserIndex,
    _In_ DWORD dwFlags,
    _Out_ XINPUT_CAPABILITIES* pCapabilities
) WIN_NOEXCEPT {
    EnsureDllInit();

    SrwSharedLock lock(gXiGamepadsLock);

    //LOG_DEBUG(L"caps {}", dwUserIndex);
    if (!gXiGamepadsEnabled[dwUserIndex])
        return pfn_XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);

    *pCapabilities = {};

    pCapabilities->Type = XINPUT_DEVTYPE_GAMEPAD;
    pCapabilities->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
    pCapabilities->Flags = 0;
    pCapabilities->Gamepad = {
        // We have all buttons
        .wButtons = 0xFFFF,
        .bLeftTrigger = 0xFF,
        .bRightTrigger = 0xFF,
        .sThumbLX = 0x7FFF,
        .sThumbLY = 0x7FFF,
        .sThumbRX = 0x7FFF,
        .sThumbRY = 0x7FFF,
    };
    pCapabilities->Vibration = {};

    return ERROR_SUCCESS;
}

XI_API_FUNC DWORD WINAPI XInputGetKeystroke(
    _In_ DWORD dwUserIndex,
    _Reserved_ DWORD dwReserved,
    _Out_ XINPUT_KEYSTROKE* pKeystroke
) WIN_NOEXCEPT {
    EnsureDllInit();

    SrwSharedLock lock(gXiGamepadsLock);

    //LOG_DEBUG(L"keystroke {}", dwUserIndex);
    if (!gXiGamepadsEnabled[dwUserIndex])
        return pfn_XInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);

    // TODO this would require us to maintain a list of input events
    //      I don't think many games actually use this?
    *pKeystroke = {};

    return ERROR_EMPTY;
}

XI_API_FUNC DWORD WINAPI XInputGetState(
    _In_ DWORD dwUserIndex,
    _Out_ XINPUT_STATE* pState
) WIN_NOEXCEPT {
    EnsureDllInit();

    SrwSharedLock lock(gXiGamepadsLock);

    //LOG_DEBUG(L"get state {}", dwUserIndex);
    if (!gXiGamepadsEnabled[dwUserIndex])
        return pfn_XInputGetState(dwUserIndex, pState);

    *pState = {};

    const auto& dev = gXiGamepads[dwUserIndex];
    pState->dwPacketNumber = dev.epoch;
    pState->Gamepad = dev.ComputeXInputGamepad();

    return ERROR_SUCCESS;
}

XI_API_FUNC DWORD WINAPI XInputSetState(
    _In_ DWORD dwUserIndex,
    _In_ XINPUT_VIBRATION* pVibration
) WIN_NOEXCEPT {
    EnsureDllInit();

    SrwSharedLock lock(gXiGamepadsLock);

    //LOG_DEBUG(L"set state {}", dwUserIndex);
    if (!gXiGamepadsEnabled[dwUserIndex])
        return pfn_XInputSetState(dwUserIndex, pVibration);

    // Ignore all vibration states, as we don't really have a way to make keyboards and mouse vibrate :P
    // NOTE: the application shouldn't be calling this function anyways, because we specified in XINPUT_CAPABILITIES.Flags that we don't support vibration
    *pVibration = {};

    return ERROR_SUCCESS;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) noexcept {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        // In win32 (that is, not 16-bit windows) HINSTANCE and HMODULE are the same thing
        gHModule = hModule;

        InitKeyCodeConv();

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
