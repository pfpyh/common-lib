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

#pragma once

#include "common/communication/serial/BaseSerial.hpp"
#include "common/Factory.hpp"

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

class Baudrate;
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

inline constexpr auto get_default_escape() noexcept {
#if defined(WINDOWS)
    return EscapeSequence::CARRIAGE_RETURN;
#else
    return EscapeSequence::LINE_FEED;
#endif
}

/**
 * @brief Cross-platform serial communication interface
 * 
 * This class provides a unified interface for serial port communication across different platforms.
 * It abstracts the underlying platform-specific implementation (Windows HANDLE / Linux file descriptor)
 * 
 * @warning Ensure proper resource management by calling close() before destruction.
 *          The destructor will automatically close open connections.
 */
class COMMON_LIB_API Uart : public BaseSerial
                          , public Factory<Uart>
{
    friend class Factory<Uart>;

public :
    /**
     * @brief Reads data from the serial port
     * 
     * Reads available data from the serial port into the provided buffer.
     * 
     * Platform-specific behavior:
     * - Windows: Reads one byte at a time until no more data is available or buffer is full
     * - Linux: Reads up to (size-1) bytes in a single system call
     * 
     * @param buffer Pointer to the buffer where read data will be stored
     * @param size Maximum number of bytes to read into the buffer
     * @return bool True if any bytes were successfully read (readSize > 0), false otherwise
     * 
     * @note Returns true even for partial reads.
     *       Timeout behavior is platform-dependent (50ms base + 10ms per byte on Windows).
     *       Ensure buffer has sufficient capacity for the requested size.
     */
    virtual auto read(char* buffer, size_t size) noexcept -> bool = 0;

    /**
     * @brief Reads a line from the serial port
     * 
     * Reads data from the serial port until the specified escape sequence is encountered or no more data is available.
     * 
     * Platform-specific behavior:
     * - Windows: Reads one character at a time, matching escape sequence byte-by-byte
     * - Linux: Reads into 256-byte buffer chunks and processes data to find escape sequence
     * 
     * @param escapeSequence The escape sequence type to terminate the line (NULL_END, LINE_FEED, or CARRIAGE_RETURN)
     *                       Default is platform-dependent: CARRIAGE_RETURN on Windows, LINE_FEED on Linux
     * @return std::string The received line data including the escape sequence characters
     * 
     * @note Returns accumulated data even if escape sequence is not found (partial reads).
     *       Empty string indicates read failure or closed port.
     *       The returned string includes the escape sequence characters.
     *       Timeout behavior is platform-dependent (50ms base + 10ms per byte on Windows).
     */
    virtual auto readline(EscapeSequence::type escapeSequence = get_default_escape()) noexcept -> std::string = 0;

    /**
     * @brief Writes data to the serial port
     * 
     * Sends the specified buffer contents to the connected serial device.
     * 
     * Platform-specific behavior:
     * - Windows: Uses WriteFile() API with timeout settings (50ms base + 10ms per byte)
     * - Linux: Uses write() system call, returns true if any bytes written (> 0)
     * 
     * @param buffer Pointer to the data buffer to transmit
     * @param size Number of bytes to write from the buffer
     * @return bool True if write operation completed successfully, false otherwise
     * 
     * @note Linux implementation returns true for any successful writes (> 0 bytes).
     *       Windows implementation relies on WriteFile() return value.
     *       Ensure buffer remains valid during the entire write operation.
     *       Does not guarantee all bytes are written in a single call.
     */
    virtual auto write(const char* buffer, size_t size) noexcept -> bool = 0;

private :
    /**
     * @brief Factory method for creating Uart instances
     * 
     * Creates a platform-specific implementation of the Uart interface.
     * This method is used internally by the Factory base class to instantiate
     * a concrete UartImpl object with the specified configuration.
     * 
     * @param port The serial port device name (e.g., "COM1" on Windows, "/dev/ttyUSB0" on Linux)
     * @param baudrate The communication baud rate (9600, 19200, 38400, 57600, or 115200)
     * @param mode The access mode flags - combination of SERIAL_READ (0x01) and SERIAL_WRITE (0x02)
     *             Use SERIAL_READ for read-only, SERIAL_WRITE for write-only, or both for read-write
     * @return std::shared_ptr<Uart> Shared pointer to a new Uart instance
     * 
     * @note The returned instance must call open() before performing I/O operations.
     *       The serial handler is automatically created and managed internally.
     */
    static auto __create(const std::string& port,
                         const Baudrate baudrate,
                         const uint8_t mode) noexcept -> std::shared_ptr<Uart>;
};

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
    auto to_speed() const -> int32_t
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
} // namespace common::communication
