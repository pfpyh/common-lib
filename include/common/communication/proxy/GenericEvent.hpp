/**********************************************************************
MIT License

Copyright (c) 2025 Park Younghwan

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************************************************/

#pragma once

#include "CommonHeader.hpp"

#include "common/communication/Event.hpp"

#include <cstring>

namespace common::communication
{
class GenericEventBus
{
private :
    EventBus _bus;

public :
    template <typename DataType>
    auto subscribe(const std::string& topic, 
                   std::function<void(const DataType&)> handler) -> EventBus::SubID
    {
        static_assert(std::is_trivially_copyable_v<DataType>,
                      "DataType must be copyable");
        return _bus.subscribe(topic, [handler](const std::vector<uint8_t>& payload){
            auto data = GenericEventBus::deserialize<DataType>(payload);
            handler(data);
        });
    }

    auto unsubscribe(EventBus::SubID subId) -> void
    {
        _bus.unsubscribe(subId);
    }

    template <typename DataType>
    auto publish(const std::string& topic, const DataType& data) -> void
    {
        static_assert(std::is_trivially_copyable_v<DataType>,
                      "DataType must be copyable");
        _bus.publish(topic, GenericEventBus::serialize(data));
    }

private :
    template <typename DataType>
    static auto serialize(const DataType& data) -> std::vector<uint8_t>
    {
        std::vector<uint8_t> payload(sizeof(DataType));
        std::memcpy(payload.data(), &data, sizeof(DataType));
        return payload;
    }

    template <typename DataType>
    static auto deserialize(const std::vector<uint8_t>& payload) -> DataType
    {
        DataType data;
        std::memcpy(&data, payload.data(), sizeof(DataType));
        return data;
    }
};
} // namespace common::communication