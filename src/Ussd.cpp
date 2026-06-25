#include "Ussd.h"

#include <cctype>
#include <cstdlib>
#include <string_view>
#include <utility>
#include <vector>

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

int HexValue(char ch) {
    if(ch >= '0' && ch <= '9')
        return ch - '0';
    if(ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

bool HexToBytes(const std::string& hex, std::vector<unsigned char>& bytes) {
    if(hex.size() % 2 != 0)
        return false;
    bytes.clear();
    bytes.reserve(hex.size() / 2);
    for(size_t i = 0; i < hex.size(); i += 2) {
        int hi = HexValue(hex[i]);
        int lo = HexValue(hex[i + 1]);
        if(hi < 0 || lo < 0)
            return false;
        bytes.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return true;
}

char DecodeGsm7Basic(unsigned int code) {
    if(code == 0x0a)
        return '\n';
    if(code == 0x0d)
        return '\r';
    if(code == 0x11)
        return '_';
    if(code == 0x02)
        return '$';
    if(code >= 0x20 && code <= 0x3f)
        return static_cast<char>(code);
    if(code >= 0x41 && code <= 0x5a)
        return static_cast<char>(code);
    if(code >= 0x61 && code <= 0x7a)
        return static_cast<char>(code);
    return '?';
}

std::string DecodeGsm7Packed(const std::string& hex) {
    std::vector<unsigned char> bytes;
    if(!HexToBytes(hex, bytes))
        return std::string();

    std::string out;
    const size_t septets = bytes.size() * 8 / 7;
    out.reserve(septets);
    for(size_t i = 0; i < septets; ++i) {
        const size_t bit = i * 7;
        const size_t byteIndex = bit / 8;
        const unsigned int shift = bit % 8;
        unsigned int value = bytes[byteIndex] >> shift;
        if(byteIndex + 1 < bytes.size())
            value |= static_cast<unsigned int>(bytes[byteIndex + 1]) << (8 - shift);
        out += DecodeGsm7Basic(value & 0x7f);
    }
    return out;
}

} // namespace

std::string DecodeUssdRequest(std::string_view command) {
    std::string value(command);
    if(!value.empty() && value.front() == 'U')
        value.erase(value.begin());
    if(value.size() < 3 || value.front() != '"')
        return std::string();

    const size_t endQuote = value.find('"', 1);
    if(endQuote == std::string::npos)
        return std::string();
    const std::string payload = value.substr(1, endQuote - 1);

    int dcs = -1;
    const size_t comma = value.find(',', endQuote + 1);
    if(comma != std::string::npos)
        dcs = std::atoi(value.c_str() + comma + 1);

    if(dcs == 15)
        return DecodeGsm7Packed(payload);
    if(dcs == 72 && IsHexString(payload))
        return DecodeUcs2(payload);
    return std::string();
}

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
