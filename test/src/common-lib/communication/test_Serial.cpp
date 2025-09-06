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

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common/communication/Serial.hpp"

#include <array>

namespace common::detail::test
{
#if defined(WINDOWS)
class MockSerialHandler : public SerialHandler
{
public:
    MOCK_METHOD(HANDLE, Wrapper_CreateFile, (LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE), (override));
    MOCK_METHOD(void, Wrapper_CloseHandle, (HANDLE), (override));
    MOCK_METHOD(bool, Wrapper_ReadFile, (HANDLE, LPVOID, DWORD, LPDWORD, LPOVERLAPPED), (override));
    MOCK_METHOD(bool, Wrapper_WriteFile, (HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED), (override));
    MOCK_METHOD(bool, Wrapper_GetCommState, (HANDLE, LPDCB), (override));
    MOCK_METHOD(bool, Wrapper_SetCommState, (HANDLE, LPDCB), (override));
    MOCK_METHOD(bool, Wrapper_SetCommTimeouts, (HANDLE, LPCOMMTIMEOUTS), (override));
};
#elif defined(LINUX)
class MockSerialHandler : public SerialHandler
{
public:
    MOCK_METHOD(int32_t, Wrapper_Open, (const std::string&, int32_t), (override));
    MOCK_METHOD(void, Wrapper_Close, (int32_t), (override));
    MOCK_METHOD(ssize_t, Wrapper_Read, (int32_t, char*, size_t), (override));
    MOCK_METHOD(bool, Wrapper_Write, (int32_t, const char*, size_t), (override));
};
#endif

TEST(test_Serial, open)
{
    using namespace ::testing;
    {
        // given
        auto mockHandler = std::make_shared<MockSerialHandler>();
    #if defined(WINDOWS)
        EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(INVALID_HANDLE_VALUE));
    #elif defined(LINUX)
        EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(-1));
    #endif

        DetailSerial serial(mockHandler);

        // when
        const auto isOpen = serial.open("COM1", Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
        serial.close();

        // then
        ASSERT_FALSE(isOpen);
        ASSERT_FALSE(serial.is_open());
    }
#if defined(WINDOWS)
    {
        // given
        auto mockHandler = std::make_shared<MockSerialHandler>();
        EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(false));

        DetailSerial serial(mockHandler);

        // when
        const auto isOpen = serial.open("COM1", Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
        serial.close();

        // then
        ASSERT_FALSE(isOpen);
        ASSERT_FALSE(serial.is_open());
    }
    {
        // given
        auto mockHandler = std::make_shared<MockSerialHandler>();
        EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(false));

        DetailSerial serial(mockHandler);

        // when
        const auto isOpen = serial.open("COM1", Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
        serial.close();

        // then
        ASSERT_FALSE(isOpen);
        ASSERT_FALSE(serial.is_open());
    }
    {
        // given
        auto mockHandler = std::make_shared<MockSerialHandler>();
        EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(false));

        DetailSerial serial(mockHandler);

        // when
        const auto isOpen = serial.open("COM1", Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
        serial.close();

        // then
        ASSERT_FALSE(isOpen);
        ASSERT_FALSE(serial.is_open());
    }
#endif
    {
        // given
        auto mockHandler = std::make_shared<MockSerialHandler>();
    #if defined(WINDOWS)
        EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_CloseHandle(_)).Times(1);
    #elif defined(LINUX)
        EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(1));
        EXPECT_CALL(*mockHandler, Wrapper_Close(_)).Times(1);
    #endif

        DetailSerial serial(mockHandler);

        // when
        const auto isOpen = serial.open("COM1", Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
        serial.close();

        // then
        ASSERT_TRUE(isOpen);
        ASSERT_FALSE(serial.is_open());
    }
}

#if defined(WINDOWS)
TEST(test_Serial, read)
{
    // given
    const std::array<char, 4> input{'T', 'E', 'S', 'T'};

    auto mockHandler = std::make_shared<MockSerialHandler>();
    EXPECT_CALL(*mockHandler, Wrapper_ReadFile(::testing::_,
                                               ::testing::_,
                                               ::testing::_,
                                               ::testing::_,
                                               ::testing::_))
        .WillRepeatedly([&input](HANDLE hFile,
                                 LPVOID lpBuffer,
                                 DWORD nNumberOfBytesToWrite,
                                 LPDWORD lpNumberOfBytesWritten,
                                 LPOVERLAPPED lpOverlapped) -> bool {
            static size_t index = 0;
            if(index == 4) { return false; }
            *reinterpret_cast<char*>(lpBuffer) = input[index++];
            return true;
    });

    // when
    DetailSerial serial(mockHandler);
    char buffer[4] = {0, };
    auto success = serial.read(buffer, 4);

    // then
    ASSERT_TRUE(success);
    for(uint8_t i = 0; i < 4; ++i)
    {
        ASSERT_EQ(buffer[i], input.at(i));
    }
}
#endif

TEST(test_Serial, readline)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_shared<MockSerialHandler>();
#if defined(WINDOWS)
    EXPECT_CALL(*mockHandler, Wrapper_ReadFile(_, _, _, _, _))
        .WillOnce(DoAll(
            WithArg<1>([](void* buffer) { *static_cast<char*>(buffer) = 'H'; }),
            SetArgPointee<3>(1),
            Return(true)))
        .WillOnce(DoAll(
            WithArg<1>([](void* buffer) { *static_cast<char*>(buffer) = 'i'; }),
            SetArgPointee<3>(1),
            Return(true)))
        .WillOnce(DoAll(
            WithArg<1>([](void* buffer) { *static_cast<char*>(buffer) = '\0'; }),
            SetArgPointee<3>(1),
            Return(true)))
        .WillRepeatedly(Return(false));
#elif defined(LINUX)
    EXPECT_CALL(*mockHandler, Wrapper_Read(_, _, _))
        .WillOnce(DoAll(
            WithArg<1>([](void* buffer) { *static_cast<char*>(buffer) = 'H'; }),
            Return(1)))
        .WillOnce(DoAll(
            WithArg<1>([](void* buffer) { *static_cast<char*>(buffer) = 'i'; }),
            Return(1)))
        .WillOnce(DoAll(
            WithArg<1>([](void* buffer) { *static_cast<char*>(buffer) = '\0'; }),
            Return(1)))
        .WillRepeatedly(Return(0));
#endif

    DetailSerial serial(mockHandler);

    // when
    const std::string result(serial.readline(EscapeSequence::NULL_END));

    // then
    ASSERT_EQ(result, "Hi");
}

TEST(test_Serial, write)
{
    using namespace ::testing;
    {
        // given
        auto mockHandler = std::make_shared<MockSerialHandler>();
    #if defined(WINDOWS)
        EXPECT_CALL(*mockHandler, Wrapper_WriteFile(_, _, _, _, _)).WillOnce(Return(false));
    #elif defined(LINUX)
        EXPECT_CALL(*mockHandler, Wrapper_Write(_, _, _)).WillOnce(Return(false));
    #endif

        DetailSerial serial(mockHandler);

        // when
        const auto buffer = "Test";
        const bool rtn = serial.write(buffer, sizeof(buffer));

        // then
        ASSERT_FALSE(rtn);
    }
    {
        // given
        auto mockHandler = std::make_shared<MockSerialHandler>();
    #if defined(WINDOWS)
        EXPECT_CALL(*mockHandler, Wrapper_WriteFile(_, _, _, _, _)).WillOnce(Return(true));
    #elif defined(LINUX)
        EXPECT_CALL(*mockHandler, Wrapper_Write(_, _, _)).WillOnce(Return(true));
    #endif

        DetailSerial serial(mockHandler);

        // when
        const auto buffer = "Test";
        const bool rtn = serial.write(buffer, sizeof(buffer));

        // then
        ASSERT_TRUE(rtn);
    }
}
} // namespace common::test
