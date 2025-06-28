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

namespace common
{
/**
 * @brief Base class that prevents objects of derived classes from being copied or assigned.
 * 
 * This class is a base class that prevents objects of derived classes from being copied or assigned.
 * It does this by deleting the copy constructor and copy assignment operator.
 */
class COMMON_LIB_API NonCopyable
{
public :
    /**
     * @brief Default constructor.
     */
    NonCopyable() = default;

    /**
     * @brief Default destructor.
     */
    virtual ~NonCopyable() = default;

    /**
     * @brief Deleted copy constructor.
     * 
     * @param other The object to be copied.
     */
    NonCopyable(const NonCopyable&) = delete;

    /**
     * @brief Deleted copy assignment operator.
     * 
     * @param other The object to be assigned.
     * @return A reference to the current object.
     */
    NonCopyable& operator=(const NonCopyable&) = delete;
};
} // namespace common