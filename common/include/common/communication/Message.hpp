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

#if defined(LINUX)

#include "common/NonCopyable.hpp"
#include "common/Factory.hpp"

#include <string>

#include <sys/ipc.h>
#include <sys/msg.h>

namespace common::communication
{
template <typename DataType>
class Message : public NonCopyable, public Factory<Message<DataType>>
{
    friend class Factory<Message<DataType>>;

private :
    struct MessageType
    {
        long _type;
        DataType _data;
    };
    key_t _key;
    int32_t _id;

public :
    Message(key_t key, int32_t id) 
        : _key(key), _id(id) {}

public :
    auto send(const DataType& data) noexcept -> bool
    {
        MessageType message;
        message._type = 1;
        message._data = data;
        return send(message);
    }

    auto send(DataType&& data) noexcept -> bool
    {
        MessageType message;
        message._type = 1;
        message._data = std::move(data);
        return send(message);
    }

    auto recv() noexcept -> DataType
    {
        MessageType message;
        const auto recvSize = msgrcv(_id, &message, sizeof(message._data), 1, 0);
        if(recvSize == sizeof(message._data))
        {
            if constexpr (std::is_move_constructible_v<DataType>)
            {
                return std::move(message._data);
            }
            else
            {
                return message._data;
            }            
        }
        return DataType();
    }

    auto close() noexcept -> bool
    {
        return msgctl(_id, IPC_RMID, nullptr) == 0 ? true : false;
    }

private :
    auto send(MessageType& message) noexcept -> bool
    {
        return msgsnd(_id, &message, sizeof(message._data), 0) == 0 ? true : false;
    }

private :
    static auto __create(const std::string& filePath, const int32_t projId) -> std::shared_ptr<Message>
    {
        const auto key = ftok(filePath.c_str(), projId);
        const auto id = msgget(key, 0666 | IPC_CREAT);
        return std::make_shared<Message>(key, id);
    }    
};
} // namespace common::communication

#endif
