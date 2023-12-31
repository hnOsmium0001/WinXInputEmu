#pragma once

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "config.h"
#include "inputdevice.h"
#include "shadowed.h"

// The prefix Xi stands for XInput
// We try to avoid using "XInput" or "XINPUT" in any names that is unrelated from the actual XInput API, to avoid confusion
struct XiGamepad {
    const UserProfile* profile = nullptr;

    // If == INVALID_HANDLE_VALUE, accept any input source
    // Otherwise accept only the specified input source
    HANDLE srcKbd = INVALID_HANDLE_VALUE;
    HANDLE srcMouse = INVALID_HANDLE_VALUE;

    // This shall be incremented every time any field is updated due to input state changes
    int epoch = 0;

    short lstickX, lstickY;
    short rstickX, rstickY;
    bool a, b, x, y;
    bool lb, rb;
    bool lt, rt;
    bool start, back;
    bool dpadUp, dpadDown, dpadLeft, dpadRight;
    bool lstickBtn, rstickBtn;

    XINPUT_GAMEPAD ComputeXInputGamepad() const noexcept;
};

extern SRWLOCK gXiGamepadsLock;
extern bool gXiGamepadsEnabled[XUSER_MAX_COUNT];
extern XiGamepad gXiGamepads[XUSER_MAX_COUNT];
