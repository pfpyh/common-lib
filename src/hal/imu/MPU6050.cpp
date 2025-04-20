#include "hal/imu/MPU6050.hpp"
#include "common/logging/Logger.hpp"

#include <sstream>

namespace common::hal::MPU6050
{
auto DefaultObserver::onEvent(common::hal::ImuData data) -> void
{
    auto now = std::chrono::steady_clock::now();
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _prevRcvTime);
    _prevRcvTime = now;

    constexpr double dt = 0.01; // 100Hz
    _direction += data._z * dt;
    while (_direction > 180.0) { _direction -= 360.0; }
    while (_direction < -180.0) { _direction += 360.0; }

    _INFO_("IMU[%f, %f, %f] DIR[%f] (%lld ms)", data._x, data._y, data._z, _direction, duration.count());

    onDirectionUpdate(_direction);
}

auto stoi(const std::string& str) -> int32_t
{
    if (str.empty()) { return 0; }

    size_t i = 0;
    bool negative = false;
    if (str[i] == '-') 
    {
        negative = true;
        i++;
    }   

    int32_t result = 0;
    bool found_digit = false;
    for (; i < str.size(); ++i) 
    {
        if (str[i] < 0) { return 0; }
        if (std::isdigit(str[i])) 
        {
            result = result * 10 + (str[i] - '0');
            found_digit = true;
        }
    }

    return found_digit ? (negative ? -result : result) : 0;
}
    
auto parse(const std::string& message) noexcept -> std::tuple<AccData, GyroData>
{
    std::vector<int32_t> values;
    std::string token;
    std::istringstream iss(message);
    while(std::getline(iss, token, ' '))
    {
        const auto value = ::common::hal::MPU6050::stoi(token);
        values.push_back(value);
    }

    if(values.size() != 6)
    {
        return {AccData(), GyroData()};
    }

    AccData accData;
    accData._x = static_cast<double>(values[0] / 16384);
    accData._y = static_cast<double>(values[1] / 16384);
    accData._z = static_cast<double>(values[2] / 16384);

    GyroData gyroData;
    gyroData._x = static_cast<double>(values[3] / 131);
    gyroData._y = static_cast<double>(values[4] / 131);
    gyroData._z = static_cast<double>(values[5] / 131);

    return {accData, gyroData};
}
} // namespace common::hal::MPU6050