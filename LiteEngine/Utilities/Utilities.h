#pragma once

#include <vector>
#include <string>
#include <Windows.h>

namespace LiteEngine {

std::vector<uint8_t> loadBinaryFromFile(const std::wstring& path);

std::string wstringToString(const std::wstring& wstr);

std::wstring stringToWstring(const std::string& wstr);

enum class LogLevel {
	DEBUG,
	INFO,
	WARNING,
	EXCEPTION
};

static inline void log(LogLevel level, const std::string& s) {
	OutputDebugStringA(s.c_str());
}

static inline void log(LogLevel level, const std::wstring& s) {
	OutputDebugStringW(s.c_str());
}

}