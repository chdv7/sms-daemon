#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "sms-daemon-config.h"

namespace chdv::sms_daemon {

struct SmsDaemonConfig {
    std::string device{DEVICE};
    std::string jobDir{OUT_SMS_DIR};
    std::string smsDir{IN_SMS_XML_DIR};
    std::string ussdDir{IN_SMS_XML_DIR};
    std::string logFile{SMS_LOG_FILE};
    std::vector<std::string> smsHooks{};
    bool smsDirConfigured{false};
    bool ussdDirConfigured{false};
    bool debug{false};
};

bool LoadSmsDaemonConfig(const std::string& path, SmsDaemonConfig& config, std::string& error, bool required = true);
inline std::ostream& operator << (std::ostream&os, const SmsDaemonConfig& config){
    os << "device: " << config.device << std::endl <<
          "jobDir: " << config.jobDir << std::endl <<
          "smsDir: " << (config.smsDirConfigured ? config.smsDir : "disabled") << std::endl <<
          "ussdDir: " << (config.ussdDirConfigured ? config.ussdDir : "disabled") << std::endl <<
          "logFile: " << config.logFile << std::endl <<
          "smsHooks: " << config.smsHooks.size() << std::endl <<
          "debug: " << (config.debug ? "true" : "false") << std::endl;
    return os;
}
} // namespace chdv::sms_daemon
