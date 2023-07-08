#include "pch.h"

#include "userdevice.h"

XINPUT_GAMEPAD XiDevice::ComputeXInputGamepad() const noexcept {
    XINPUT_GAMEPAD res;
    const auto& s = this->state;

    if (s.a) res.wButtons &= XINPUT_GAMEPAD_A;
    if (s.b) res.wButtons &= XINPUT_GAMEPAD_B;
    if (s.x) res.wButtons &= XINPUT_GAMEPAD_X;
    if (s.y) res.wButtons &= XINPUT_GAMEPAD_Y;

    if (s.lb) res.wButtons &= XINPUT_GAMEPAD_LEFT_SHOULDER;
    if (s.rb) res.wButtons &= XINPUT_GAMEPAD_RIGHT_SHOULDER;

    res.bLeftTrigger = s.lt ? 255 : 0;
    res.bRightTrigger = s.rt ? 255 : 0;

    if (s.start) res.wButtons &= XINPUT_GAMEPAD_START;
    if (s.back) res.wButtons &= XINPUT_GAMEPAD_BACK;

    if (s.dpadUp) res.wButtons &= XINPUT_GAMEPAD_DPAD_UP;
    if (s.dpadDown) res.wButtons &= XINPUT_GAMEPAD_DPAD_DOWN;
    if (s.dpadLeft) res.wButtons &= XINPUT_GAMEPAD_DPAD_LEFT;
    if (s.dpadRight) res.wButtons &= XINPUT_GAMEPAD_DPAD_RIGHT;

    if (s.lstickBtn) res.wButtons &= XINPUT_GAMEPAD_LEFT_THUMB;
    if (s.rstickBtn) res.wButtons &= XINPUT_GAMEPAD_RIGHT_THUMB;

    res.sThumbLX = s.lstickX;
    res.sThumbLY = s.lstickY;
    res.sThumbRX = s.rstickX;
    res.sThumbRY = s.rstickY;

    return res;
}


SRWLOCK gXidevLock = SRWLOCK_INIT;
bool gXidevEnabled[XUSER_MAX_COUNT] = {};
XiDevice gXidev[XUSER_MAX_COUNT] = {};
