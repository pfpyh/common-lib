#pragma once

#include <gmock/gmock.h>

#include "common/communication/Serial.hpp"

namespace common::mock
{
class MockSerial : public Serial
{
public:
    MOCK_METHOD(bool, open, (const std::string&, const Baudrate, const uint8_t), (override, noexcept));
    MOCK_METHOD(void, close, (), (override, noexcept));
    MOCK_METHOD(bool, is_open, (), (override, noexcept));
    MOCK_METHOD(std::string, readline, (), (override, noexcept));
    MOCK_METHOD(bool, write, (const char*, const size_t), (override, noexcept));
};
} // namespace common::mock