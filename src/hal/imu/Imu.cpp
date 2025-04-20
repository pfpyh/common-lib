#include "hal/imu/Imu.hpp"
#include "hal/imu/MPU6050.hpp"

#include "common/logging/Logger.hpp"
#include "math/constant.hpp"

namespace common::hal
{
Imu::Imu(std::shared_ptr<Serial> serial)
    : _serial(serial) {}

auto Imu::subscribe_update_imuData(std::shared_ptr<BaseObserver<ImuData>> observer) -> void
{
    _updateImuEvent.regist(std::move(observer));
}

auto Imu::unsubscribe_update_imuData(std::shared_ptr<BaseObserver<ImuData>> observer) -> void
{
    _updateImuEvent.unregist(std::move(observer));
}

auto Imu::__work() -> void
{
    const std::string msg(_serial->readline());
    const auto [acc, gyr] = MPU6050::parse(msg);

    /********************************************************************************/
    /* Complimentary Filter */
    /********************************************************************************/
    // constexpr double ALPHA = 0.98;
    // constexpr double dt = 0.01;

    // auto roll = atan2(acc._y, acc._z) * math::DEG2RAD;
    // auto pitch = atan(-acc._x / sqrt(acc._y * acc._y + acc._z * acc._z)) * math::DEG2RAD;    

    // _data._x = (1 - ALPHA) * (_data._x + gyr._x * dt) + ALPHA * roll;
    // _data._y = (1 - ALPHA) * (_data._y + gyr._y * dt) + ALPHA * roll;
    /********************************************************************************/

    /********************************************************************************/
    /* Kalman Filter */
    /*****************************************4***************************************/
    constexpr double dt = 0.01; // 100Hz

    const auto accRoll = atan2(acc._y, acc._z);
    const auto orgPitch = (acc._y * acc._y) + (acc._z * acc._z);
    const auto accPitch = orgPitch == 0 ? 0 : atan(-acc._x / sqrt(orgPitch));

    const auto gyrRoll = gyr._x * math::DEG2RAD;
    const auto gyroPitch = gyr._y * math::DEG2RAD;
    const auto gyrYaw = gyr._z * math::DEG2RAD;

    _data._x = _filterRoll.run(accRoll, _data._x + gyrRoll * dt);
    _data._y = _filterRoll.run(accPitch, _data._y + gyroPitch * dt);
    /********************************************************************************/    

    _data._z = _yawCal.calculate(gyr._z, acc._x, acc._y, dt);

    _updateImuEvent.notify(_data);
}

auto Imu::__create(std::shared_ptr<Serial> serial) -> std::shared_ptr<Imu>
{
    return std::make_shared<Imu>(serial);
}
} // namespace common::hal