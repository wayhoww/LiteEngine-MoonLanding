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

template <char c>
constexpr uint32_t getKeyCode() {
	constexpr UINT KEYCODE_A = 0x41;
	if constexpr (c >= 'a' && c <= 'z') {
		return KEYCODE_A + (c - 'a');
	} else if constexpr (c >= 'A' && c <= 'Z') {
		return KEYCODE_A + (c - 'A');
	} else {
		static_assert(false);
	}
}

uint32_t getKeyCode(char c);
constexpr float PI = 3.141592653589793115997963468544f;

}