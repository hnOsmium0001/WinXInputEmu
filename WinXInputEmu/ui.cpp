#include "pch.h"

#include "ui.h"

#include <imgui.h>
#include <imgui_stdlib.h>

#include "userdevice.h"

#define FORMAT_GAMEPAD_NAME(VAR, USER_INDEX ) char VAR[256]; snprintf(VAR, sizeof(VAR), "Gamepad %d", (int)USER_INDEX);

struct UIStatePrivate {
    int selectedUserIndex = -1;
    bool showDemoWindow = false;

    UIStatePrivate(UIState& s)
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

    ImGui::Begin("Gamepads");
    for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
        FORMAT_GAMEPAD_NAME(id, userIndex);
        bool selected = p.selectedUserIndex == userIndex;
        if (ImGui::Selectable(id, &selected)) {
            p.selectedUserIndex = userIndex;
        }
    }
    ImGui::End();

    ImGui::Begin("Gamepad info");
    if (p.selectedUserIndex != -1) {
        auto userIndex = p.selectedUserIndex;
        auto& profileName = s.config->xiGamepadBindings[userIndex];

        if (ImGui::Button("Rebind keyboard")) {
            s.bindIdevFromNextKey = userIndex;
        }
        ImGui::SameLine();
        if (ImGui::Button("Rebind mouse")) {
            s.bindIdevFromNextMouse = userIndex;
        }

        if (s.bindIdevFromNextKey == userIndex)
            ImGui::Text("Bound keyboard: [press any key]");
        else
            if (gXiGamepads[userIndex].srcKbd == INVALID_HANDLE_VALUE)
                ImGui::Text("Bound keyboard: [any]");
            else
                ImGui::Text("Bound keyboard: %p", gXiGamepads[userIndex].srcKbd);

        if (s.bindIdevFromNextMouse == userIndex)
            ImGui::Text("Bound mouse: [press any mouse button]");
        else
            if (gXiGamepads[userIndex].srcMouse == INVALID_HANDLE_VALUE)
                ImGui::Text("Bound mouse: [any]");
            else
                ImGui::Text("Bound mouse: %p", gXiGamepads[userIndex].srcMouse);

        if (ImGui::InputText("Profile name", &profileName)) {
            auto iter = s.config->profiles.find(profileName);
            if (iter != s.config->profiles.end()) {
                auto& profile = *iter->second;

                LOG_DEBUG(L"UI: rebound gamepad {} to profile '{}'", userIndex, Utf8ToWide(profileName));
                BindProfileToGamepad(userIndex, profile);
                CALL_IF_NOT_NULL(s.config->onGamepadBindingChanged, userIndex, profileName, profile);
            }
        }
    }
    else {
        ImGui::Text("Select a gamepad to show details");
    }
    ImGui::End();

    if (p.showDemoWindow) {
        ImGui::ShowDemoWindow(&p.showDemoWindow);
    }
}
