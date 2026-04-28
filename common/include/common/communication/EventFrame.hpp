/**********************************************************************
MIT License

Copyright (c) 2026 Park Younghwan

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

#include "common/CommonHeader.hpp"

namespace common::communication
{
/**
 * @brief Wire-format frame for the event pub/sub protocol.
 *
 * Frame layout: topic(2B) | type(1B) | payload(nB).
 */
struct Frame
{
    /**
     * @brief Frame type discriminator.
     */
    enum type : uint8_t
    {
        REGIST,
        UNREGIST,
        DATA,
        ACK,
    };

    uint16_t _topic;
    type _type;
    Bytes payload;

    /**
     * @brief Parses a raw byte buffer into a Frame.
     * @param raw Raw bytes from the TCP stream; must be at least 3 bytes long.
     * @return Parsed Frame with topic, type, and payload populated.
     * @warning Asserts @c raw.size() >= 3. Passing a shorter buffer is undefined behavior.
     */
    static auto parse(const Bytes& raw) -> Frame
    {
        assert(raw.size() >= 3);

        const uint16_t topic = (static_cast<uint16_t>(raw[0] << 8) | raw[1]);
        const auto type = static_cast<Frame::type>(raw[2]);
        const Bytes payload(raw.begin() + 3, raw.end());

        return {topic, type, payload};
    }

    /**
     * @brief Serializes frame fields into a byte buffer.
     * @param topic Topic identifier (big-endian, 2 bytes).
     * @param type Frame type discriminator.
     * @param payload Optional payload bytes. Defaults to empty.
     * @return Serialized frame ready for transmission.
     */
    static auto make(uint16_t topic, Frame::type type, const Bytes& payload = {}) -> Bytes
    {
        Bytes message;
        message.reserve(3 + payload.size());
        message.push_back(static_cast<uint8_t>(topic >> 8));
        message.push_back(static_cast<uint8_t>(topic & 0xFF));
        message.push_back(static_cast<uint8_t>(type));
        message.insert(message.end(), payload.begin(), payload.end());

        return message;
    }
};
} // namespace common::communication