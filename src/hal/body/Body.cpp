#include "hal/body/Body.hpp"

namespace common::hal
{
Body::Body(std::shared_ptr<Serial> Serial)
    : _serial(Serial) {}

auto Body::subscribe_update_speed(std::shared_ptr<BaseObserver<double>> observer) -> void
{
    _updateSpeedEvent.regist(std::move(observer));
}

auto Body::unsubscribe_update_speed(std::shared_ptr<BaseObserver<double>> observer) -> void
{
    _updateSpeedEvent.unregist(std::move(observer));
}

auto Body::__work() -> void
{
    const std::chrono::milliseconds delay(100);
    std::this_thread::sleep_for(delay);

    _updateSpeedEvent.notify(1.0);
}

auto Body::__create(std::shared_ptr<Serial> serial) -> std::shared_ptr<Body>
{
    return std::make_shared<Body>(serial);
}
} // namespace common::hal