#pragma once

#include "CommonHeader.hpp"
#include "common/NonCopyable.hpp"
#include "common/Factory.hpp"

#include <string>

#if defined(WINDOWS)
#include <windows.h>
#elif defined(LINUX)
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#endif

namespace common
{
static constexpr uint8_t SERIAL_READ = 0x01;
static constexpr uint8_t SERIAL_WRITE = 0x02;

class COMMON_LIB_API Baudrate
{
public :
    enum type : uint8_t
    {
        _9600,
        _14400,
        _19200,
        _38400,
        _57600,
        _115200,
        _128000,
        _256000,
    };

    type _value = _9600;

public :
    Baudrate() = default;
    Baudrate(const type value) : _value(value) {};
    Baudrate(const uint32_t value)
    {
        switch(value)
        {
            case 9600:
                _value = _9600;
                break;
            case 14400:
                _value = _14400;
                break;
            case 19200:
                _value = _19200;
                break;
            case 38400:
                _value = _38400;
                break;
            case 57600:
                _value = _57600;
                break;
            case 115200:
                _value = _115200;
                break;
            case 128000:
                _value = _128000;
                break;
            case 256000:
                _value = _256000;
                break;
            default:
                _value = _9600;
                break;
        }
    }

    auto operator=(const type value) -> Baudrate
    {
        return Baudrate(value);
    }

    auto operator=(const uint32_t value) -> Baudrate
    {
        return Baudrate(value);
    }

    auto uint32_t() const
    {
        switch(_value)
        {
            case _9600 : return 9600;
            case _14400 : return 14400;
            case _19200 : return 19200;
            case _38400 : return 38400;
            case _57600 : return 57600;
            case _115200 : return 115200;
            case _128000 : return 128000;
            case _256000 : return 256000;
            default : return 0;
        }
    }
};

class COMMON_LIB_API Serial : public NonCopyable
                            , public Factory<Serial>
{
    friend class Factory<Serial>;

public :
    virtual ~Serial() = default;

public :
    virtual auto open(const std::string& port,
                      const Baudrate baudrate,
                      const uint8_t mode) noexcept -> bool = 0;
    virtual auto close() noexcept -> void = 0;
    virtual auto is_open() noexcept -> bool = 0;
    virtual auto readline() noexcept -> std::string = 0;
    virtual auto write(const char* buffer, const size_t size) noexcept -> bool = 0;

public :
    static auto __create() noexcept -> std::shared_ptr<Serial>;
};

namespace detail
{
class COMMON_LIB_API SerialHandler
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
                                    HANDLE hTemplateFile) -> HANDLE;

    virtual auto Wrapper_CloseHandle(HANDLE hObject) -> void;

    virtual auto Wrapper_ReadFile(HANDLE hFile,
                                  LPVOID lpBuffer,
                                  DWORD nNumberOfBytesToRead,
                                  LPDWORD lpNumberOfBytesRead,
                                  LPOVERLAPPED lpOverlapped) -> bool;

    virtual auto Wrapper_WriteFile(HANDLE hFile,
                                   LPCVOID lpBuffer,
                                   DWORD nNumberOfBytesToWrite,
                                   LPDWORD lpNumberOfBytesWritten,
                                   LPOVERLAPPED lpOverlapped) -> bool;  

    virtual auto Wrapper_GetCommState(HANDLE hFile,
                                      LPDCB lpDCB) -> bool;

    virtual auto Wrapper_SetCommState(HANDLE hFile,
                                      LPDCB lpDCB) -> bool;

    virtual auto Wrapper_SetCommTimeouts(HANDLE hFile,
                                         LPCOMMTIMEOUTS lpCommTimeouts) -> bool;
#elif defined(LINUX)
    virtual auto Wrapper_Open(const std::string& port, int32_t mode) -> int32_t;
    virtual auto Wrapper_Close(int32_t fd) -> void;
    virtual auto Wrapper_Read(int32_t fd, char* buffer, size_t size) -> ssize_t;
    virtual auto Wrapper_Write(int32_t fd, const char* buffer, size_t size) -> bool;
#endif
};

class COMMON_LIB_API DetailSerial : public Serial
{
private :
    std::shared_ptr<SerialHandler> _handler;
    bool _isOpen = false;

#if defined(WINDOWS)
    HANDLE _handle;
#elif defined(LINUX)
    int32_t _fd;
#endif

public :
    DetailSerial()
        : _handler(std::make_shared<SerialHandler>()) {}
    DetailSerial(std::shared_ptr<SerialHandler> handler)
        : _handler(handler) {}

public :
    auto open(const std::string& port,
              const Baudrate baudRate,
              const uint8_t mode) noexcept -> bool override;
    auto close() noexcept -> void override;
    inline auto is_open() noexcept -> bool override { return _isOpen; }
    auto readline() noexcept -> std::string override;
    auto write(const char* buffer, const size_t size) noexcept -> bool override;
};
} // namespace detail
} // namespace common
