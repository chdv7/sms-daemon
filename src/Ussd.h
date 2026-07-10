#pragma once

#include <ctime>
#include <string>
#include <string_view>

namespace chdv::sms_daemon {

struct ReceivedUssd {
    std::string request;
    time_t sendTime{};
    int mode{};
    int dcs{-1};
    std::string text;
    std::string raw;
    std::string interface;
    std::string imsi;
    std::string imei;
    time_t receiveTime{time(nullptr)};
};

bool ParseUssdResponse(const std::string& line, ReceivedUssd& response);
std::string DecodeUssdRequest(std::string_view command);

} // namespace chdv::sms_daemon
