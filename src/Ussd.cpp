#include "Ussd.h"

#include <cctype>
#include <cstdlib>
#include <utility>

#include "ut.h"

namespace chdv::sms_daemon {
namespace {

void SkipSpaces(const char*& ptr) {
    while(*ptr && std::isspace(static_cast<unsigned char>(*ptr)))
        ++ptr;
}

bool ParseInt(const char*& ptr, int& value) {
    SkipSpaces(ptr);
    char* end = nullptr;
    long parsed = std::strtol(ptr, &end, 10);
    if(end == ptr)
        return false;
    value = static_cast<int>(parsed);
    ptr = end;
    return true;
}

bool IsHexString(const std::string& value) {
    if(value.empty() || value.size() % 4 != 0)
        return false;
    for(char ch : value)
        if(!std::isxdigit(static_cast<unsigned char>(ch)))
            return false;
    return true;
}

std::string DecodeUcs2(const std::string& value) {
    std::wstring decoded;
    decoded.reserve(value.size() / 4);
    for(size_t i = 0; i < value.size(); i += 4) {
        wchar_t ch = static_cast<wchar_t>(std::strtoul(value.substr(i, 4).c_str(), nullptr, 16));
        decoded += ch;
    }
    return toUTF8(decoded);
}

} // namespace

bool ParseUssdResponse(const std::string& line, ReceivedUssd& response) {
    const auto marker = line.find("+CUSD:");
    if(marker == std::string::npos)
        return false;

    const char* ptr = line.c_str() + marker + 6;
    ReceivedUssd parsed;
    parsed.raw = line;
    if(!ParseInt(ptr, parsed.mode))
        return false;

    SkipSpaces(ptr);
    if(*ptr == ',') {
        ++ptr;
        SkipSpaces(ptr);
        if(*ptr == '"') {
            ++ptr;
            while(*ptr && *ptr != '"') {
                if(*ptr == '\\' && ptr[1])
                    ++ptr;
                parsed.text += *ptr++;
            }
            if(*ptr != '"')
                return false;
            ++ptr;
        }

        SkipSpaces(ptr);
        if(*ptr == ',') {
            ++ptr;
            if(!ParseInt(ptr, parsed.dcs))
                return false;
        }
    }

    // 0x48 (72) is the commonly used UCS2 data coding scheme for USSD.
    if(parsed.dcs == 72 && IsHexString(parsed.text))
        parsed.text = DecodeUcs2(parsed.text);

    response = std::move(parsed);
    return true;
}

} // namespace chdv::sms_daemon
