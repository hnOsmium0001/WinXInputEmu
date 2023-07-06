#pragma once

#include <Xinput.h>

#include "inputdevice.h"

using KeyCode = BYTE;

struct ButtonInput {
	KeyCode keyCode;
	bool pressed = false;
};

struct JoystickInput {
	union {
		struct {
			ButtonInput up, down, left, right;
			float speed = 1.0f;
		} kbd;
		struct {
			// TODO
		} mouse;
	};
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

	XINPUT_GAMEPAD ComputeXInputState() const noexcept;
};


extern bool gUserDeviceEnabled[XUSER_MAX_COUNT];
extern UserDevice gUserDevices[XUSER_MAX_COUNT];
