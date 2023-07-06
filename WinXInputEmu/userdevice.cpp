#include "pch.h"

#include "userdevice.h"

static void ComputeXInputStateForJoystick(const JoystickInput& joystick, SHORT& outX, SHORT& outY) {
	constexpr int kStickMaxVal = 32767;
	int stickActualVal = (int)(kStickMaxVal * joystick.kbd.speed);

	if (joystick.useMouse) {
		// TODO
	}
	else {
		outX = (joystick.kbd.left.pressed ? stickActualVal : 0) + (joystick.kbd.right.pressed ? -stickActualVal : 0);
		outY = (joystick.kbd.up.pressed ? stickActualVal : 0) + (joystick.kbd.down.pressed ? -stickActualVal : 0);
	}
}

XINPUT_GAMEPAD UserDevice::ComputeXInputState() const noexcept {
	XINPUT_GAMEPAD res;

	if (a.pressed) res.wButtons &= XINPUT_GAMEPAD_A;
	if (b.pressed) res.wButtons &= XINPUT_GAMEPAD_B;
	if (x.pressed) res.wButtons &= XINPUT_GAMEPAD_X;
	if (y.pressed) res.wButtons &= XINPUT_GAMEPAD_Y;

	if (lb.pressed) res.wButtons &= XINPUT_GAMEPAD_LEFT_SHOULDER;
	if (rb.pressed) res.wButtons &= XINPUT_GAMEPAD_RIGHT_SHOULDER;

	res.bLeftTrigger = lt.pressed ? 255 : 0;
	res.bRightTrigger = rt.pressed ? 255 : 0;

	if (start.pressed) res.wButtons &= XINPUT_GAMEPAD_START;
	if (back.pressed) res.wButtons &= XINPUT_GAMEPAD_BACK;

	if (dpadUp.pressed) res.wButtons &= XINPUT_GAMEPAD_DPAD_UP;
	if (dpadDown.pressed) res.wButtons &= XINPUT_GAMEPAD_DPAD_DOWN;
	if (dpadLeft.pressed) res.wButtons &= XINPUT_GAMEPAD_DPAD_LEFT;
	if (dpadRight.pressed) res.wButtons &= XINPUT_GAMEPAD_DPAD_RIGHT;

	if (lstickBtn.pressed) res.wButtons &= XINPUT_GAMEPAD_LEFT_THUMB;
	if (rstickBtn.pressed) res.wButtons &= XINPUT_GAMEPAD_RIGHT_THUMB;

	ComputeXInputStateForJoystick(lstick, res.sThumbLX, res.sThumbLY);
	ComputeXInputStateForJoystick(rstick, res.sThumbRX, res.sThumbRY);

	return res;
}


bool gUserDeviceEnabled[XUSER_MAX_COUNT] = {};
UserDevice gUserDevices[XUSER_MAX_COUNT] = {};
