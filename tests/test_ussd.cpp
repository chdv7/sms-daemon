#include <gtest/gtest.h>

#include "../src/Ussd.h"

namespace chdv::sms_daemon::test {

TEST(UssdTest, ParsesPlainTextResponse) {
    ReceivedUssd response;
    ASSERT_TRUE(ParseUssdResponse("+CUSD: 0,\"Balance: 10.50\",15", response));
    EXPECT_EQ(response.mode, 0);
    EXPECT_EQ(response.dcs, 15);
    EXPECT_EQ(response.text, "Balance: 10.50");
}

TEST(UssdTest, DecodesUcs2Response) {
    ReceivedUssd response;
    ASSERT_TRUE(ParseUssdResponse("+CUSD: 0,\"04110430043B0430043D0441\",72", response));
    EXPECT_EQ(response.text, u8"Баланс");
}

TEST(UssdTest, ParsesResponseInsideAtLog) {
    ReceivedUssd response;
    ASSERT_TRUE(ParseUssdResponse("\r\n+CUSD: 2,\"Done\",15\r\nOK\r\n", response));
    EXPECT_EQ(response.mode, 2);
    EXPECT_EQ(response.text, "Done");
}

TEST(UssdTest, RejectsNonUssdLine) {
    ReceivedUssd response;
    EXPECT_FALSE(ParseUssdResponse("OK", response));
}

} // namespace chdv::sms_daemon::test
