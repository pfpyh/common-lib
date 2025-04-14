#if defined(LINUX)

#include <gtest/gtest.h>

#include "common/communication/Message.hpp"

namespace common::test
{
TEST(test_Message, create)
{
    // given
    auto message = Message<std::string>::create("testFile", 65);

    // when

    // then
}
} // namespace common::test

#endif
