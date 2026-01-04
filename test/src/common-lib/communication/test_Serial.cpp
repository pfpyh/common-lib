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
#include "common/Logger.hpp"

#include <array>

namespace common::communication::detail::test
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
#endif

#if defined(LINUX)
class MockSerialHandler : public SerialHandler
{
public:
    MOCK_METHOD(int32_t, Wrapper_Open, (const std::string&, int32_t), (override));
    MOCK_METHOD(void, Wrapper_Close, (int32_t), (override));
    MOCK_METHOD(ssize_t, Wrapper_Read, (int32_t, char*, size_t), (override));
    MOCK_METHOD(bool, Wrapper_Write, (int32_t, const char*, size_t), (override));
};
#endif

static constexpr char TEST_SERIAL_PATH[] = "TestPath";

TEST(test_Serial, open_success)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    // when
#if defined(WINDOWS)
    EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_CloseHandle(_)).Times(1);
#elif defined(LINUX)
    EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(20));
#endif

    DetailSerial serial(std::move(mockHandler));
    const auto isOpen = serial.open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
    serial.close();

    // then
    ASSERT_TRUE(isOpen);
    ASSERT_FALSE(serial.is_open());
}

TEST(test_Serial, open_success_without_close)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    // when
#if defined(WINDOWS)
    EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_CloseHandle(_)).Times(1);
#elif defined(LINUX)
    EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(20));
#endif

    auto* serial = new DetailSerial(std::move(mockHandler));
    const auto isOpen = serial->open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);

    // then
    ASSERT_TRUE(isOpen);
    ASSERT_TRUE(serial->is_open());
#if STRICT_MODE_ENABLED
    ASSERT_DEATH({ delete serial; }, "Serial is not closed");
#endif
}

TEST(test_Serial, open_double)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    // when & then
#if defined(WINDOWS)
    EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_CloseHandle(_)).Times(1);
#elif defined(LINUX)
    EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(20));
#endif

    auto* serial = new DetailSerial(std::move(mockHandler));
    const auto isOpen = serial->open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
    _INFO_("STRICT_MODE_ENABLED=%d", STRICT_MODE_ENABLED);
#if STRICT_MODE_ENABLED
    ASSERT_DEATH({
        serial->open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_READ | SERIAL_WRITE);
    }, "Serial double open is not allow");
#else
    ASSERT_FALSE(serial->open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_READ | SERIAL_WRITE));
#endif
}

TEST(test_Serial, read_success)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    const std::array<char, 4> input{'T', 'E', 'S', 'T'};

    // when
#if defined(WINDOWS)
    EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_CloseHandle(_)).Times(1);
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
#elif defined(LINUX)
    EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(20));
    EXPECT_CALL(*mockHandler, Wrapper_Read(_, _, _))
        .WillRepeatedly([&input](int32_t fd, char* buffer, size_t size) -> ssize_t {
            static size_t index = 0;
            if(index >= input.size()) { return 0; }
            buffer[0] = input[index++];
            return 1;
        });
#endif

    DetailSerial serial(std::move(mockHandler));
    char buffer[4] = {0, };
    serial.open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_READ);
    auto success = serial.read(buffer, 4);
    serial.close();

    // then
    ASSERT_TRUE(success);
    for(uint8_t i = 0; i < 4; ++i)
    {
        ASSERT_EQ(buffer[i], input.at(i));
    }
}

#if STRICT_MODE_ENABLED
TEST(test_Serial, read_without_open)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    // when 
    DetailSerial serial(std::move(mockHandler));
    char buffer[4] = {0, };

    // then
    ASSERT_DEATH({ serial.read(buffer, 4); }, "Serial is not opened");
}
#endif

TEST(test_Serial, readline_success)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    const std::array<char, 4> input{'T', 'E', 'S', 'T'};

    // when
#if defined(WINDOWS)
    EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
    EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(true));
    EXPECT_CALL(*mockHandler, Wrapper_CloseHandle(_)).Times(1);
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
#elif defined(LINUX)
    EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(20));
    EXPECT_CALL(*mockHandler, Wrapper_Read(_, _, _))
        .WillRepeatedly([&input](int32_t fd, char* buffer, size_t size) -> ssize_t {
            static size_t index = 0;
            if(index >= input.size()) { return 0; }
            buffer[0] = input[index++];
            return 1;
        });
#endif

    DetailSerial serial(std::move(mockHandler));
    serial.open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_READ);
    const std::string result(serial.readline(EscapeSequence::NULL_END));
    serial.close();

    // then
    ASSERT_EQ(result, "TEST");
}

#if STRICT_MODE_ENABLED
TEST(test_Serial, readline_without_open)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    const std::array<char, 4> input{'T', 'E', 'S', 'T'};

    // when
    DetailSerial serial(std::move(mockHandler));

    // then
    ASSERT_DEATH({
        std::string result(serial.readline(EscapeSequence::NULL_END));
    }, "Serial is not opened");
}
#endif

TEST(test_Serial, write_success)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    // when
    #if defined(WINDOWS)
        EXPECT_CALL(*mockHandler, Wrapper_CreateFile(_, _, _, _, _, _, _)).WillOnce(Return(nullptr));
        EXPECT_CALL(*mockHandler, Wrapper_GetCommState(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_SetCommState(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_SetCommTimeouts(_, _)).WillOnce(Return(true));
        EXPECT_CALL(*mockHandler, Wrapper_CloseHandle(_)).Times(1);
        EXPECT_CALL(*mockHandler, Wrapper_WriteFile(_, _, _, _, _)).WillOnce(Return(true));
    #elif defined(LINUX)
        EXPECT_CALL(*mockHandler, Wrapper_Open(_, _)).WillOnce(Return(20));
        EXPECT_CALL(*mockHandler, Wrapper_Write(_, _, _)).WillOnce(Return(true));
    #endif

    DetailSerial serial(std::move(mockHandler));

    const auto buffer = "Test";
    serial.open(TEST_SERIAL_PATH, Baudrate::_9600, SERIAL_WRITE);
    const bool rtn = serial.write(buffer, sizeof(buffer));
    serial.close();

    // then
    ASSERT_TRUE(rtn);
}

#if STRICT_MODE_ENABLED
TEST(test_Serial, write_without_open)
{
    using namespace ::testing;

    // given
    auto mockHandler = std::make_unique<MockSerialHandler>();
    testing::Mock::AllowLeak(mockHandler.get());

    // when
    DetailSerial serial(std::move(mockHandler));
    const auto buffer = "Test";

    // then
    ASSERT_DEATH({ serial.write(buffer, sizeof(buffer)); }, "Serial is not opened");
}
#endif
} // namespace common::communication::detail::test
