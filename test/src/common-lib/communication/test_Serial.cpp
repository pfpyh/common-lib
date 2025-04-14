#ifdef TEMPORARY_DISABLED

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "common/communication/Serial.hpp"

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
            WithArg<1>([](void* buffer) { *static_cast<char*>(buffer) = '\n'; }),
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
    const std::string result(serial.readline());

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

#endif