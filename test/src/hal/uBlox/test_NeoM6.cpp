#include <gtest/gtest.h>

#include "hal/gnss/NeoM6.hpp"

namespace common::hal::test
{
TEST(test_NeoM6, GPGGA)
{
    // given
    constexpr auto GPGGA = "$GPGGA,114455.532,3735.0079,N,12701.6446,E,1,03,7.9,48.8,M,19.6,M,0.0,0000*48";

    // when
    auto position = NeoM6::parse(GPGGA);

    // then
    ASSERT_TRUE((position._type & NeoM6::SupportType::GPGGA) == NeoM6::SupportType::GPGGA);
    ASSERT_TRUE(position._latitude >= 37.58346);
    ASSERT_TRUE(position._longitude >= 127.02741);
}
} // namespace common::hal::test