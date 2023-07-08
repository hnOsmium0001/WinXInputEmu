#include "pch.h"

#include "config.h"

#include <algorithm>
#include <format>
#include <toml++/toml.h>

#include "userdevice.h"

using namespace std::literals;

std::string StringifyConfig(const Config& config)  noexcept {
    return ""; // TODO
}

static void ReadButton(toml::node_view<toml::node> t, UserProfile::Button& btn) {
    btn.keyCode = KeyCodeFromString(t.value_or<std::string_view>(""sv)).value_or(0xFF);
}

static void ReadJoystick(toml::node_view<toml::node> t, UserProfile::Joystick& js, UserProfile::Button& jsBtn) {
    ReadButton(t["Button"], jsBtn);

    if (const auto& v = t["Type"];
        v == "keyboard")
    {
        ReadButton(t["Up"], js.kbd.up);
        ReadButton(t["Down"], js.kbd.down);
        ReadButton(t["Left"], js.kbd.left);
        ReadButton(t["Right"], js.kbd.right);
        js.kbd.speed = std::clamp(t["Speed"].value_or<float>(1.0f), 0.0f, 1.0f);
    }
    else if (v == "mouse") {
        // TODO
    }
}

void LoadConfig(Config& config, std::string_view str) noexcept {
    config.profiles.clear();
    config.xidevBindings = {};

    auto toml = toml::parse(str);

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
        ReadButton(tomlProfile["A"], profile->start);
        ReadButton(tomlProfile["A"], profile->back);
        ReadButton(tomlProfile["DpadUp"], profile->dpadUp);
        ReadButton(tomlProfile["DpadDown"], profile->dpadDown);
        ReadButton(tomlProfile["DpadLeft"], profile->dpadLeft);
        ReadButton(tomlProfile["DpadRight"], profile->dpadRight);
        ReadJoystick(tomlProfile["LStick"], profile->lstick, profile->rstickBtn);
        ReadJoystick(tomlProfile["RStick"], profile->rstick, profile->lstickBtn);

        config.profiles.try_emplace(name.value(), std::move(profile));
    }

    {
        SrwExclusiveLock lock(gXidevLock);
        for (int i = 0; i < XUSER_MAX_COUNT; ++i) {
            auto key = std::format("Gamepad{}", i);
            auto profileName = toml["Binding"][key].value<std::string_view>();
            if (profileName.has_value()) {
                auto iter = config.profiles.find(profileName);
                if (iter != config.profiles.end()) {
                    gXidevEnabled[i] = true;
                    gXidev[i] = {};
                    gXidev[i].profile = iter->second.get();
                    continue;
                }
            }

            gXidevEnabled[i] = false;
            gXidev[i] = {};
        }
    }
}
