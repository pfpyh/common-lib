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