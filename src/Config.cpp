#include "Config.h"

#include <cctype>
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

} // namespace

bool LoadSmsDaemonConfig(const std::string& path, SmsDaemonConfig& config, std::string& error) {
    std::ifstream input(path);
    if(!input) {
        error = "Can not open config file " + path;
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
        else {
            error = path + ":" + std::to_string(lineNumber) + ": unknown setting " + key;
            return false;
        }
    }

    if(config.device.empty()) {
        error = path + ": device must not be empty";
        return false;
    }
    return true;
}

} // namespace chdv::sms_daemon
