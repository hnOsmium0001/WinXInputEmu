#include "pch.h"

#include "utils.h"

std::wstring Utf8ToWide(std::string_view utf8) {
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size(), nullptr, 0);
    std::wstring result;
    result.resize_and_overwrite(
        len,
        [utf8](wchar_t* buf, size_t bufSize) { return MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size(), buf, bufSize); });
    return result;
}

std::string WideToUtf8(std::wstring_view wide) {
    int len = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide.size(), nullptr, 0, nullptr, nullptr);
    std::string result;
    result.resize_and_overwrite(
        len,
        [wide](char* buf, size_t bufSize) { return WideCharToMultiByte(CP_UTF8, 0, wide.data(), wide.size(), buf, bufSize, nullptr, nullptr); });
    return result;

}

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
