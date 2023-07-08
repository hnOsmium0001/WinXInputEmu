#include "pch.h"

#include "utils.h"

std::wstring GetLastErrorStr() noexcept {
    // https://stackoverflow.com/a/17387176
    DWORD errId = ::GetLastError();
    if (errId == 0) {
        return std::wstring();
    }

    LPWSTR messageBuffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&messageBuffer, 0, NULL);

    std::wstring errMsg(messageBuffer, size);
    LocalFree(messageBuffer);
    return errMsg;
}
