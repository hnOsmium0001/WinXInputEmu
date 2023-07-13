#include "pch.h"

#include "inputsrc.h"

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <utility>
#include <vector>

#include <hidusage.h>

#include "inputdevice.h"

void InputTranslationStruct::Clear() {
    for (int i = 0; i < 0xFF; ++i) {
        for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
            btns[userIndex][i] = XiButton::None;
        }
    }
}

void InputTranslationStruct::PopulateFromConfig(const Config& config) {
    Clear();

    for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
        const auto& profileName = config.xiGamepadBindings[userIndex];
        if (profileName.empty()) continue;
        const auto& profile = *config.profiles.at(profileName);

        using enum XiButton;
#define BTN(KEY_ENUM, THE_BTN) if (THE_BTN.keyCode != 0xFF) btns[userIndex][THE_BTN.keyCode] = KEY_ENUM;
        BTN(A, profile.a);
        BTN(B, profile.b);
        BTN(X, profile.x);
        BTN(Y, profile.y);
        BTN(LB, profile.lb);
        BTN(RB, profile.rb);
        BTN(LT, profile.lt);
        BTN(RT, profile.rt);
        BTN(Start, profile.start);
        BTN(Back, profile.back);
        BTN(DpadUp, profile.dpadUp);
        BTN(DpadDown, profile.dpadDown);
        BTN(DpadLeft, profile.dpadLeft);
        BTN(DpadRight, profile.dpadRight);
        BTN(LStickBtn, profile.lstickBtn);
        BTN(RStickBtn, profile.rstickBtn);
#define STICK(PREFIX, THE_STICK) \
    if (THE_STICK.useMouse) {} \
    else { BTN(PREFIX##StickUp, THE_STICK.kbd.up); BTN(PREFIX##StickDown, THE_STICK.kbd.down); BTN(PREFIX##StickLeft, THE_STICK.kbd.left); BTN(PREFIX##StickRight, THE_STICK.kbd.right); }
        STICK(L, profile.lstick);
        STICK(R, profile.rstick);
#undef STICK
#undef BTN
    }
}

struct IsrcSw_State {
    std::vector<IdevDevice> devices;

    Config config = {};
    InputTranslationStruct its = {};

    // For a RAWINPUT*
    std::unique_ptr<std::byte[]> rawinput;
    size_t rawinputSize = 0;
};

static void HandleKeyPress(BYTE vkey, bool pressed, InputTranslationStruct& its, const Config& config) {
    for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
        if (!gXiGamepadsEnabled[userIndex]) continue;

        auto& dev = gXiGamepads[userIndex];
        auto& extra = its.xiGamepadExtraInfo[userIndex];

        bool recompute_lstick = false;
        bool recompute_rstick = false;

        switch (its.btns[userIndex][vkey]) {
            using enum XiButton;
        case A: dev.a = pressed; break;
        case B: dev.b = pressed; break;
        case X: dev.x = pressed; break;
        case Y: dev.y = pressed; break;

        case LB: dev.lb = pressed; break;
        case RB: dev.rb = pressed; break;
        case LT: dev.lt = pressed; break;
        case RT: dev.rt = pressed; break;

        case Start: dev.start = pressed; break;
        case Back: dev.back = pressed; break;

        case DpadUp: dev.dpadUp = pressed; break;
        case DpadDown: dev.dpadDown = pressed; break;
        case DpadLeft: dev.dpadLeft = pressed; break;
        case DpadRight: dev.dpadRight = pressed; break;

        case LStickBtn: dev.lstickBtn = pressed; break;
        case RStickBtn: dev.rstickBtn = pressed; break;

            // NOTE: we assume that if any key is setup for the joystick directions, it's on keyboard mode
            //       that is, we rely on the translation struct being populated from the current user config correctly
#define STICKBUTTON(THEENUM, STICK, DIR) case THEENUM: recompute_##STICK = true; extra.STICK.DIR = pressed; break;
            STICKBUTTON(LStickUp, lstick, up);
            STICKBUTTON(LStickDown, lstick, down);
            STICKBUTTON(LStickLeft, lstick, left);
            STICKBUTTON(LStickRight, lstick, right);
            STICKBUTTON(RStickUp, rstick, up);
            STICKBUTTON(RStickDown, rstick, down);
            STICKBUTTON(RStickLeft, rstick, left);
            STICKBUTTON(RStickRight, rstick, right);
#undef STICKBUTTON

        case None: break;
        }

        constexpr int kStickMaxVal = 32767;
        if (recompute_lstick) {
            // Stick's actual value per user's speed setting
            int val = (int)(kStickMaxVal * dev.profile->lstick.kbd.speed);
            dev.lstickX = (extra.lstick.right ? val : 0) + (extra.lstick.left ? -val : 0);
            dev.lstickY = (extra.lstick.up ? val : 0) + (extra.lstick.down ? -val : 0);
        }
        if (recompute_rstick) {
            int val = (int)(kStickMaxVal * dev.profile->rstick.kbd.speed);
            dev.rstickX = (extra.rstick.right ? val : 0) + (extra.rstick.left ? -val : 0);
            dev.rstickY = (extra.rstick.up ? val : 0) + (extra.rstick.down ? -val : 0);
        }

        ++dev.epoch;
    }
}

LRESULT CALLBACK IsrcSw_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
    auto& s = *(IsrcSw_State*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_INPUT: {
        HRAWINPUT hri = (HRAWINPUT)lParam;

        UINT size = 0;
        GetRawInputData(hri, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
        if (size > s.rawinputSize || s.rawinput == nullptr) {
            s.rawinput = std::make_unique<std::byte[]>(size);
            s.rawinputSize = size;
        }

        if (GetRawInputData(hri, RID_INPUT, s.rawinput.get(), &size, sizeof(RAWINPUTHEADER)) == (UINT)-1) {
            LOG_DEBUG(L"GetRawInputData() failed");
            break;
        }
        RAWINPUT* ri = (RAWINPUT*)s.rawinput.get();

        // We are going to modify/push data onto XiGamepad's below, from this input event
        SrwExclusiveLock lock(gXiGamepadsLock);

        switch (ri->header.dwType) {
        case RIM_TYPEMOUSE: {
            const auto& mouse = ri->data.mouse;
            // TODO
        } break;

        case RIM_TYPEKEYBOARD: {
            const auto& kbd = ri->data.keyboard;

            // This message is a part of a longer makecode sequence -- the actual Vkey is in another one
            if (kbd.VKey == 0xFF)
                break;

            bool press = !(kbd.Flags & RI_KEY_BREAK);

            assert(kbd.VKey <= 0xFF);
            HandleKeyPress((BYTE)kbd.VKey, press, s.its, s.config);
        } break;
        }

        return 0;
    }

    case WM_INPUT_DEVICE_CHANGE: {
        HANDLE hDevice = (HANDLE)lParam;

        if (wParam == GIDC_ARRIVAL) {
            // NOTE: WM_INPUT_DEVICE_CHANGE seem to fire when RIDEV_DEVNOTIFY is first set on a window, so we get duplicate devices here as the ones collected in _glfwPollKeyboardsWin32()
            // Filter duplicate devices
            for (const auto& idev : s.devices) {
                if (idev.hDevice == hDevice)
                    return 0;
            }

            s.devices.push_back(IdevDevice::FromHANDLE(hDevice));

            const auto& idev = s.devices.back();
            LOG_DEBUG("Connected {} {}", RawInputTypeToString(idev.info.dwType), idev.name);
        }
        else if (wParam == GIDC_REMOVAL) {
            // HACK: this relies on std::erase_if only visiting each element once (which is almost necessarily the case) but still technically not standard-compliant
            //       for the behavior of "log the device being removed once"
            std::erase_if(
                s.devices,
                [&](const IdevDevice& idev) {
                    if (idev.hDevice == hDevice) {
                        LOG_DEBUG("Disconnected {} {}", RawInputTypeToString(idev.info.dwType), idev.name);
                        return true;
                    }
                    else {
                        return false;
                    }
                });
        }

        return 0;
    }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void InputSource_RunSeparateWindow(HINSTANCE hinst, Config config) {
    LOG_DEBUG(L"Starting input source window");

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = IsrcSw_WindowProc;
    wc.hInstance = hinst;
    wc.lpszClassName = L"WinXInputEmu";
    ATOM atom = RegisterClassExW(&wc);
    if (!atom) {
        LOG_DEBUG(L"Error creating Input Source window class: {}", GetLastErrorStr());
        return;
    }

    HWND hwnd = CreateWindowExW(
        0,
        MAKEINTATOM(atom),
        L"WinXInputEmu Input Source",
        WS_OVERLAPPEDWINDOW,

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

        NULL,  // Parent window    
        NULL,  // Menu
        hinst, // Instance handle
        NULL   // Additional application data
    );
    if (hwnd == nullptr) {
        LOG_DEBUG(L"Error creating Input Source window: {}", GetLastErrorStr());
        return;
    }

    IsrcSw_State wndState{
        .config = std::move(config),
    };
    wndState.its.PopulateFromConfig(wndState.config);
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)&wndState);

    ShowWindow(hwnd, SW_SHOW);

    RAWINPUTDEVICE rid[2];

    // We don't use RIDEV_NOLEGACY because all the window manipulation (e.g. dragging the title bar) relies on the "legacy messages"
    // RIDEV_INPUTSINK so that we get input even if the game window is current in focus instead
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid[0].hwndTarget = hwnd;

    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    rid[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[1].hwndTarget = hwnd;

    if (RegisterRawInputDevices(rid, std::size(rid), sizeof(RAWINPUTDEVICE)) == false) {
        return;
    }

    LOG_DEBUG(L"Starting working thread's message pump");

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
