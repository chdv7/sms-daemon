#include <iostream>
#include <gtest/gtest.h>

// твой заголовок, путь подстрой
#include "../src/PduSMS.h"

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
