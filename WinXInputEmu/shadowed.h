#pragma once

#include "Xinput.h"

// A list of functions present in XInput
// Sourced from https://learn.microsoft.com/en-us/windows/win32/api/_xinput/

// The strategy is to implement and thus shadowing the symbol for all of them
// When called, detect if `dwUserIndex` (the gamepad ID) matches the one we're trying to override
// if so, return the values generated from our frontend
// if not, simply redirect to the actual XInput API

// Note that XInput has a variety of different versions: https://learn.microsoft.com/en-us/windows/win32/xinput/xinput-versions and the relevant functions differ. 
// We are targetting xinput1_4.dll as you would see on a typical Win10 installation

// Deprecated in Win10
//using Pfn_XInputEnable = std::add_pointer_t<decltype(XInputEnable)>;
//extern Pfn_XInputEnable pfn_XInputEnable;

using Pfn_XInputGetAudioDeviceIds = std::add_pointer_t<decltype(XInputGetAudioDeviceIds)>;
extern Pfn_XInputGetAudioDeviceIds pfn_XInputGetAudioDeviceIds;

using Pfn_XInputGetBatteryInformation = std::add_pointer_t<decltype(XInputGetBatteryInformation)>;
extern Pfn_XInputGetBatteryInformation pfn_XInputGetBatteryInformation;

using Pfn_XInputGetCapabilities = std::add_pointer_t<decltype(XInputGetCapabilities)>;
extern Pfn_XInputGetCapabilities pfn_XInputGetCapabilities;

// Deprecated since Win8
// NOTE(hnOsmium0001): the definition is gone alltogether from Win10, on my dev machine
//using Pfn_XInputGetDSoundAudioDeviceGuids = std::add_pointer_t<decltype(XInputGetDSoundAudioDeviceGuids)>;
//extern Pfn_XInputGetDSoundAudioDeviceGuids pfn_XInputGetDSoundAudioDeviceGuids;

using Pfn_XInputGetKeystroke = std::add_pointer_t<decltype(XInputGetKeystroke)>;
extern Pfn_XInputGetKeystroke pfn_XInputGetKeystroke;

using Pfn_XInputGetState = std::add_pointer_t<decltype(XInputGetState)>;
extern Pfn_XInputGetState pfn_XInputGetState;

using Pfn_XInputSetState = std::add_pointer_t<decltype(XInputSetState)>;
extern Pfn_XInputSetState pfn_XInputSetState;
