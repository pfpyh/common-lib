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

#include "common/lifecycle/Application.h"
#include "common/logging/Logger.hpp"

#include <signal.h>
#include <string>
#include <filesystem>
#include <thread>
#include <iostream>

#if defined(WINDOWS)
    #include <windows.h>
    #include <psapi.h>
#elif defined(LINUX)
    #include <unistd.h>
    #include <limits.h>
#endif

namespace common
{
#if defined(WINDOWS)
static HANDLE g_eventHandler = nullptr;

BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    switch (dwCtrlType) 
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            SetEvent(g_eventHandler);
            return TRUE;
        default:
            return FALSE;
    }
}
#endif

auto get_binary_path() -> std::string
{
    std::string path;
#ifdef WINDOWS
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length > 0 && length < MAX_PATH) 
    {
        path = std::move(std::string(buffer, length));
    }
#elif defined(LINUX)
    char buffer[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length != -1) 
    {
        buffer[length] = '\0';
        path = std::move(std::string(buffer));
    }
#endif
    return path;
}

auto get_binary_name(const std::string& fullPath) -> std::string
{
    try 
    {
        std::filesystem::path p(fullPath);
        return p.filename().string();
    } 
    catch (const std::exception&) 
    {
        size_t pos = fullPath.find_last_of("/\\");
        if (pos != std::string::npos) 
        {
            return fullPath.substr(pos + 1);
        }
        return fullPath;
    }
}

Application::~Application() = default;

auto Application::run() -> int32_t
{
    _path = get_binary_path();
    if(!_path.empty()) { _name = get_binary_name(_path); }

    _INFO_("Hello, my name is %s", _name.c_str());
#if defined(WINDOWS)
    g_eventHandler = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    if(!g_eventHandler)
    {
        _ERROR_("Failed to create ConsoleEventHandler");
        return -1;
    }

    if(!SetConsoleCtrlHandler(ConsoleCtrlHandler, true))
    {
        _ERROR_("Failed to set ConsoleCtrlHandler");
        CloseHandle(g_eventHandler);
        return -1;
    }

    _t->start([this]() {
        _INFO_("%s is initializing", _name.c_str());
        auto bootupFuture = bootup();
        if(bootupFuture) { bootupFuture->wait(); }
        _INFO_("%s is running", _name.c_str());

        while (!_shutdown.load()) 
        {
            DWORD signal = WaitForSingleObject(g_eventHandler, 1000); // 1sec timeout
            if (signal == WAIT_OBJECT_0) // Receive terminate signal
            {
                _INFO_("%s receiving shutdown signal(%d)", _name.c_str(), signal);
                signal_handler(SIGTERM);
            }
        }

        _INFO_("%s going to shutdown", _name.c_str());
        auto shutdownFuture = shutdown();
        if(shutdownFuture) { shutdownFuture->wait(); }
        _INFO_("%s will be closed", _name.c_str());
        
    }).wait();

    SetConsoleCtrlHandler(ConsoleCtrlHandler, FALSE);
    CloseHandle(g_eventHandler);
    g_eventHandler = nullptr;
#elif defined(LINUX)
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGHUP);

    pthread_sigmask(SIG_BLOCK, &set, nullptr);

    _t->start([this, set]() {
        _INFO_("%s is initializing", _name.c_str());
        auto bootupFuture = bootup();
        if(bootupFuture) { bootupFuture->wait(); }
        _INFO_("%s is running", _name.c_str());

        while (!_shutdown.load()) 
        {
            int32_t signal;
            if (sigwait(&set, &signal) == 0) 
            {
                _INFO_("%s receiving signal(%d)", _name.c_str(), signal);
                signal_handler(signal);
            }
        }

        _INFO_("%s going to shutdown", _name.c_str());
        auto shutdownFuture = shutdown();
        if(shutdownFuture) { shutdownFuture->wait(); }
        _INFO_("%s will be closed", _name.c_str());
    }).wait();
#endif
    
    _INFO_("Bye, %s", _name.c_str());
    return 0;
}

auto Application::signal_handler(int32_t signal) -> void
{
    switch(signal)
    {
        case SIGTERM :
        case SIGINT :
            _shutdown.store(true);
            break;
        default : break;
    }
}
} // namespace common
