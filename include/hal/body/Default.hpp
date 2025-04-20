#pragma once

#include "CommonHeader.hpp"
#include "common/Observer.hpp"

namespace common::hal
{
class DefaultObserver : public Observer<DefaultObserver, double>
{
private :
    double _last = 0.0;

private :
    auto onEvent(double data) -> void override;

public :
    virtual auto onSpeedUpdate(const double speed) -> void = 0;
};
} // namespace common::hal