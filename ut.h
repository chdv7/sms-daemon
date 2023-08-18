#pragma once
#include "xmlParser.h"
#include <inttypes.h>
#include <unistd.h>


const char* GetXMLStr (XMLNode& n, const char* name, const char*def);
int GetXMLInt (XMLNode& n, const char* name, int def);
int UTF8ToUnicode (wchar_t* d, const char* src, int sz);
int UnicodeToUTF8 (char* d, const wchar_t* src, size_t sz);

int64_t GetTickCnt64 ();
int32_t GetTickCnt32 ();

#define GetTickCount() GetTickCnt32()
#define Sleep(a) usleep (((useconds_t)a)*1000)
