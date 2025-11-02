/**********************************************************************
MIT License

Copyright (c) 2025 Park Younghwan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************************************************/

#include "common/communication/driver/Uart.hpp"

#if defined(WINDOWS)
#include <windows.h>
#elif defined(LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#endif

namespace common::communication
{
class SerialHandler
{
public :
    virtual ~SerialHandler() = default;

#if defined(WINDOWS)
public :
    virtual auto Wrapper_CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess,
                                    DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                    DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                                    HANDLE hTemplateFile) noexcept -> HANDLE = 0;
    virtual auto Wrapper_CloseHandle(HANDLE hObject) noexcept -> void = 0;
    virtual auto Wrapper_ReadFile(HANDLE hFile, LPVOID lpBuffer,
                                  DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead,
                                  LPOVERLAPPED lpOverlapped) noexcept -> bool = 0;
    virtual auto Wrapper_WriteFile(HANDLE hFile, LPCVOID lpBuffer,
                                   DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
                                   LPOVERLAPPED lpOverlapped) noexcept -> bool = 0;
    virtual auto Wrapper_GetCommState(HANDLE hFile, LPDCB lpDCB) noexcept -> bool = 0;
    virtual auto Wrapper_SetCommState(HANDLE hFile, LPDCB lpDCB) noexcept -> bool = 0;
    virtual auto Wrapper_SetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) noexcept -> bool = 0;
#elif defined(LINUX)
    virtual auto Wrapper_Open(const std::string& port, int32_t mode) noexcept -> int32_t = 0;
    virtual auto Wrapper_Close(int32_t fd) noexcept -> void = 0;
    virtual auto Wrapper_Read(int32_t fd, char* buffer, size_t size) noexcept -> ssize_t = 0;
    virtual auto Wrapper_Write(int32_t fd, const char* buffer, size_t size) noexcept -> bool = 0;
#endif
};

class SerialHandlerImpl : public SerialHandler
{
#if defined(WINDOWS)
public :
    auto Wrapper_CreateFile(LPCSTR lpFileName, DWORD dwDesiredAccess,
                            DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes,
                            HANDLE hTemplateFile) noexcept -> HANDLE override
    {
        return CreateFile(lpFileName, dwDesiredAccess, dwShareMode, 
                          lpSecurityAttributes, dwCreationDisposition, 
                          dwFlagsAndAttributes, hTemplateFile);
    }

    auto Wrapper_CloseHandle(HANDLE hObject) noexcept -> void override
    {
        CloseHandle(hObject);
    }

    virtual auto Wrapper_ReadFile(HANDLE hFile, LPVOID lpBuffer,
                                  DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead,
                                  LPOVERLAPPED lpOverlapped) noexcept -> bool
    {
        return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, 
                        lpNumberOfBytesRead, lpOverlapped);
    }

    virtual auto Wrapper_WriteFile(HANDLE hFile, LPCVOID lpBuffer,
                                   DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten,
                                   LPOVERLAPPED lpOverlapped) noexcept -> bool
    {
        return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, 
                         lpNumberOfBytesWritten, lpOverlapped);
    }

    virtual auto Wrapper_GetCommState(HANDLE hFile, LPDCB lpDCB) noexcept -> bool
    {
        return GetCommState(hFile, lpDCB);
    }

    virtual auto Wrapper_SetCommState(HANDLE hFile, LPDCB lpDCB) noexcept -> bool
    {
        return SetCommState(hFile, lpDCB);
    }

    virtual auto Wrapper_SetCommTimeouts(HANDLE hFile, LPCOMMTIMEOUTS lpCommTimeouts) noexcept -> bool
    {
        return SetCommTimeouts(hFile, lpCommTimeouts);
    }
#elif defined(LINUX)
    virtual auto Wrapper_Open(const std::string& port, int32_t mode) noexcept -> int32_t
    {
        return ::open(port.c_str(), mode);
    }

    virtual auto Wrapper_Close(int32_t fd) noexcept -> void
    {
        ::close(fd);
    }
    virtual auto Wrapper_Read(int32_t fd, char* buffer, size_t size) noexcept -> ssize_t
    {
        return ::read(fd, buffer, size);
    }

    virtual auto Wrapper_Write(int32_t fd, const char* buffer, size_t size) noexcept -> bool
    {
        return ::write(fd, buffer, size) > 0 ? true : false;
    }
#endif
};

Uart::Uart(UartInfo* info)
    : Uart(info, std::make_unique<SerialHandlerImpl>()) {}

Uart::~Uart()
{

}

auto Uart::open() noexcept -> bool
{
    return false;
}

auto Uart::close() noexcept -> void
{

}

auto Uart::read(unsigned char* buffer, size_t size) noexcept -> bool
{
    return false;
}

auto Uart::write(const unsigned char* buffer, size_t size) noexcept -> bool
{
    return false;
}
} // namespace common::communication