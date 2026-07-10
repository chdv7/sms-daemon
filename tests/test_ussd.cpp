#include <gtest/gtest.h>

#include "../src/gen-xml.hpp"
#include "../src/Ussd.h"
#include "../src/RecvSMS.h"

#include <memory>

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

TEST(UssdTest, DecodesGsm7HexResponseWithDcs1) {
    ReceivedUssd response;
    ASSERT_TRUE(ParseUssdResponse("+CUSD: 0,\"D3B2DC9E1E9741EE371D14B687D3ECB0985D7601\",1", response));
    EXPECT_EQ(response.dcs, 1);
    EXPECT_EQ(response.text, "Service not available.");
}

TEST(UssdTest, ParsesResponseInsideAtLog) {
    ReceivedUssd response;
    ASSERT_TRUE(ParseUssdResponse("\r\n+CUSD: 2,\"Done\",15\r\nOK\r\n", response));
    EXPECT_EQ(response.mode, 2);
    EXPECT_EQ(response.text, "Done");
}

TEST(UssdTest, DecodesPackedRequest) {
    EXPECT_EQ(DecodeUssdRequest("\"AA180C3602\",15"), "*100#");
}

TEST(UssdTest, ParsesCallForwardingResponse) {
    ReceivedUssd response;
    ASSERT_TRUE(ParseUssdResponse("\r\n+CCFC: 0,1\r\nOK\r\n", response));
    EXPECT_EQ(response.mode, 0);
    EXPECT_NE(response.text.find("+CCFC: 0,1"), std::string::npos);
}

TEST(UssdTest, RejectsNonUssdLine) {
    ReceivedUssd response;
    EXPECT_FALSE(ParseUssdResponse("OK", response));
}

TEST(UssdTest, XmlContainsRequestAndSendTime) {
    ReceivedUssd response;
    response.mode = 0;
    response.dcs = 72;
    response.request = "*100#";
    response.sendTime = 1700000000;
    response.receiveTime = 1700000005;
    response.interface = "/dev/ttyUSB2";
    response.imsi = "250020000000001";
    response.imei = "352964812937045";
    response.text = "Balance";

    XMLNode xml = GenXML(response, false);
    EXPECT_STREQ(xml.getAttribute("Request"), "*100#");
    EXPECT_NE(xml.getAttribute("SendTime"), nullptr);
    EXPECT_STREQ(xml.getAttribute("Interface"), "/dev/ttyUSB2");
    EXPECT_STREQ(xml.getAttribute("IMSI"), "250020000000001");
    EXPECT_STREQ(xml.getAttribute("IMEI"), "352964812937045");
}

TEST(SmsXmlTest, DebugFlagControlsParts) {
    auto part = std::make_unique<CRecvSMSPart>();
    part->m_From = L"+100";
    part->m_SMSC = L"+200";
    part->m_sText = L"hello";
    part->m_TimeStamp = "24/06/25,10:20:30+00";
    part->m_nPartNo = 1;
    part->m_nParts = 1;
    part->m_nRefNr = 7;
    part->m_RawText = "raw-pdu";

    ReceivedSMS sms(std::move(part), "/dev/ttyUSB2");
    sms.m_IMSI = "250020000000001";
    sms.m_IMEI = "352964812937045";

    XMLNode noDebug = GenXML(sms, false, false);
    EXPECT_TRUE(noDebug.getChildNode("Part").isEmpty());
    EXPECT_STREQ(noDebug.getAttribute("IMSI"), "250020000000001");
    EXPECT_STREQ(noDebug.getAttribute("IMEI"), "352964812937045");

    XMLNode debug = GenXML(sms, true, true);
    XMLNode xPart = debug.getChildNode("Part");
    ASSERT_FALSE(xPart.isEmpty());
    EXPECT_STREQ(xPart.getAttribute("raw"), "raw-pdu");
}

} // namespace chdv::sms_daemon::test
