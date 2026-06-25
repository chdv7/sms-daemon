#pragma once

#include <string>

void SetLogPath(const std::string& path);
void Log(const char* text);
void LogError(int code, const char* text);
