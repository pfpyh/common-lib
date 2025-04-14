#pragma once

#include "CommonHeader.hpp"
#include "common/Observer.hpp"

namespace common::hal
{
struct COMMON_LIB_API ImuRawData
{
    double _x = 0.0;
    double _y = 0.0;
    double _z = 0.0;
};

using GyroData = ImuRawData; /* unit = g */
using AccData = ImuRawData; /* unit = deg/s */
using ImuData = ImuRawData;
} // namespace common::hal