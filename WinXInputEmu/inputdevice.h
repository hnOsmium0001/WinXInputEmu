#pragma once
#pragma once

#include <optional>
#include <string>
#include <string_view>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

// Win32 Vkey keycode
using KeyCode = BYTE;

void InitKeyCodeConv();
std::string_view KeyCodeToString(KeyCode key);
std::optional<KeyCode> KeyCodeFromString(std::string_view str);

GUID ParseRawInputDeviceGUID(std::wstring_view name);

// For RIM_TYPExxx values
std::wstring_view RawInputTypeToString(DWORD type);

struct IdevDevice {
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    GUID guid = {};
    std::wstring name;
    RID_DEVICE_INFO info;

    static IdevDevice FromHANDLE(HANDLE hDevice);
};

// All pointer params are optional
void PollInputDevices(std::vector<IdevDevice>& out);
