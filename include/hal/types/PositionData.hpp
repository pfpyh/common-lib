#pragma once

#include "CommonHeader.hpp"
#include "common/Observer.hpp"

namespace common::hal
{
struct COMMON_LIB_API PositionData
{
    uint32_t _type = 0x0;
    double _utc = 0.0;
    double _longitude = 0.0;
    double _latitude = 0.0;
    int32_t _altitude = 0;
};

class COMMON_LIB_API EventUpdatePositionData : public Subject<PositionData> {};
} // namespace hal