#pragma once

#include "config.h"
#include "userdevice.h"

enum class XiButton : unsigned char {
    A, B, X, Y,
    LB, RB,
    LT, RT,
    Start, Back,
    DpadUp, DpadDown, DpadLeft, DpadRight,
    LStickBtn, RStickBtn,
    LStickUp, LStickDown, LStickLeft, LStickRight,
    RStickUp, RStickDown, RStickLeft, RStickRight,
    None,
};

// Information and lookup tables computable from a Config object
// used for translating input key presses/mouse movements into gamepad state
struct InputTranslationStruct {
    struct {
        struct {
            // Keyboard mode stuff
            bool up, down, left, right;

            // Mouse mode stuff
        } lstick, rstick;
    } xiGamepadExtraInfo[XUSER_MAX_COUNT];

    // VK_xxx is BYTE, max 255 values
    // NOTE: XiButton::COUNT is used to indicate "this mapping is not bound"
    XiButton btns[XUSER_MAX_COUNT][0xFF];

    void Clear();
    void PopulateFromConfig(const Config& config);
};

void InputSource_RunSeparateWindow(HINSTANCE hinst, Config config);
