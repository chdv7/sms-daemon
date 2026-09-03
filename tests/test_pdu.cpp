#include <iostream>
#include <gtest/gtest.h>

// твой заголовок, путь подстрой
#include "../src/PduSMS.h"
#include "../src/RecvSMS.h"
namespace chdv::sms_daemon::test {
TEST(PduSmsTest, SimpleSMS_7bit){
    auto result = COutPduSms ("+70123456789", "Test").ParseText();
    EXPECT_EQ(1,result.size());
//    std::cout << result[0] << std::endl;
    EXPECT_EQ(result[0], "0001000B910721436587F9000004D4F29C0E");
}

// TEST(PduSmsTest, SimpleSMS_7bitAll){
//     std::string smsTest =
//     u8"@£$¥èéùìòÇØøÅåΔ_ΦΓΛΩΠΨΣΘΞÆæßÉ "
//     u8"!\"#¤%&'()*+,-./0123456789:;<=>?"
//     u8"¡ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÑÜ§¿"
//     u8"abcdefghijklmnopqrstuvwxyzäöñüà"
//     u8"^{}\\[~]|€";
//     COutPduSms pdu ("+70123456789", smsTest.c_str(), false);
//     auto result = pdu.ParseText();
//     EXPECT_EQ(1,result.size());
//     std::cout << result[0] << std::endl;
//     EXPECT_EQ(result[0], "0001000B910721436587F9000004D4F29C0E");
// }


TEST(PduSmsTest, IncomingSmsWith16BitConcatUdh){
    CRecvSMSPart part;
    ASSERT_EQ(0, part.ProcessRecvPDU("07919712690020F6400ED0CDF2399C749BDF0008629020320163218B060804BE3602010412043004480020043A043E043400200434043B044F0020043004320442043E0440043804370430044604380438003A0020003800360035003100340034002E00200412043204350434043804420435002004350433043E0020043D043000200441044204400430043D0438044604350020043F043E0434044204320435044004360434"));
    EXPECT_EQ(0xBE36, part.m_nRefNr);
    EXPECT_EQ(2, part.m_nParts);
    EXPECT_EQ(1, part.m_nPartNo);
    EXPECT_EQ(std::wstring(L"MegaInfo"), part.m_From);
    EXPECT_EQ("2026-09-02 23:10:36 UTC+03:00", part.m_TimeStamp);
    EXPECT_NE(std::wstring::npos, part.m_sText.find(L"Ваш код"));
}

TEST(PduSmsTest, IncomingSmsTimestampUsesIsoDate){
    CRecvSMSPart part;
    ASSERT_EQ(0, part.ProcessRecvPDU("00040B910721436587F900006260922153942104D4F29C0E"));
    EXPECT_EQ("2026-06-29 12:35:49 UTC+03:00", part.m_TimeStamp);
}

TEST(PduSmsTest, SimpleSMS_Utf8){
    COutPduSms pdu("+70123456789", "Test");
    pdu.ForceUnicode();
    auto result = pdu.ParseText();
    EXPECT_EQ(1,result.size());
  //  std::cout << result[0] << std::endl;
    EXPECT_EQ(result[0], "0001000B910721436587F90008080054006500730074");
}

TEST(PduSmsTest, Multipart2_SMS_7bit){
    COutPduSms pdu ("+70123456789", "Test"
        "0123456789" // 1
        "0123456789" // 2
        "0123456789" // 3
        "0123456789" // 4
        "0123456789" // 5
        "0123456789" // 6
        "0123456789" // 7
        "0123456789" // 8
        "0123456789" // 9
        "0123456789" // 10
        "0123456789" // 11
        "0123456789" // 12
        "0123456789" // 13
        "0123456789" // 14
        "0123456789" // 15
        "0123456789" // 16 // 164 characters
        "End"
    );
    pdu.SetRefNo (0x78);
    auto result = pdu.ParseText();
    EXPECT_EQ(2,result.size());
//    for (auto& it : result)
//        std::cout  << std::endl << it << std::endl;

    EXPECT_EQ(result[0], "0041000B910721436587F90000A0050003780201A8E5391D1693CD6835DB0D9783C564335ACD76C3"
                         "E56031D98C56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C"
                         "56B3DD7039584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD7039"
                         "584C36A3D56C375C0E1693CD6835DB0D9783C564335ACD76C3E56031D98C56B3DD70");
    EXPECT_EQ(result[1], "0041000B910721436587F900001505000378020272B0986C46ABD96EB85CD14D06");
}
} // namespace chdv::sms_daemon::test