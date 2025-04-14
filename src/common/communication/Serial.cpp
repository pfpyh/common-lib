#include "common/communication/Serial.hpp"
#include "common/logging/Logger.hpp"

namespace common
{
auto Serial::__create() noexcept -> std::shared_ptr<Serial>
{
    return std::make_shared<detail::DetailSerial>();
}

namespace detail
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

auto DetailSerial::open(const std::string& port,
                        const Baudrate baudrate,
                        const uint8_t mode) noexcept -> bool
{
#if defined(WINDOWS)
    if(_isOpen)
    {
        _ERROR_("Already opened : %s", port.c_str());
        return false;
    }

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

    int32_t CBRBaudrate = 0;
    switch (baudrate._value)
    {
        case Baudrate::_9600: CBRBaudrate = CBR_9600; break;
        case Baudrate::_14400: CBRBaudrate = CBR_14400; break;
        case Baudrate::_19200: CBRBaudrate = CBR_19200; break;
        case Baudrate::_38400: CBRBaudrate = CBR_38400; break;
        case Baudrate::_57600: CBRBaudrate = CBR_57600; break;
        case Baudrate::_115200: CBRBaudrate = CBR_115200; break;
        case Baudrate::_128000: CBRBaudrate = CBR_128000; break;
        case Baudrate::_256000: CBRBaudrate = CBR_256000; break;
        default: CBRBaudrate = CBR_9600; break;
    };

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
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
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

auto DetailSerial::readline() noexcept -> std::string
{
#if defined(WINDOWS)
    char ch;
    std::string line = "";

    DWORD bytesRead;
    while (true) 
    {
        if (!_handler->Wrapper_ReadFile(_handle, &ch, 1, &bytesRead, nullptr)) { break; }
        if (bytesRead > 0) 
        {
            if (ch == '\n') { break; }
            line += ch;
        }
    }
    return line;
#elif defined(LINUX)
    std::string result;
    char buffer[256];
    int32_t n;

    while (true) 
    {
        n = _handler->Wrapper_Read(_fd, buffer, sizeof(buffer) - 1);
        if (n > 0) 
        {
            buffer[n] = '\0';
            result += buffer;
            if (result.find('\n') != std::string::npos) { break; }
        } 
        else { break; }
    }
    return result;
#endif
}

auto DetailSerial::write(const char* buffer, const size_t size) noexcept -> bool
{
#if defined(WINDOWS)
    [[maybe_unused]] DWORD bytesWritten;
    return _handler->Wrapper_WriteFile(_handle, buffer, size, &bytesWritten, NULL);
#elif defined(LINUX)
    return _handler->Wrapper_Write(_fd, buffer, size);
#endif
}
} // namespace detail
} // namespace common
