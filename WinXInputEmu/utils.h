#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct SrwExclusiveLock {
    SRWLOCK* theLock;

    SrwExclusiveLock(SRWLOCK& lock)
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

    SrwSharedLock(SRWLOCK& lock)
        : theLock{ &lock }
    {
        AcquireSRWLockShared(theLock);
    }

    ~SrwSharedLock() {
        ReleaseSRWLockShared(theLock);
    }
};
