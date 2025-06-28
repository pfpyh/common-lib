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

#include <cmath>

namespace common::math
{
class Euler;
class Quaternion;
template <typename T> class Matrix;

class COMMON_LIB_API Euler
{
public :
    double _roll = 0.0;
    double _pitch = 0.0;
    double _yaw = 0.0;

    Euler();
    Euler(const double roll, const double pitch, const double yaw);
    Euler(const Euler& e);
    Euler(Euler&& e) noexcept;

    auto to_quaternion() noexcept ->Quaternion;
    auto to_matrix() noexcept ->Matrix<double>;

    static auto from_matrix(const Matrix<double>& m) -> Euler;
    static auto from_acc(const double x, 
                         const double y, 
                         const double z) noexcept -> Euler;
    static auto from_acc(const double x,
                         const double y) noexcept -> Euler;
};

class COMMON_LIB_API Quaternion
{
public:
    double _w = 0.0;
    double _x = 0.0;
    double _y = 0.0;
    double _z = 0.0;

    Quaternion();
    Quaternion(const double w, const double x, const double y, const double z);
    Quaternion(const Quaternion& q);
    Quaternion(Quaternion&& q) noexcept;

    auto to_euler() noexcept ->Euler;
    auto to_matrix() noexcept ->Matrix<double>;

    static auto from_matrix(const Matrix<double>& m)->Quaternion;
};
} // namespace common::math