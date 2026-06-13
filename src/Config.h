#pragma once

#include <string>

namespace chdv::sms_daemon {

struct SmsDaemonConfig {
    std::string device;
};

bool LoadSmsDaemonConfig(const std::string& path, SmsDaemonConfig& config, std::string& error);

} // namespace chdv::sms_daemon
