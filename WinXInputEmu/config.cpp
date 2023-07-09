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

    auto& tomlProfiles = *toml["UserProfiles"].as_array();
    for (auto& e : tomlProfiles) {
        auto& tomlProfile = *e.as_table();

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

    for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
        auto key = std::format("Gamepad{}", i);
        config.xiGamepadBindings[i] = toml["Binding"][key].value_or<std::string>(""s);
    }

    return config;
}

void BindProfileToGamepad(Config& config, int userIndex, std::string_view profileName) {
    if (userIndex < 0 || userIndex >= XUSER_MAX_COUNT)
        return;

    auto iter = config.profiles.find(profileName);
    if (iter != config.profiles.end()) {
        SrwExclusiveLock lock(gXiGamepadsLock);
        config.xiGamepadBindings[userIndex] = std::string(profileName);
        gXiGamepadsEnabled[userIndex] = true;
        gXiGamepads[userIndex] = {};
        gXiGamepads[userIndex].profile = iter->second.get();
    }
}

void BindAllConfigGamepadBindings(const Config& config) {
    SrwExclusiveLock lock(gXiGamepadsLock);
    for (int userIndex = 0; userIndex < XUSER_MAX_COUNT; ++userIndex) {
        gXiGamepadsEnabled[userIndex] = true;
        gXiGamepads[userIndex] = {};
        gXiGamepads[userIndex].profile = config.profiles.at(config.xiGamepadBindings[userIndex]).get();
    }
}
