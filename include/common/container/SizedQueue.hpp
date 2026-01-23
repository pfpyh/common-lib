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

#include <queue>
#include <stdint.h>

namespace common
{
template <typename _tp, uint64_t _size>
class SizedQueue
{
    static_assert(_size != 0, "Size of the buffer must be greater than zero.");

private :
    std::queue<_tp> _q;

public :
    auto front() -> const _tp&
    { 
        return _q.front(); 
    }

    auto back() -> const _tp&
    { 
        return _q.back(); 
    }

    auto empty() -> bool { return _q.empty(); }
    auto size() -> size_t { return _q.size(); }

    auto push_back(const _tp& x) -> void { emplace_back(x); }
    auto push_back(_tp&& x) -> void { emplace_back(std::move(x)); }
    template <typename ...Args> auto push_back(Args&&... args) -> void { emplace_back(args...); }
    template <typename ...Args> auto emplace_back(Args&&... args) -> void { 
        if(_q.size() == _size) { _q.pop(); }
        _q.emplace(args...); 
    }

    auto pop_front() -> void { _q.pop(); }
};
} // namespace common
