#pragma once

#include <format>
#include <string>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CONCAT_3(a, b, c) CONCAT(a, CONCAT(b, c))
#define CONCAT_4(a, b, c, d) CONCAT(CONCAT(a, b), CONCAT(c, d))

#define UNIQUE_NAME(prefix) CONCAT(prefix, __COUNTER__)
#define DISCARD UNIQUE_NAME(_discard)

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

std::wstring Utf8ToWide(std::string_view utf8);
std::string WideToUtf8(std::wstring_view wide);

std::wstring GetLastErrorStr() noexcept;

#define LOG_DEBUG(msg, ...) OutputDebugStringW(std::format(L"[WinXInputEmu] " msg, __VA_ARGS__).c_str())

template <typename TFunc>
struct ScopeGuard {
    TFunc func;

    ScopeGuard(TFunc func) noexcept : func{ std::move(func) } {}
    ~ScopeGuard() { func(); }
};

#define SCOPE_GUARD(name) ScopeGuard name = [&]()
#define DEFER ScopeGuard UNIQUE_NAME(scopeGuard) = [&]()
