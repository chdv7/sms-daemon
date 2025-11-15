#pragma once
#include <inttypes.h>
#include <unistd.h>

#include <string>

#include "xmlParser.h"

const char* GetXMLStr(XMLNode& n, const char* name, const char* def);
int GetXMLInt(XMLNode& n, const char* name, int def);
int UTF8ToUnicode(wchar_t* d, const char* src, int sz);

std::string toLocalTime(time_t);

template<class T>
std::string toUTF8(const T& srcStr) {
    static_assert(std::is_same_v<T, std::wstring> || std::is_same_v<T, std::wstring_view>);
    std::string dst;
    dst.reserve(srcStr.size() * 2);
    for(uint16_t srcChr : srcStr) {
        size_t code_length = 1; // src    7 bit - code 1
        if(srcChr & ~0x007F)
            code_length++; // src >= 8 bit - code 2
        if(srcChr & ~0x07FF)
            code_length++; // src >=12 bit - code 3
        switch(code_length) {
        case 1:                               // srcChr ( 7 bit) 000000000zzzzzzz
            dst += static_cast<char>(srcChr); // 0zzzzzzz
            break;

        case 2:                                                   // srcChr (11 bit) 00000yyyyyzzzzzz
            dst += static_cast<char>((srcChr >> 6) | 0x00c0);     // 110yyyyy
            dst += static_cast<char>((srcChr & 0x003f) | 0x0080); // 10zzzzzz
            break;

        case 3:                                                        // srcChr (16 bit) xxxxyyyyyyzzzzzz
            dst += static_cast<char>(srcChr >> 12 | 0x00e0);           // 1110xxxx
            dst += static_cast<char>((srcChr >> 6 & 0x003f) | 0x0080); // 10yyyyyy
            dst += static_cast<char>((srcChr & 0x003f) | 0x0080);      // 10zzzzzz
            break;
        }
    }
    return dst;
}

inline int Sleep(uint32_t delay) {
    return usleep(static_cast<useconds_t>(delay) * 1000);
}
