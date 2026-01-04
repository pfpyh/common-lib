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
#include "common/threading/Thread.hpp"

#include <future>
#include <string>

namespace common::lifecycle
{
using Future = std::shared_ptr<std::future<void>>;

inline std::string appPath;
inline std::string appName{"application"};

static auto get_app_path() -> const std::string& { return appPath; }
static auto get_app_name() -> const std::string& { return appName; }

class COMMON_LIB_API Application : public NonCopyable
{
    SINGLE_INSTANCE_ONLY(Application)

private :
    std::shared_ptr<threading::Thread> _t = threading::Thread::create();
    std::atomic<bool> _shutdown{false};

public :
    Application() noexcept;
    virtual ~Application() noexcept;

public :
    auto run() -> int32_t;

private :
    auto signal_handler(int32_t signal) -> void;

public :
    virtual auto bootup() -> Future = 0;
    virtual auto shutdown() -> Future = 0;
};
} // namespace common::lifecycle