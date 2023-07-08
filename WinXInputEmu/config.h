#pragma once

#include <string>
#include <string_view>

#include "shadowed.h"
#include "inputdevice.h"

struct UserProfile {
    struct Button {
        KeyCode keyCode;
    };

    struct Joystick {
        // Keep both keyboard and mouse configurations in memory because:
        // 1. both union{} and std::variant are pain in the ass to use
        // 2. allows user to switch betweeen both configs without losing previous values

        struct {
            Button up, down, left, right;
            // Range: [0,1] i.e. works as a percentage
            float speed = 1.0f;
        } kbd;

        struct {
            // 
            // Lower value corresponds to higher sensitivity
            float sensitivity = 15.0f;
            // 1.0 is linear
            // < 1 makes center more sensitive
            float nonLinear = 1.0f;
            // Range: [0,1]
            float deadzone = 0.0f;
            // Recommends 50-10
            int mouseCheckFrequency = 75;
            bool invertXAxis = false;
            bool invertYAxis = false;
        } mouse;

        // If true, both axis will be generated from mouse movements (specifically the mouse specified by XiDevice.srcMouse)
        bool useMouse = false;
    };

    Button a, b, x, y;
    Button lb, rb;
    Button lt, rt;
    Button start, back;
    Button dpadUp, dpadDown, dpadLeft, dpadRight;
    Button lstickBtn, rstickBtn;
    Joystick lstick, rstick;
};

struct Config {
    UserProfile profiles[XUSER_MAX_COUNT] = {};

    std::string Serialize() const noexcept;
    void Deserialize(std::string_view str) noexcept;
};
