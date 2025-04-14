#pragma once

#include "CommonHeader.hpp"
#include "common/Observer.hpp"
#include "common/thread/Runnable.hpp"
#include "common/Factory.hpp"
#include "common/communication/Serial.hpp"

#include "math/filter/KalmanFilter.hpp"

#include "hal/types/ImuData.hpp"

namespace common::hal
{
namespace detail
{
class COMMON_LIB_API YawCalculator
{
private :
    double _prevAccYaw = 0.0;
    double _prevGyrYaw = 0.0;
    const double _alpha = 0.98;

public :
    auto calculate(double gyrZ, double accX, double accY, double dt) -> double
    {
        const auto accYaw = atan2(accY, accX);
        const auto accYawRate = (accYaw - _prevAccYaw) / dt;
        const auto fusionYawRate = _alpha * gyrZ + (1 - _alpha) * accYawRate;
        _prevAccYaw = accYaw;
        return fusionYawRate;
    }
};
} // namespace detail

class COMMON_LIB_API UpdateImuEvent : public Subject<ImuData> {};

class COMMON_LIB_API Imu : public Runnable
                         , public Factory<Imu>
{
    friend class Factory<Imu>;

private :
    std::shared_ptr<Serial> _serial;

    math::KalmanFilter<double> _filterRoll{0.1, 1.0, 1.0};
    math::KalmanFilter<double> _filterPitch{0.1, 1.0, 1.0};
    math::KalmanFilter<double> _filterYaw{0.1, 1.0, 1.0};

    detail::YawCalculator _yawCal;

    UpdateImuEvent _updateImuEvent;
    ImuData _data;

public :
    Imu(std::shared_ptr<Serial> serial);

public :
    auto subscribe_update_imuData(std::shared_ptr<BaseObserver<ImuData>> observer) -> void;
    auto unsubscribe_update_imuData(std::shared_ptr<BaseObserver<ImuData>> observer) -> void;

private :
    auto __work() -> void override;
    static auto __create(std::shared_ptr<Serial> serial) -> std::shared_ptr<Imu>;
};
} // namespace common::hal