#include "Config.h"

#include <cerrno>
#include <cctype>
#include <cstring>
#include <fstream>

namespace chdv::sms_daemon {
namespace {

std::string Trim(const std::string& value) {
    size_t first = 0;
    while(first < value.size() && std::isspace(static_cast<unsigned char>(value[first])))
        ++first;
    size_t last = value.size();
    while(last > first && std::isspace(static_cast<unsigned char>(value[last - 1])))
        --last;
    return value.substr(first, last - first);
}

bool ParseBool(const std::string& value, bool& result) {
    std::string normalized;
    normalized.reserve(value.size());
    for(char ch : value)
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));

    if(normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        result = true;
        return true;
    }
    if(normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        result = false;
        return true;
    }
    return false;
}

} // namespace

bool LoadSmsDaemonConfig(const std::string& path, SmsDaemonConfig& config, std::string& error, bool required) {
    std::ifstream input(path);
    if(!input) {
        if(!required)
            return true;
        error = "Can not open config file " + path + ": " + std::strerror(errno);
        return false;
    }

    std::string line;
    int lineNumber = 0;
    while(std::getline(input, line)) {
        ++lineNumber;
        line = Trim(line);
        if(line.empty() || line[0] == '#' || line[0] == ';')
            continue;

        const auto separator = line.find('=');
        if(separator == std::string::npos) {
            error = path + ":" + std::to_string(lineNumber) + ": expected key=value";
            return false;
        }

        const std::string key = Trim(line.substr(0, separator));
        const std::string value = Trim(line.substr(separator + 1));
        if(key == "device")
            config.device = value;
        else if(key == "job_dir")
            config.jobDir = value;
        else if(key == "sms_dir") {
            config.smsDir = value;
            config.smsDirConfigured = true;
        }
        else if(key == "ussd_dir") {
            config.ussdDir = value;
            config.ussdDirConfigured = true;
        }
        else if(key == "log_file")
            config.logFile = value;
        else if(key == "sms_hook")
            config.smsHooks.push_back(value);
        else if(key == "debug") {
            if(!ParseBool(value, config.debug)) {
                error = path + ":" + std::to_string(lineNumber) + ": invalid boolean value for debug: " + value;
                return false;
            }
        }
        else {
            error = path + ":" + std::to_string(lineNumber) + ": unknown setting " + key;
            return false;
        }
    }

    if(config.device.empty()) {
        error = path + ": device must not be empty";
        return false;
    }
    if(config.jobDir.empty()) {
        error = path + ": job_dir must not be empty";
        return false;
    }
    if(config.smsDirConfigured && config.smsDir.empty()) {
        error = path + ": sms_dir must not be empty";
        return false;
    }
    if(config.ussdDirConfigured && config.ussdDir.empty()) {
        error = path + ": ussd_dir must not be empty";
        return false;
    }
    if(config.logFile.empty()) {
        error = path + ": log_file must not be empty";
        return false;
    }
    for(const auto& smsHook : config.smsHooks) {
        if(smsHook.empty()) {
            error = path + ": sms_hook must not be empty";
            return false;
        }
    }
    return true;
}

} // namespace chdv::sms_daemon
