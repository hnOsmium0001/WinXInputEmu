#pragma once

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// We don't include Xinput.h because dllmain.cpp wants separate declarations -- adding __declspec(dllexport), which is incompatible from the ones in Xinput.h
// We instead cherry-picked declarations from Windows SDK's Xinput.h into shadowed.h
