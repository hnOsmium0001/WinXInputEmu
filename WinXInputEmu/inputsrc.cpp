#include "pch.h"

#include "inputsrc.h"

#include <algorithm>
#include <cstddef>
#include <d3d11.h>
#include <format>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <memory>
#include <utility>
#include <vector>

#include <hidusage.h>

#include "inputdevice.h"
#include "ui.h"

enum class XiButton : unsigned char {
    None = 0,
    A, B, X, Y,
    LB, RB,
    LT, RT,
    Start, Back,
    DpadUp, DpadDown, DpadLeft, DpadRight,
    LStickBtn, RStickBtn,
    LStickUp, LStickDown, LStickLeft, LStickRight,
    RStickUp, RStickDown, RStickLeft, RStickRight,
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
    void PopulateBtnLut(int userIndex, const UserProfile& profile);
    void PopulateFromConfig(const Config& config);
};

void InputTranslationStruct::Clear() {
    for (int i = 0; i < 0xFF; ++i) {
        for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
            btns[userIndex][i] = XiButton::None;
        }
    }
}

void InputTranslationStruct::PopulateBtnLut(int userIndex, const UserProfile& profile) {
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

void InputTranslationStruct::PopulateFromConfig(const Config& config) {
    Clear();

    for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
        const auto& profileName = config.xiGamepadBindings[userIndex];
        if (profileName.empty()) continue;
        const auto& profile = *config.profiles.at(profileName);
        PopulateBtnLut(userIndex, profile);
    }
}

struct InputSrc_State {
    std::vector<IdevDevice> devices;

    Config config = {};
    InputTranslationStruct its = {};

    // https://github.com/ocornut/imgui/blob/master/examples/example_win32_directx11/main.cpp
    // For ImGui main viewport
    ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dDeviceContext = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    ID3D11RenderTargetView* mainRenderTargetView = nullptr;

    HWND mainWindow = NULL;

    // For a RAWINPUT*
    std::unique_ptr<std::byte[]> rawinput;
    size_t rawinputSize = 0;
};

static void HandleMouseMovement(LONG dx, LONG dy, InputTranslationStruct& its, const Config& config) {

}

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

static void CleanupRenderTarget(InputSrc_State& s) {
    if (s.mainRenderTargetView) {
        s.mainRenderTargetView->Release();
        s.mainRenderTargetView = nullptr;
    }
}

static void CleanupDeviceD3D(InputSrc_State& s) {
    CleanupRenderTarget(s);
    if (s.swapChain) { s.swapChain->Release(); s.swapChain = nullptr; }
    if (s.d3dDeviceContext) { s.d3dDeviceContext->Release(); s.d3dDeviceContext = nullptr; }
    if (s.d3dDevice) { s.d3dDevice->Release(); s.d3dDevice = nullptr; }
}

static void CreateRenderTarget(InputSrc_State& s) {
    ID3D11Texture2D* backBuffer;
    s.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    s.d3dDevice->CreateRenderTargetView(backBuffer, nullptr, &s.mainRenderTargetView);
    backBuffer->Release();
}

static bool CreateDeviceD3D(InputSrc_State& s, HWND hWnd) {
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    D3D_FEATURE_LEVEL featureLevelArray[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &s.swapChain, &s.d3dDevice, &featureLevel, &s.d3dDeviceContext);
    // Try high-performance WARP software driver if hardware is not available.
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &s.swapChain, &s.d3dDevice, &featureLevel, &s.d3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget(s);
    return true;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK InputSrc_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept {
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return true;

    auto& s = *(InputSrc_State*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_SIZE: {
        if (wParam == SIZE_MINIMIZED)
            return 0;
        auto resizeWidth = (UINT)LOWORD(lParam);
        auto resizeHeight = (UINT)HIWORD(lParam);
        CleanupRenderTarget(s);
        s.swapChain->ResizeBuffers(0, resizeWidth, resizeHeight, DXGI_FORMAT_UNKNOWN, 0);
        CreateRenderTarget(s);
        return 0;
    }

    case WM_CLOSE: {
        if (hwnd == s.mainWindow) {
            ShowWindow(hwnd, SW_HIDE);
            return 0;
        }
        else {
            break;
        }
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

            if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN) HandleKeyPress(VK_LBUTTON, true, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP) HandleKeyPress(VK_LBUTTON, false, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN) HandleKeyPress(VK_RBUTTON, true, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP) HandleKeyPress(VK_RBUTTON, false, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN) HandleKeyPress(VK_MBUTTON, true, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP) HandleKeyPress(VK_MBUTTON, false, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_DOWN) HandleKeyPress(VK_XBUTTON1, true, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_4_UP) HandleKeyPress(VK_XBUTTON1, false, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_DOWN) HandleKeyPress(VK_XBUTTON2, true, s.its, s.config);
            if (mouse.usButtonFlags & RI_MOUSE_BUTTON_5_UP) HandleKeyPress(VK_XBUTTON2, false, s.its, s.config);

            if (mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
                LOG_DEBUG("Warning: RAWINPUT reported absolute mouse corrdinates, not supported");
                break;
            } // else: MOUSE_MOVE_RELATIVE

            HandleMouseMovement(mouse.lLastX, mouse.lLastY, s.its, s.config);
        } break;

        case RIM_TYPEKEYBOARD: {
            const auto& kbd = ri->data.keyboard;

            // This message is a part of a longer makecode sequence -- the actual Vkey is in another one
            if (kbd.VKey == 0xFF)
                break;
            // All of the relevant keys that we support fit in a BYTE
            if (kbd.VKey > 0xFF)
                break;

            bool press = !(kbd.Flags & RI_KEY_BREAK);

            // TODO move this to config
            if (kbd.VKey == VK_F1) {
                ShowWindow(s.mainWindow, SW_SHOWNORMAL);
                SetFocus(s.mainWindow);
                break;
            }

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

void RunInputSource(HINSTANCE hinst, Config config) {
    LOG_DEBUG(L"Starting input source window");

    InputSrc_State s{
        .config = std::move(config),
    };
    s.config.onGamepadBindingChanged = [&](int userIndex, const std::string& profileName, const UserProfile& profile) { s.its.PopulateBtnLut(userIndex, profile); };
    s.its.PopulateFromConfig(s.config);

    UIState us{
        .config = &s.config,
    };

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = InputSrc_WndProc;
    wc.hInstance = hinst;
    wc.lpszClassName = L"WinXInputEmu";
    ATOM atom = RegisterClassExW(&wc);
    if (!atom) {
        LOG_DEBUG(L"Error creating Input Source window class: {}", GetLastErrorStr());
        return;
    }

    s.mainWindow = CreateWindowExW(
        0,
        MAKEINTATOM(atom),
        L"WinXInputEmu Config",
        WS_OVERLAPPEDWINDOW,

        // Position
        CW_USEDEFAULT, CW_USEDEFAULT,
        // Size
        1024, 640,

        NULL,  // Parent window    
        NULL,  // Menu
        hinst, // Instance handle
        NULL   // Additional application data
    );
    if (s.mainWindow == nullptr) {
        LOG_DEBUG(L"Error creating Input Source window: {}", GetLastErrorStr());
        return;
    }

    if (!CreateDeviceD3D(s, s.mainWindow)) {
        CleanupDeviceD3D(s);
        LOG_DEBUG(L"Error creating D3D context");
        return;
    }

    SetWindowLongPtrW(s.mainWindow, GWLP_USERDATA, (LONG_PTR)&s);
    ShowWindow(s.mainWindow, SW_SHOWDEFAULT);
    UpdateWindow(s.mainWindow);

    RAWINPUTDEVICE rid[2];

    // We don't use RIDEV_NOLEGACY because all the window manipulation (e.g. dragging the title bar) relies on the "legacy messages"
    // RIDEV_INPUTSINK so that we get input even if the game window is current in focus instead
    rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[0].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
    rid[0].hwndTarget = s.mainWindow;

    rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rid[1].dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK;
    rid[1].usUsage = HID_USAGE_GENERIC_MOUSE;
    rid[1].hwndTarget = s.mainWindow;

    if (RegisterRawInputDevices(rid, std::size(rid), sizeof(RAWINPUTDEVICE)) == false) {
        return;
    }

    // NB: we still can't run multiple copies of this thread, because ImGui context is global
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui_ImplWin32_Init(s.mainWindow);
    ImGui_ImplDX11_Init(s.d3dDevice, s.d3dDeviceContext);

    LOG_DEBUG(L"Starting working thread's main loop");
    bool done = false;
    while (!done) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport();
        ShowUI(us);

        ImGui::Render();
        constexpr ImVec4 kClearColor{ 0.45f, 0.55f, 0.60f, 1.00f };
        constexpr float kClearColorPremultAlpha[]{ kClearColor.x * kClearColor.w, kClearColor.y * kClearColor.w, kClearColor.z * kClearColor.w, kClearColor.w };
        s.d3dDeviceContext->OMSetRenderTargets(1, &s.mainRenderTargetView, nullptr);
        s.d3dDeviceContext->ClearRenderTargetView(s.mainRenderTargetView, kClearColorPremultAlpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        s.swapChain->Present(1, 0); // Present with vsync
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D(s);
    // Do we actually need this?
    //DestroyWindow(s.mainWindow);
    UnregisterClassW(MAKEINTATOM(atom), hinst);

    LOG_DEBUG(L"Stopping working thread");
}
