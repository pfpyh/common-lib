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

#include "math/matrix.hpp"
#include "math/angle.hpp"

#include <stdint.h>

namespace common::math
{
template <typename T>
class COMMON_LIB_API KalmanFilter
{
private :
    const T _Q;
    const T _R;
    T _P;

public :
    KalmanFilter(const T& Q,
                 const T& R,
                 const T& P)
        : _Q(Q), _R(R), _P(P) {};

    auto run(const T& measurement, const T& prediction) -> T
    {
        // predict
        _P = _P + _Q;

        // kalman gain
        auto K = _P / (_P + _R);

        // estimate
        auto xp = prediction + K * (measurement - prediction);

        // update
        _P = (1 - K) * _P;

        return xp;
    };
};

template <typename T>
class COMMON_LIB_API KalmanFilter<Matrix<T>>
{
private :
    Matrix<T> _H;
    Matrix<T> _Q;
    Matrix<T> _R;
    Matrix<T> _x;
    Matrix<T> _P;
    bool _first = true;

public :
    KalmanFilter(const Matrix<T>& H,
                 const Matrix<T>& Q,
                 const Matrix<T>& R,
                 const Matrix<T>& x,
                 const Matrix<T>& P)
        : _H(H), _Q(Q), _R(R), _x(x), _P(P) {};

    auto run(const Matrix<T>& A, const Matrix<T>& z) -> decltype(_x)
    {
        if (!_first)
        {
            // predict
            auto xp = A * _x;
            auto Pp = A * _P * A.transpose() + _Q;

            // kalman gain
            auto K = (Pp * _H.transpose()) * util::inverse(_H * Pp * _H.transpose() + _R);

            // estimate
            _x = xp + K * (z - (_H * xp));
            _P = Pp - K * _H * Pp;
        }
        else _first = false;
        return _x;
    };
};
} // namespace common::math