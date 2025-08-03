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

#include "common/NonCopyable.hpp"
#include "common/thread/Thread.hpp"

#include <future>
#include <string>

namespace common
{
using Future = std::shared_ptr<std::future<void>>;

class Application : public NonCopyable
{
private :
    std::string _name{"Application"};
    std::string _path;
    std::shared_ptr<Thread> _t = Thread::create();
    std::atomic<bool> _shutdown{false};

public :
    virtual ~Application();

public :
    auto run() -> void;

private :
    auto signal_handler(int32_t signal) -> void;

public :
    virtual auto bootup() -> Future = 0;
    virtual auto shutdown() -> Future = 0;
};
} // namespace common