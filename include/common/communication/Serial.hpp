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

namespace common::communication
{
static constexpr uint8_t SERIAL_READ = 0x01;
static constexpr uint8_t SERIAL_WRITE = 0x02;

class COMMON_LIB_API Baudrate
{
public :
    enum type : uint8_t
    {
        _9600,
        _19200,
        _38400,
        _57600,
        _115200,
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
            case _19200 : return 19200;
            case _38400 : return 38400;
            case _57600 : return 57600;
            case _115200 : return 115200;
            default : return 0;
        }
    }

#if defined(WINDOWS)
    auto to_baudrate() const -> int32_t
    {
        switch (_value)
        {
            case Baudrate::_9600: return CBR_9600;
            case Baudrate::_19200: return CBR_19200;
            case Baudrate::_38400: return CBR_38400;
            case Baudrate::_57600: return CBR_57600;
            case Baudrate::_115200: return CBR_115200;
            default: return CBR_9600;
        };
    }
#elif defined(LINUX)
    auto to_speed() const -> speed_t
    {
        switch (_value)
        {
            case Baudrate::_9600: return B9600;
            case Baudrate::_19200: return B19200;
            case Baudrate::_38400: return B38400;
            case Baudrate::_57600: return B57600;
            case Baudrate::_115200: return B115200;
            default: return B9600;
        }
    }
#endif
};

struct EscapeSequence
{
    enum type : uint8_t
    {
        NULL_END = 0,     /*  \0  */
        LINE_FEED,        /*  \n  */
        CARRIAGE_RETURN,  /* \r\n */
        MAX,
    };
};

/**
 * @brief Cross-platform serial communication interface
 * 
 * This class provides a unified interface for serial port communication across different platforms.
 * It abstracts the underlying platform-specific implementation (Windows HANDLE / Linux file descriptor)
 * 
 * @warning Ensure proper resource management by calling close() before destruction.
 *          The destructor will automatically close open connections.
 */
class COMMON_LIB_API Serial : public NonCopyable
                            , public Factory<Serial>
{
    friend class Factory<Serial>;

public :
    virtual ~Serial() = default;

public :
    /**
     * @brief Opens a serial port connection
     * 
     * Establishes connection to the specified serial port with the given baudrate and access mode.
     * The mode parameter controls read/write permissions using SERIAL_READ and SERIAL_WRITE flags.
     * 
     * Platform-specific behavior:
     * - Windows: Uses CreateFile() with GENERIC_READ/WRITE flags, sets 8N1 configuration and timeouts
     * - Linux: Uses open() with O_RDWR/O_RDONLY/O_WRONLY flags, configures termios for 8N1
     * 
     * @param port The serial port identifier (e.g., "COM1" on Windows, "/dev/ttyUSB0" on Linux)
     * @param baudrate Communication speed setting (9600, 19200, 38400, 57600, or 115200)
     * @param mode Access mode flags (SERIAL_READ | SERIAL_WRITE for full duplex)
     * @return bool True if connection established successfully, false otherwise
     * 
     * @note Multiple calls to open() without close() will fail. Check is_open() before opening.
     *       Serial configuration is fixed to 8 data bits, no parity, 1 stop bit (8N1).
     */
    virtual auto open(const std::string& port,
                      const Baudrate baudrate,
                      const uint8_t mode) noexcept -> bool = 0;

    /**
     * @brief Closes the serial port connection
     * 
     * Safely closes the active serial port connection and releases system resources.
     * This operation is idempotent - multiple calls are safe.
     * 
     * @note Always call close() before opening a different port or when done with communication.
     */
    virtual auto close() noexcept -> void = 0;

    /**
     * @brief Checks if the serial port is currently open
     * 
     * @return bool True if a serial port connection is active, false otherwise
     */
    virtual auto is_open() noexcept -> bool = 0;

    virtual auto read(char* buffer, size_t size) noexcept -> bool = 0;

    /**
     * @brief Reads a line from the serial port
     * 
     * Reads data from the serial port until a newline character is encountered or no more data is available.
     * 
     * Platform-specific behavior:
     * - Windows: Reads one character at a time until '\n' is found, with configured timeouts
     * - Linux: Reads into buffer and accumulates until '\n' is found in the result string
     * 
     * @return std::string The received line data (may include '\r' but excludes '\n')
     * 
     * @note Returns accumulated data even if newline is not found (partial reads).
     *       Empty string indicates read failure or closed port.
     *       Timeout behavior is platform-dependent (50ms base + 10ms per byte on Windows).
     */
#if defined(WINDOWS)
    virtual auto readline(EscapeSequence::type escapeSequence = EscapeSequence::CARRIAGE_RETURN) noexcept -> std::string = 0;
#else
    virtual auto readline(EscapeSequence::type escapeSequence = EscapeSequence::LINE_FEED) noexcept -> std::string = 0;
#endif

    /**
     * @brief Writes data to the serial port
     * 
     * Sends the specified buffer contents to the connected serial device.
     * 
     * Platform-specific behavior:
     * - Windows: Uses WriteFile() API, returns success based on API call result
     * - Linux: Uses write() system call, returns true if any bytes written (> 0)
     * 
     * @param buffer Pointer to the data buffer to transmit
     * @param size Number of bytes to write from the buffer
     * @return bool True if write operation completed successfully, false otherwise
     * 
     * @note Linux implementation returns true for partial writes (> 0 bytes written).
     *       Windows implementation relies on WriteFile() return value.
     *       Ensure buffer remains valid during the entire write operation.
     */
    virtual auto write(const char* buffer, size_t size) noexcept -> bool = 0;

public :
    /**
     * @brief Factory method for creating Serial instances
     * 
     * Creates a platform-specific implementation of the Serial interface.
     * This method is used internally by the Factory base class.
     * 
     * @return std::shared_ptr<Serial> Shared pointer to a new Serial instance
     */
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
    std::unique_ptr<SerialHandler> _handler;
    bool _isOpen = false;

#if defined(WINDOWS)
    HANDLE _handle;
#elif defined(LINUX)
    int32_t _fd;
#endif

public :
    DetailSerial(std::unique_ptr<SerialHandler>&& handler);
    ~DetailSerial();

public :
    auto open(const std::string& port,
              const Baudrate baudRate,
              const uint8_t mode) noexcept -> bool override;
    auto close() noexcept -> void override;
    inline auto is_open() noexcept -> bool override { return _isOpen; }
    auto read(char* buffer, size_t size) noexcept -> bool override;
#if defined(WINDOWS)
    auto readline(EscapeSequence::type escapeSequence = EscapeSequence::CARRIAGE_RETURN) noexcept -> std::string override;
#else
    auto readline(EscapeSequence::type escapeSequence = EscapeSequence::LINE_FEED) noexcept -> std::string override;
#endif
    auto write(const char* buffer, size_t size) noexcept -> bool override;
};
} // namespace detail
} // namespace common::communication
