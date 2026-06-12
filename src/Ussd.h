#pragma once

#include <ctime>
#include <string>

namespace chdv::sms_daemon {

struct ReceivedUssd {
    int mode{};
    int dcs{-1};
    std::string text;
    std::string raw;
    std::string interface;
    time_t receiveTime{time(nullptr)};
};

bool ParseUssdResponse(const std::string& line, ReceivedUssd& response);

} // namespace chdv::sms_daemon
