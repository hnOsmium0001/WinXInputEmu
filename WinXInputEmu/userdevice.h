#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "inputdevice.h"
#include "shadowed.h"

struct ButtonInput {
    KeyCode keyCode;
    bool pressed = false;
};

struct JoystickInput {
    // Keep both keyboard and mouse configurations in memory because:
    // 1. both union{} and std::variant are pain in the ass to use
    // 2. allows user to switch betweeen both configs without losing previous values

    struct {
        ButtonInput up, down, left, right;
        float speed = 1.0f;
    } kbd;
    
    struct {  
        // TODO
    } mouse;
    
    // If true, both axis will be generated from mouse movements (specifically the mouse specified by UserDevice.srcMouse)
    bool useMouse = false;
};

struct UserDevice {
    IdevKeyboard srcKbd;
    IdevMouse srcMouse;

    ButtonInput a, b, x, y;
    ButtonInput lb, rb;
    ButtonInput lt, rt;
    ButtonInput start, back;
    ButtonInput dpadUp, dpadDown, dpadLeft, dpadRight;
    ButtonInput lstickBtn, rstickBtn;
    JoystickInput lstick, rstick;

    // This shall be incremented every time any field is updated due to input state changes
    int epoch = 0;

    XINPUT_GAMEPAD ComputeXInputGamepad() const noexcept;
};

extern SRWLOCK gUserDevicesLock;
extern bool gUserDevicesEnabled[XUSER_MAX_COUNT];
extern UserDevice gUserDevices[XUSER_MAX_COUNT];
