#pragma once

#if defined(LINUX)

#include "common/NonCopyable.hpp"
#include "common/Factory.hpp"

#include <string>

#include <sys/ipc.h>
#include <sys/msg.h>

namespace common
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
} // namespace common

#endif
