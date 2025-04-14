#pragma once

#include "common/thread/Runnable.hpp"
#include "common/Observer.hpp"
#include "common/Factory.hpp"
#include "common/communication/Serial.hpp"

#include "CommonHeader.hpp"

namespace common::hal
{
class COMMON_LIB_API UpdateSpeedEvent : public Subject<double> {};

class COMMON_LIB_API Body : public Runnable
                          , public Factory<Body>
{
    friend class Factory<Body>;

private :
    std::shared_ptr<Serial> _serial;

    UpdateSpeedEvent _updateSpeedEvent;

public :
    Body(std::shared_ptr<Serial> serial);

public :
    auto subscribe_update_speed(std::shared_ptr<BaseObserver<double>> observer) -> void;
    auto unsubscribe_update_speed(std::shared_ptr<BaseObserver<double>> observer) -> void;

private :
    auto __work() -> void override;
    static auto __create(std::shared_ptr<Serial> serial) -> std::shared_ptr<Body>;
};
} // namespace common::hal