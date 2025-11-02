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

#include "common/communication/Serial.hpp"
#include "common/logging/Logger.hpp"

#include <array>

namespace common::communication::detail
{
#if defined(WINDOWS)
auto SerialHandler::Wrapper_CreateFile(LPCSTR lpFileName,
                                       DWORD dwDesiredAccess,
                                       DWORD dwShareMode,
                                       LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                       DWORD dwCreationDisposition,
                                       DWORD dwFlagsAndAttributes,
                                       HANDLE hTemplateFile) -> HANDLE
{
    return CreateFile(lpFileName, 
                      dwDesiredAccess, 
                      dwShareMode, 
                      lpSecurityAttributes, 
                      dwCreationDisposition, 
                      dwFlagsAndAttributes, 
                      hTemplateFile);
}

auto SerialHandler::Wrapper_CloseHandle(HANDLE hObject) -> void
{
    CloseHandle(hObject);
}

auto SerialHandler::Wrapper_ReadFile(HANDLE hFile,
                                     LPVOID lpBuffer,
                                     DWORD nNumberOfBytesToRead,
                                     LPDWORD lpNumberOfBytesRead,
                                     LPOVERLAPPED lpOverlapped) -> bool
{
    return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

auto SerialHandler::Wrapper_WriteFile(HANDLE hFile,
                                      LPCVOID lpBuffer,
                                      DWORD nNumberOfBytesToWrite,
                                      LPDWORD lpNumberOfBytesWritten,
                                      LPOVERLAPPED lpOverlapped) -> bool
{
    return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

auto SerialHandler::Wrapper_GetCommState(HANDLE hFile,
                                         LPDCB lpDCB) -> bool
{
    return GetCommState(hFile, lpDCB);
}

auto SerialHandler::Wrapper_SetCommState(HANDLE hFile,
                                         LPDCB lpDCB) -> bool
{
    return SetCommState(hFile, lpDCB);
}

auto SerialHandler::Wrapper_SetCommTimeouts(HANDLE hFile,
                                            LPCOMMTIMEOUTS lpCommTimeouts) -> bool
{
    return SetCommTimeouts(hFile, lpCommTimeouts);
}
#elif defined(LINUX)
auto SerialHandler::Wrapper_Open(const std::string& port, int32_t mode) -> int32_t
{
    return ::open(port.c_str(), mode);
}

auto SerialHandler::Wrapper_Close(int32_t fd) -> void
{
    ::close(fd);
}

auto SerialHandler::Wrapper_Read(int32_t fd, char* buffer, size_t size) -> ssize_t
{
    return ::read(fd, buffer, size);
}

auto SerialHandler::Wrapper_Write(int32_t fd, const char* buffer, size_t size) -> bool
{
    return ::write(fd, buffer, size) > 0 ? true : false;
}
#endif

DetailSerial::DetailSerial(std::unique_ptr<SerialHandler>&& handler)
    : _handler(std::move(handler)) {}

DetailSerial::~DetailSerial()
{
    if constexpr (STRICT_MODE_ENABLED)
    {
        if(_isOpen)
        {
            _ERROR_("Serial is not closed");
            assert(false && "Serial is not closed");
        }
    }
}

auto DetailSerial::open(const std::string& port,
                        const Baudrate baudrate,
                        const uint8_t mode) noexcept -> bool
{
    if(_isOpen)
    {
        if constexpr (STRICT_MODE_ENABLED)
        {
            _ERROR_("Serial double open is not allow");
            assert(false && "Serial double open is not allow");
        }
        else
        {
            _ERROR_("Already opened : %s", port.c_str());
            return false;
        }
    }

#if defined(WINDOWS)
    int32_t generic = 0;
    if ((mode & SERIAL_READ) == SERIAL_READ) { generic |= GENERIC_READ; }
    if ((mode & SERIAL_WRITE) == SERIAL_WRITE) { generic |= GENERIC_WRITE;}

    _handle = _handler->Wrapper_CreateFile(port.c_str(), generic, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (_handle == INVALID_HANDLE_VALUE)
    {
        _ERROR_("Failed to serial open : %s", port.c_str());
        return _isOpen;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!_handler->Wrapper_GetCommState(_handle, &dcbSerialParams)) 
    {
        _ERROR_("Failed to get current serial parameters : %s", port.c_str());
        return _isOpen;
    }

    int32_t CBRBaudrate = baudrate.to_baudrate();

    dcbSerialParams.BaudRate = CBRBaudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!_handler->Wrapper_SetCommState(_handle, &dcbSerialParams)) 
    {
        _ERROR_("Failed to set serial parameters : %s", port.c_str());
        return _isOpen;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!_handler->Wrapper_SetCommTimeouts(_handle, &timeouts)) 
    {
        _ERROR_("Failed to set timeouts : %s", port.c_str());
        return _isOpen;
    }

    _isOpen = true;
    return _isOpen;
#elif defined(LINUX)
    uint16_t modeValue = 0;
    if ((mode & SERIAL_READ) == SERIAL_READ && (mode & SERIAL_WRITE) != SERIAL_WRITE) 
    { 
        modeValue |= O_RDONLY; 
    }
    else if ((mode & SERIAL_READ) != SERIAL_READ && (mode & SERIAL_WRITE) == SERIAL_WRITE) 
    { 
        modeValue |= O_WRONLY;
    }
    else
    { 
        modeValue |= O_RDWR;
    }

    _fd = _handler->Wrapper_Open(port.c_str(), modeValue | O_NOCTTY | O_NDELAY);
    if(_fd == -1) { return _isOpen; }

    struct termios options;
    tcgetattr(_fd, &options);
    cfsetispeed(&options, baudrate.to_speed());
    cfsetospeed(&options, baudrate.to_speed());
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    tcsetattr(_fd, TCSANOW, &options);

    _isOpen = true;
    return _isOpen;
#endif
}

auto DetailSerial::close() noexcept -> void
{
    if(_isOpen) 
    { 
#if defined(WINDOWS)
        _handler->Wrapper_CloseHandle(_handle); 
#elif defined(LINUX)
        _handler->Wrapper_Close(_fd); 
#endif
        _isOpen = false;
    }
}

auto DetailSerial::read(char* buffer, size_t size) noexcept -> bool
{
    if constexpr (STRICT_MODE_ENABLED)
    {
        if(!_isOpen)
        {
            _ERROR_("Serial is not opened");
            assert(false && "Serial is not opened");
        }
    }

#if defined(WINDOWS)
    char ch;
    DWORD bytesRead;
    size_t readSize = 0;
    while(true)
    {
        if (!_handler->Wrapper_ReadFile(_handle, &ch, 1, &bytesRead, nullptr)) { break; }
        if (bytesRead > 0) 
        {
            if(readSize < size) { buffer[readSize++] = ch; }
            else { break; } // buffer is full
        }
    }
    return (readSize > 0);
#elif defined(LINUX)
    int32_t readSize = _handler->Wrapper_Read(_fd, buffer, (size - 1));
    return (readSize > 0);
#endif
}

auto DetailSerial::readline(EscapeSequence::type escapeSequence) noexcept -> std::string
{
    static std::array<std::string, EscapeSequence::MAX> EndOfLine{ 
        std::string("\0"),
        std::string("\n"),
        std::string("\r\n"),
    };
    static_assert(EndOfLine.size() == EscapeSequence::MAX, 
                  "Size of EndOfLine should be fulfilled");

    if constexpr (STRICT_MODE_ENABLED)
    {
        if(!_isOpen)
        {
            _ERROR_("Serial is not opened");
            assert(false && "Serial is not opened");
        }
    }

    const std::string& end = EndOfLine[static_cast<size_t>(escapeSequence)];
    std::string line;
#if defined(WINDOWS)
    char ch;
    size_t endIdx = 0;

    DWORD bytesRead;
    while (true) 
    {
        if (!_handler->Wrapper_ReadFile(_handle, &ch, 1, &bytesRead, nullptr)) { break; }
        if (bytesRead > 0) 
        {
            if(endIdx < end.length() && ch == end[endIdx])
            {
                ++endIdx;
                line += ch;
                if(endIdx == end.length()) { break; }
            }
            else
            {
                if(endIdx > 0)
                {
                    for(size_t i = 0; i < endIdx; ++i) { line += end[i]; }
                    endIdx = 0;

                    if(ch == end[0]) { endIdx = 1; }
                    else { line += ch; }
                }
                else { line += ch; }
            }
        }
    }
#elif defined(LINUX)
    char buffer[256];
    size_t endIdx = 0;

    while (true) 
    {
        const int32_t readSize = _handler->Wrapper_Read(_fd, buffer, sizeof(buffer) - 1);
        if (readSize > 0) 
        {
            for (int32_t i = 0; i < readSize; ++i) 
            {
                char ch = buffer[i];
                if (endIdx < end.length() && ch == end[endIdx]) 
                {
                    ++endIdx;
                    line += ch;
                    if (endIdx == end.length()) { return line; }
                } 
                else 
                {
                    if (endIdx > 0) 
                    {
                        for (size_t j = 0; j < endIdx; ++j) { line += end[j]; }
                        endIdx = 0;

                        if (ch == end[0]) { endIdx = 1; } 
                        else { line += ch; }
                    } 
                    else { line += ch; }
                }
            }
        } 
        else { break; }
    }
#endif
    return line;
}

auto DetailSerial::write(const char* buffer, size_t size) noexcept -> bool
{
    if constexpr (STRICT_MODE_ENABLED)
    {
        if(!_isOpen)
        {
            _ERROR_("Serial is not opened");
            assert(false && "Serial is not opened");
        }
    }

#if defined(WINDOWS)
    [[maybe_unused]] DWORD bytesWritten;
    return _handler->Wrapper_WriteFile(_handle, buffer, size, &bytesWritten, NULL);
#elif defined(LINUX)
    return _handler->Wrapper_Write(_fd, buffer, size);
#endif
}
} // namespace common::communication::detail

namespace common::communication
{
auto Serial::__create() noexcept -> std::shared_ptr<Serial>
{
    return std::shared_ptr<detail::DetailSerial>(new detail::DetailSerial(std::make_unique<detail::SerialHandler>()), 
                                                 [](detail::DetailSerial* obj){
        obj->close();
        delete obj;
    });
}
} // namespace common::communication
