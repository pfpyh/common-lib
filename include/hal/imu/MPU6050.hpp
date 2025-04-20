#pragma once

#include "CommonHeader.hpp"
#include "common/Observer.hpp"

#include "hal/types/ImuData.hpp"

#include <tuple>
#include <string>
#include <chrono>
#include <functional>
#include <unordered_map>

namespace common::hal::MPU6050
{
class COMMON_LIB_API DefaultObserver : public Observer<DefaultObserver, ImuData>
{
private :
    common::hal::ImuData _last;
    std::chrono::time_point<std::chrono::steady_clock> _prevRcvTime = std::chrono::steady_clock::now();
    double _direction = 0.0;

private :
    auto onEvent(common::hal::ImuData data) -> void override;

public :
    virtual auto onDirectionUpdate(const double direction) -> void = 0;
};

auto COMMON_LIB_API parse(const std::string& message) noexcept -> std::tuple<AccData, GyroData>;
} // namespace common::hal::MPU6050