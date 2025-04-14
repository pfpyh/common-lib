#pragma once

#include "CommonHeader.hpp"
#include <cmath>

namespace common::math
{
#ifndef M_PI
constexpr double M_PI = 3.14159265358979323846;
#endif
constexpr double DEG2RAD = 180 / M_PI;

} // namespace common::math