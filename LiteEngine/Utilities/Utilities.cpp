// C++ 暂未提供新方案，且保证新方案出来前不会移除 codecvt
// 也可以用 Windows 提供的 MultiByteToWideChar 函数转换
#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "Utilities.h"

#include <vector>
#include <string>
#include <iterator>
#include <locale>
#include <codecvt>


namespace LiteEngine {

	std::vector<uint8_t> loadBinaryFromFile(const std::wstring& path) {
		FILE* file = nullptr;
		auto err = _wfopen_s(&file, path.c_str(), L"rb");
		if (err || file == nullptr) { throw std::exception("cannot open file"); }
		constexpr size_t unitLength = 4096;
		std::vector<uint8_t> out;
		std::vector<uint8_t> buffer(unitLength);
		size_t readSize = 0;
		do {
			readSize = fread_s(buffer.data(), unitLength, 1, unitLength, file);
			std::copy_n(buffer.begin(), readSize, std::back_insert_iterator<std::vector<uint8_t>>(out));
		} while (readSize > 0);
		return out;
	}

	std::string wstringToString(const std::wstring& wstr) {
		std::wstring_convert< std::codecvt_utf8<wchar_t>, wchar_t> converter;
		return converter.to_bytes(wstr);
	}

	std::wstring stringToWstring(const std::string& wstr) {
		std::wstring_convert< std::codecvt_utf8<wchar_t>, wchar_t> converter;
		return converter.from_bytes(wstr);
	}



	uint32_t getKeyCode(char c) {
		UINT KEYCODE_A = 0x41;
		if (c >= 'a' && c <= 'z') {
			return KEYCODE_A + (c - 'a');
		} else if (c >= 'A' && c <= 'Z') {
			return KEYCODE_A + (c - 'A');
		} else {
			throw std::exception("keycode is invalid");
		}
	}

}