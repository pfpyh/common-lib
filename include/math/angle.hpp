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