#pragma once

namespace common::communication
{
class BaseSerial
{
public :
    virtual ~BaseSerial() = default;

    virtual auto open() -> bool = 0;
    virtual auto close() -> void = 0;
};
} // namespace common::communication