#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "inputdevice.h"
#include "shadowed.h"

// The prefix Xi stands for XInput
// We try to avoid using "XInput" or "XINPUT" in any names that is unrelated from the actual XInput API, to avoid confusion
struct XiDevice {
    IdevDevice srcKbd;
    IdevDevice srcMouse;

    struct {
        short lstickX, lstickY;
        short rstickX, rstickY;
        bool a, b, x, y;
        bool lb, rb;
        bool lt, rt;
        bool start, back;
        bool dpadUp, dpadDown, dpadLeft, dpadRight;
        bool lstickBtn, rstickBtn;
    } state;

    // This shall be incremented every time any field is updated due to input state changes
    int epoch = 0;

    XINPUT_GAMEPAD ComputeXInputGamepad() const noexcept;
};

extern SRWLOCK gXidevLock;
extern bool gXidevEnabled[XUSER_MAX_COUNT];
extern XiDevice gXidev[XUSER_MAX_COUNT];
