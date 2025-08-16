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

#include <cassert>
#include <atomic>

#if defined(WINDOWS)
    #ifdef BUILDING_COMMON_LIB
        #define COMMON_LIB_API __declspec(dllexport)
    #else
        #define COMMON_LIB_API __declspec(dllimport)
    #endif
#else
    #ifndef COMMON_LIB_API
        #define COMMON_LIB_API
    #endif
#endif

#define SINGLE_INSTANCE_ONLY(ClassName) \
private: \
    static inline std::atomic<bool> _isSingleInstance{false}; \
    class InstanceGuard \
    { \
    public: \
        InstanceGuard() \
        { \
            bool expected = false; \
            if (!ClassName::_isSingleInstance.compare_exchange_strong(expected, true)) { \
                assert(false && #ClassName " can be created only once!"); \
                std::terminate(); \
            } \
        } \
        ~InstanceGuard() \
        { \
            ClassName::_isSingleInstance.store(false); \
        } \
    } _instanceGuard;