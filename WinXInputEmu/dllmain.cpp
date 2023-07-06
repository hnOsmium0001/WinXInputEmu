#include "pch.h"

#include "shadowed.h"
#include "userdevice.h"

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
	xinput_dll = LoadLibraryW(L"XInput1_4.dll");
	if (!xinput_dll) {
		// TODO proper logging code
		// Ideally we don't want to touch cstdio and iostream alltogether, to avoid their implicit encoding conversion issues and just use WriteFile() on STD_OUTPUT_HANDLE
		std::cerr << "Error opening XInput1_4.dll, code: " << GetLastError() << '\n';
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

DWORD WINAPI XInputGetAudioDeviceIds(
	_In_ DWORD dwUserIndex,
	_Out_writes_opt_(*pRenderCount) LPWSTR pRenderDeviceId,
	_Inout_opt_ UINT* pRenderCount,
	_Out_writes_opt_(*pCaptureCount) LPWSTR pCaptureDeviceId,
	_Inout_opt_ UINT* pCaptureCount
) WIN_NOEXCEPT {
	if (!gUserDeviceEnabled[dwUserIndex])
		return pfn_XInputGetAudioDeviceIds(dwUserIndex, pRenderDeviceId, pRenderCount, pCaptureDeviceId, pCaptureCount);

	// We pretend that a headset is not connected to this emulated gamepad

	if (pRenderDeviceId) *pRenderDeviceId = '\0';
	if (pRenderCount) *pRenderCount = 0;
	if (pCaptureDeviceId) *pCaptureDeviceId = '\0';
	if (pCaptureCount) *pCaptureCount = 0;

	return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetBatteryInformation(
	_In_ DWORD dwUserIndex,
	_In_ BYTE devType,
	_Out_ XINPUT_BATTERY_INFORMATION* pBatteryInformation
) WIN_NOEXCEPT {
	if (!gUserDeviceEnabled[dwUserIndex])
		return pfn_XInputGetBatteryInformation(dwUserIndex, devType, pBatteryInformation);

	*pBatteryInformation = {};

	switch (devType)
	{
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

DWORD WINAPI XInputGetCapabilities(
	_In_ DWORD dwUserIndex,
	_In_ DWORD dwFlags,
	_Out_ XINPUT_CAPABILITIES* pCapabilities
) WIN_NOEXCEPT {
	if (!gUserDeviceEnabled[dwUserIndex])
		return pfn_XInputGetCapabilities(dwUserIndex, dwFlags, pCapabilities);

	*pCapabilities = {};

	pCapabilities->Type = XINPUT_DEVTYPE_GAMEPAD;
	pCapabilities->SubType = XINPUT_DEVSUBTYPE_GAMEPAD;
	pCapabilities->Flags = 0;
	// TODO is this right? because this would be a functionality overlap with XInputGetState()
	//      not mentioned in the docs: logically, XInputGetCapabilities() would will all buttons and joystick values as 1 if they exist at all, not corresponding to the current input state
	pCapabilities->Gamepad = gUserDevices[dwUserIndex].ComputeXInputState();
	pCapabilities->Vibration = {};

	return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetKeystroke(
	_In_ DWORD dwUserIndex,
	_Reserved_ DWORD dwReserved,
	_Out_ PXINPUT_KEYSTROKE pKeystroke
) WIN_NOEXCEPT {
	if (!gUserDeviceEnabled[dwUserIndex])
		return pfn_XInputGetKeystroke(dwUserIndex, dwReserved, pKeystroke);

	// TODO

	return ERROR_SUCCESS;
}

DWORD WINAPI XInputGetState(
	_In_ DWORD dwUserIndex,
	_Out_ XINPUT_STATE* pState
) WIN_NOEXCEPT {
	if (!gUserDeviceEnabled[dwUserIndex])
		return pfn_XInputGetState(dwUserIndex, pState);

	// TODO

	return ERROR_SUCCESS;
}

DWORD WINAPI XInputSetState(
	_In_ DWORD dwUserIndex,
	_In_ XINPUT_VIBRATION* pVibration
) WIN_NOEXCEPT {
	if (!gUserDeviceEnabled[dwUserIndex])
		return pfn_XInputSetState(dwUserIndex, pVibration);

	// TODO

	return ERROR_SUCCESS;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved) noexcept {
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		InitializeShadowedPfns();
		break;

	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return true;
}
