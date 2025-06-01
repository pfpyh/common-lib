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

#include <stdint.h>

namespace common::utils
{
/**
 * @brief Calculates the next power of 2 greater than or equal to the given number.
 *
 * This function returns the smallest power of 2 that is greater than or equal to the input value @p n.
 * If @p n is already a power of 2, the function returns @p n itself.
 *
 * @param n The input unsigned 32-bit integer.
 * @return uint32_t The next power of 2 greater than or equal to @p n.
 */
auto next_pwr_of_2(uint32_t n) noexcept -> uint32_t
{
    if (n <= 1) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}
} // namespace common::utils