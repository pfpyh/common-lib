/**********************************************************************
MIT License

Copyright (c) 2026 Park Younghwan

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

#include "common/communication/serial/Uart.hpp"
#include "common/Logger.hpp"

#include <array>

namespace common::communication
{
namespace detail
{
class SerialHandler
{
public :
    virtual ~SerialHandler() = default;

#if defined(WINDOWS)
public :
    virtual auto Wrapper_CreateFile(LPCSTR lpFileName,
                                    DWORD dwDesiredAccess,
                                    DWORD dwShareMode,
                                    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                    DWORD dwCreationDisposition,
                                    DWORD dwFlagsAndAttributes,
                                    HANDLE hTemplateFile) -> HANDLE
    {
        return CreateFile(lpFileName, dwDesiredAccess, dwShareMode, 
                          lpSecurityAttributes, dwCreationDisposition, 
                          dwFlagsAndAttributes, hTemplateFile);
    }

    virtual auto Wrapper_CloseHandle(HANDLE hObject) -> void
    {
        CloseHandle(hObject);
    }

    virtual auto Wrapper_ReadFile(HANDLE hFile,
                                  LPVOID lpBuffer,
                                  DWORD nNumberOfBytesToRead,
                                  LPDWORD lpNumberOfBytesRead,
                                  LPOVERLAPPED lpOverlapped) -> bool
    {
        return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, 
                        lpNumberOfBytesRead, lpOverlapped);
    }

    virtual auto Wrapper_WriteFile(HANDLE hFile,
                                   LPCVOID lpBuffer,
                                   DWORD nNumberOfBytesToWrite,
                                   LPDWORD lpNumberOfBytesWritten,
                                   LPOVERLAPPED lpOverlapped) -> bool
    {
        return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, 
                         lpNumberOfBytesWritten, lpOverlapped);
    }

    virtual auto Wrapper_GetCommState(HANDLE hFile, LPDCB lpDCB) -> bool
    {
        return GetCommState(hFile, lpDCB);
    }

    virtual auto Wrapper_SetCommState(HANDLE hFile, LPDCB lpDCB) -> bool
    {
        return SetCommState(hFile, lpDCB);
    }

    virtual auto Wrapper_SetCommTimeouts(HANDLE hFile,
                                         LPCOMMTIMEOUTS lpCommTimeouts) -> bool
    {
        return SetCommTimeouts(hFile, lpCommTimeouts);
    }
#elif defined(LINUX)
    virtual auto Wrapper_Open(const std::string& port, int32_t mode) -> int32_t 
    {
        return ::open(port.c_str(), mode);
    }

    virtual auto Wrapper_Close(int32_t fd) -> void 
    { 
        ::close(fd); 
    }

    virtual auto Wrapper_Read(int32_t fd, char* buffer, size_t size) -> ssize_t 
    {
        return ::read(fd, buffer, size);
    }

    virtual auto Wrapper_Write(int32_t fd, const char* buffer, size_t size) -> bool
    {
        return ::write(fd, buffer, size) > 0 ? true : false;
    }
#endif
};

class UartImpl final : public Uart
{
private :
    const std::string _port;
    const Baudrate _baudrate;
    const uint8_t _mode;

    std::unique_ptr<SerialHandler> _handler;
    bool _isOpen = false;

#if defined(WINDOWS)
    HANDLE _handle;
#elif defined(LINUX)
    int32_t _fd;
#endif

public :
    explicit UartImpl(const std::string& port,
                      const Baudrate baudrate,
                      const uint8_t mode,
                      std::unique_ptr<SerialHandler> handler) noexcept
                : _port(port), _baudrate(baudrate), _mode(mode), _handler(std::move(handler)) {}

    ~UartImpl()
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

public :
    auto open() -> bool override
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
                _ERROR_("Already opened : %s", _port.c_str());
                return false;
            }
        }

    #if defined(WINDOWS)
        int32_t generic = 0;
        if ((_mode & SERIAL_READ) == SERIAL_READ) { generic |= GENERIC_READ; }
        if ((_mode & SERIAL_WRITE) == SERIAL_WRITE) { generic |= GENERIC_WRITE;}

        _handle = _handler->Wrapper_CreateFile(_port.c_str(), generic, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (_handle == INVALID_HANDLE_VALUE)
        {
            _ERROR_("Failed to serial open : %s", _port.c_str());
            return _isOpen;
        }

        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!_handler->Wrapper_GetCommState(_handle, &dcbSerialParams)) 
        {
            _ERROR_("Failed to get current serial parameters : %s", _port.c_str());
            _handler->Wrapper_CloseHandle(_handle); 
            return _isOpen;
        }

        int32_t CBRBaudrate = _baudrate.to_speed();

        dcbSerialParams.BaudRate = CBRBaudrate;
        dcbSerialParams.ByteSize = 8;
        dcbSerialParams.StopBits = ONESTOPBIT;
        dcbSerialParams.Parity = NOPARITY;

        if (!_handler->Wrapper_SetCommState(_handle, &dcbSerialParams)) 
        {
            _ERROR_("Failed to set serial parameters : %s", _port.c_str());
            _handler->Wrapper_CloseHandle(_handle); 
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
            _ERROR_("Failed to set timeouts : %s", _port.c_str());
            _handler->Wrapper_CloseHandle(_handle); 
            return _isOpen;
        }

        _isOpen = true;
        return _isOpen;
    #elif defined(LINUX)
        uint16_t modeValue = 0;
        if ((_mode & SERIAL_READ) == SERIAL_READ && (_mode & SERIAL_WRITE) != SERIAL_WRITE) 
        { 
            modeValue |= O_RDONLY; 
        }
        else if ((_mode & SERIAL_READ) != SERIAL_READ && (_mode & SERIAL_WRITE) == SERIAL_WRITE) 
        { 
            modeValue |= O_WRONLY;
        }
        else
        { 
            modeValue |= O_RDWR;
        }

        _fd = _handler->Wrapper_Open(_port.c_str(), modeValue | O_NOCTTY | O_NDELAY);
        if(_fd == -1) { return _isOpen; }

        struct termios options;
        tcgetattr(_fd, &options);
        cfsetispeed(&options, _baudrate.to_speed());
        cfsetospeed(&options, _baudrate.to_speed());
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

    auto close() -> void override
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

    auto read(char* buffer, size_t size) noexcept -> bool override
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
        int32_t readSize = _handler->Wrapper_Read(_fd, buffer, size);
        return (readSize > 0);
    #endif
    }

    auto readline(EscapeSequence::type escapeSequence) noexcept -> std::string override
    {
        static std::array<std::string, EscapeSequence::MAX> EndOfLine{ 
            std::string(1, '\0'),
            std::string(1, '\n'),
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

    auto write(const char* buffer, size_t size) noexcept -> bool override
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
};
} // namespace detail

auto Uart::__create(const std::string& port,
                    const Baudrate baudrate,
                    const uint8_t mode) noexcept -> std::shared_ptr<Uart>
{
    return std::make_shared<detail::UartImpl>(port, baudrate, mode, 
                                              std::make_unique<detail::SerialHandler>());
}
} // namespace common::communication
