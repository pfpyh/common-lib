#include "hal/body/Default.hpp"

namespace common::hal
{
auto DefaultObserver::onEvent(double data) -> void
{
    onSpeedUpdate(data);
}
} // namespace common::hal