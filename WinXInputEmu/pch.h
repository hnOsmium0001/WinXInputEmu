#pragma once

////////// Standard library //////////

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

////////// System headers //////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// We don't include Xinput.h because dllmain.cpp wants separate declarations -- adding __declspec(dllexport), which is incompatible from the ones in Xinput.h
// We instead cherry-picked declarations from Windows SDK's Xinput.h into shadowed.h

////////// Project headers //////////

#include "export.h"
#include "shadowed.h"
#include "utils.h"
