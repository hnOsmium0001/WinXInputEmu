#include "pch.h"

#include "ui.h"

void ShowUI() {
    static bool gShowDemoWindow = false;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("WinXInputEmu")) {
            if (ImGui::MenuItem("Quit")) {
                PostQuitMessage(0);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("ImGui demo window", nullptr, &gShowDemoWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Gamepad bindings");
    ImGui::End();

    ImGui::Begin("Gamepad");
    ImGui::End();

    if (gShowDemoWindow) {
        ImGui::ShowDemoWindow(&gShowDemoWindow);
    }
}
