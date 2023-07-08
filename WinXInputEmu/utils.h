#pragma once

#include <format>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct SrwExclusiveLock {
    SRWLOCK* theLock;

    SrwExclusiveLock(SRWLOCK& lock) noexcept
        : theLock{ &lock }
    {
        AcquireSRWLockExclusive(theLock);
    }

    ~SrwExclusiveLock() {
        ReleaseSRWLockExclusive(theLock);
    }
};

struct SrwSharedLock {
    SRWLOCK* theLock;

    SrwSharedLock(SRWLOCK& lock) noexcept
        : theLock{ &lock }
    {
        AcquireSRWLockShared(theLock);
    }

    ~SrwSharedLock() {
        ReleaseSRWLockShared(theLock);
    }
};

std::wstring GetLastErrorStr() noexcept;

#define LOG_DEBUG(msg, ...) OutputDebugStringW(std::format(L"[WinXInputEmu] " msg, __VA_ARGS__).c_str())
