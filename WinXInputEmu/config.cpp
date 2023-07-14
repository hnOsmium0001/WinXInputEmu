#include "pch.h"

#include "config.h"

#include <algorithm>
#include <format>

#include "userdevice.h"

using namespace std::literals;

toml::table StringifyConfig(const Config& config)  noexcept {
    return {}; // TODO
}

static void ReadButton(toml::node_view<const toml::node> t, UserProfile::Button& btn) {
    btn.keyCode = KeyCodeFromString(t.value_or<std::string_view>(""sv)).value_or(0xFF);
}

static void ReadJoystick(toml::node_view<const toml::node> t, UserProfile::Joystick& js, UserProfile::Button& jsBtn) {
    ReadButton(t["Button"], jsBtn);

    if (const auto& v = t["Type"];
        v == "keyboard")
    {
        js.useMouse = false;
        ReadButton(t["Up"], js.kbd.up);
        ReadButton(t["Down"], js.kbd.down);
        ReadButton(t["Left"], js.kbd.left);
        ReadButton(t["Right"], js.kbd.right);
        js.kbd.speed = std::clamp(t["Speed"].value_or<float>(1.0f), 0.0f, 1.0f);
    }
    else if (v == "mouse") {
        js.useMouse = true;
        // TODO
    }
    else {
        // Resets to inactive mode
        js = {};
    }
}

Config LoadConfig(const toml::table& toml) noexcept {
    Config config;

    config.hotkeyShowUI = KeyCodeFromString(toml["HotKeys"]["ShowUI"].value_or<std::string_view>(""sv)).value_or(0xFF);

    if (auto tomlProfiles = toml["UserProfiles"].as_array()) {
        for (auto& e : *tomlProfiles) {
            auto e1 = e.as_table();
            if (!e1) continue;
            auto& tomlProfile = *e1;

            auto name = tomlProfile["Name"].value<std::string>();
            if (!name) {
                LOG_DEBUG("UserProfile cannot have empty name");
                continue;
            }

            auto profile = std::make_unique<UserProfile>();
            ReadButton(tomlProfile["A"], profile->a);
            ReadButton(tomlProfile["B"], profile->b);
            ReadButton(tomlProfile["X"], profile->x);
            ReadButton(tomlProfile["Y"], profile->y);
            ReadButton(tomlProfile["LB"], profile->lb);
            ReadButton(tomlProfile["RB"], profile->rb);
            ReadButton(tomlProfile["LT"], profile->lt);
            ReadButton(tomlProfile["RT"], profile->rt);
            ReadButton(tomlProfile["Start"], profile->start);
            ReadButton(tomlProfile["Back"], profile->back);
            ReadButton(tomlProfile["DpadUp"], profile->dpadUp);
            ReadButton(tomlProfile["DpadDown"], profile->dpadDown);
            ReadButton(tomlProfile["DpadLeft"], profile->dpadLeft);
            ReadButton(tomlProfile["DpadRight"], profile->dpadRight);
            ReadJoystick(tomlProfile["LStick"], profile->lstick, profile->rstickBtn);
            ReadJoystick(tomlProfile["RStick"], profile->rstick, profile->lstickBtn);

            config.profiles.try_emplace(name.value(), std::move(profile));
        }
    }

    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        auto key = std::format("Gamepad{}", i);
        config.xiGamepadBindings[i] = toml["Binding"][key].value_or<std::string>(""s);
    }

    return config;
}

void BindProfileToGamepad(int userIndex, const UserProfile& profile) {
    SrwExclusiveLock lock(gXiGamepadsLock);
    gXiGamepadsEnabled[userIndex] = true;
    gXiGamepads[userIndex] = {};
    gXiGamepads[userIndex].profile = &profile;
}

void BindAllConfigGamepadBindings(const Config& config) {
    SrwExclusiveLock lock(gXiGamepadsLock);
    for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
        const auto& profileName = config.xiGamepadBindings[userIndex];
        if (profileName.empty()) continue;

        auto iter = config.profiles.find(profileName);
        if (iter != config.profiles.end()) {
            const auto& profile = iter->second;
            LOG_DEBUG(L"Binding profile '{}' to gamepad {}", Utf8ToWide(profileName), userIndex);
            gXiGamepadsEnabled[userIndex] = true;
            gXiGamepads[userIndex] = {};
            gXiGamepads[userIndex].profile = profile.get();
        }
        else {
            LOG_DEBUG(L"Cannout find profile '{}' for binding gamepads, skipping", Utf8ToWide(profileName));
        }
    }
}
