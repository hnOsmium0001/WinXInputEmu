#pragma once

#include "config.h"

struct UIState {
    std::unique_ptr<void, void(*)(void*)> p{ nullptr, nullptr };

    /* [In] */ Config* config;
};

void ShowUI(UIState& s);
