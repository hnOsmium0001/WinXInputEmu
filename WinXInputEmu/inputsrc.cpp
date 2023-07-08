#include "pch.h"

#include <cstddef>
#include <format>
#include <memory>
#include <utility>

#include <hidusage.h>

#include "inputsrc.h"

struct IsrcSw_State {
    // For a RAWINPUT*
    std::unique_ptr<std::byte[]> rawinput;
    size_t rawinputSize = 0;
};

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

        switch (ri->header.dwType) {
        case RIM_TYPEKEYBOARD: {
            const auto& mouse = ri->data.mouse;
            // TODO
        } break;

        case RIM_TYPEMOUSE: {
            const auto& kbd = ri->data.keyboard;
        } break;
        }

        return 0;
    }

    case WM_INPUT_DEVICE_CHANGE: {
        // TODO
        return 0;
    }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void InputSource_RunSeparateWindow(HINSTANCE hinst) {
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

    IsrcSw_State wndState;
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)&wndState);

    ShowWindow(hwnd, SW_SHOW);

    RAWINPUTDEVICE rid[2];

    // We don't use RIDEV_NOLEGACY because all the window manipulation (e.g. dragging the title bar) relies on the "legacy messages"
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].dwFlags = 0;
    rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid[0].hwndTarget = hwnd;

    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].dwFlags = 0;
    rid[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[1].hwndTarget = hwnd;

    if (RegisterRawInputDevices(rid, std::size(rid), sizeof(RAWINPUTDEVICE)) == false) {
        return;
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}
