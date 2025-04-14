#include "math/angle.hpp"
#include "math/matrix.hpp"

#include <cstdlib>

namespace common::math
{

Euler::Euler() {};
Euler::Euler(const double roll, const double pitch, const double yaw) 
    : _roll(roll), _pitch(pitch), _yaw(yaw) {};
Euler::Euler(const Euler& e) 
    : _roll(e._roll), _pitch(e._pitch), _yaw(e._yaw) {};
Euler::Euler(Euler&& e) noexcept
    : _roll(std::move(e._roll)), _pitch(std::move(e._pitch)), _yaw(std::move(e._yaw)) {};

auto Euler::to_quaternion() noexcept -> Quaternion
{
    Quaternion q;

    /*q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;*/

    const double sr = std::sin(_roll / 2);
    const double cr = std::cos(_roll / 2);
    const double sp = std::sin(_pitch / 2);
    const double cp = std::cos(_pitch / 2);
    const double sy = std::sin(_yaw / 2);
    const double cy = std::cos(_yaw / 2);

    q._w = (cr * cp * cy) + (sr * sp * sy);
    q._x = (sr * cp * cy) - (cr * sp * sy);
    q._y = (cr * sp * cy) + (sr * cp * sy);
    q._z = (cr * cp * sy) - (sr * sp * cy);

    return q;
};

auto Euler::to_matrix() noexcept -> Matrix<double>
{
    Matrix<double> m(3, 1);
    m[0][0] = _roll;
    m[1][0] = _pitch;
    m[2][0] = _yaw;
    return m;
};

auto Euler::from_matrix(const Matrix<double>& m) -> Euler
{
    if (m._col != 1 || m._row != 3)
        throw std::out_of_range("Matrix size not matched.");
    return Euler(m[0][0], m[1][0], m[2][0]);
};

auto Euler::from_acc(const double x,
                     const double y,
                     const double z) noexcept -> Euler
{
    Euler e;
    e._pitch = std::asin(x / 9.8);
    e._roll = std::asin((-1) * y / (9.8 * std::cos(e._pitch)));
    e._yaw = 0.0;
    return e;
};

auto Euler::from_acc(const double x,
                     const double y) noexcept -> Euler
{
    return from_acc(x, y, 0.0);
};

Quaternion::Quaternion() {};
Quaternion::Quaternion(const double w, const double x, const double y, const double z) : _w(w), _x(x), _y(y), _z(z) {};
Quaternion::Quaternion(const Quaternion& q) : _x(q._x), _y(q._y), _z(q._z), _w(q._w) {};
Quaternion::Quaternion(Quaternion&& q) noexcept
    : _x(std::move(q._x)), _y(std::move(q._y)), _z(std::move(q._z)), _w(std::move(q._w)) {};

auto Quaternion::to_euler() noexcept -> Euler
{
    Euler e;

#if 0
    //t0 = +2.0 * (w * x + y * z)
    //    t1 = +1.0 - 2.0 * (x * x + y * y)
    //    roll_x = math.atan2(t0, t1)
    const double t0 = 2.0 * (_w * _x + _y * _z);
    const double t1 = 1.0 - 2.0 * (_x * _x + _y * _y);
    e._x = std::atan2(t0, t1);

    //    t2 = +2.0 * (w * y - z * x)
    //    t2 = +1.0 if t2 > +1.0 else t2
    //    t2 = -1.0 if t2 < -1.0 else t2
    //    pitch_y = math.asin(t2)
    double t2 = 2.0 * (_w * _y - _z * _x);
    if (t2 > 1.0) t2 = 1.0;
    else if (t2 < -1.0) t2 = -1.0;
    e._y = (-1) * asin(t2);

    //    t3 = +2.0 * (w * z + x * y)
    //    t4 = +1.0 - 2.0 * (y * y + z * z)
    //    yaw_z = math.atan2(t3, t4)
    const double t3 = 2.0 * (_w * _z + _x * _y);
    const double t4 = 1.0 - 2.0 * (_y * _y - _z * _z);
    e._z = std::atan2(t3, t4);
#else
    /*phi = atan2(2 * (_y * _z + _w * _x), 1 - 2 * (_x * *2 + _y * *2))
    theta = -asin(2 * (_x * _z - _w * _y))
    psi = atan2(2 * (_x * _y + _w * _z), 1 - 2 * (_y * *2 + _z * *2))*/
    const double t0 = 2.0 * (_y * _z + _w * _x);
    const double t1 = 1.0 - 2.0 * (_x * _x + _y * _y);
    e._roll = std::atan2(t0, t1);

    const double t2 = 2.0 * (_x * _z - _w * _y);
    e._pitch = (-1.0) * std::asin(t2);

    const double t3 = 2.0 * (_x * _y + _w * _z);
    const double t4 = 1.0 - 2.0 * (_y * _y + _z * _z);
    e._yaw = std::atan2(t3, t4);
#endif

    return e;
};

auto Quaternion::to_matrix() noexcept -> Matrix<double>
{
    Matrix<double> m(4, 1);
    m[0][0] = _w;
    m[1][0] = _x;
    m[2][0] = _y;
    m[3][0] = _z;
    return m;
};

auto Quaternion::from_matrix(const Matrix<double>& m) -> Quaternion
{
    if (m._col != 1 || m._row != 4)
        throw std::out_of_range("Matrix size not matched.");
    return Quaternion(m[0][0], m[1][0], m[2][0], m[3][0]);
};
} // namespace common::math