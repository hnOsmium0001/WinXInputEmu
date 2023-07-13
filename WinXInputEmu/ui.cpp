#include "pch.h"

#include "ui.h"

#include <imgui.h>
#include <imgui_stdlib.h>

struct UIStatePrivate {
    decltype(Config::xiGamepadBindings) xiGamepadBindingEditBuffer;
    bool showDemoWindow = false;
    int currentGamepad = -1;

    UIStatePrivate(UIState& s)
        : xiGamepadBindingEditBuffer(s.config->xiGamepadBindings)
    {
    }
};

void ShowUI(UIState& s) {
    if (s.p == nullptr) {
        void* p = new UIStatePrivate(s);
        void(*deleter)(void*) = [](void* p) { delete (UIStatePrivate*)p; };
        s.p = { p, deleter };
    }
    auto& p = *static_cast<UIStatePrivate*>(s.p.get());

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("WinXInputEmu")) {
            if (ImGui::MenuItem("Quit")) {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("ImGui demo window", nullptr, &p.showDemoWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Gamepad bindings");
    for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
        auto& profile = p.xiGamepadBindingEditBuffer[userIndex];
        char id[] = "Gamepad 0";
        //             0 ----> 8
        id[8] = '0' + userIndex;

        if (ImGui::InputText(id, &profile, ImGuiInputTextFlags_EnterReturnsTrue)) {
            LOG_DEBUG(L"UI: rebound gamepad {} to profile '{}'", userIndex, Utf8ToWide(profile));
            BindProfileToGamepad(*s.config, userIndex, profile);
        }
    }
    ImGui::End();

    ImGui::Begin("Gamepad");
    ImGui::End();

    if (p.showDemoWindow) {
        ImGui::ShowDemoWindow(&p.showDemoWindow);
    }
}
