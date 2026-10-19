#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace NXBasics
{
	// Remaster text boundary. Byte capacities and serialized object layouts stay unchanged.
	void Text_SetLegacyCodePage(std::uint32_t codePage);
	std::uint32_t Text_GetLegacyCodePage();
	bool Utf8_IsValid(const char* text);
	std::uint32_t Utf8_Decode(const char*& text);
	std::string Utf8_Encode(std::uint32_t character);
	std::string Text_ToUtf8(const char* text);
	std::wstring Text_ToWide(const char* text);
	std::string Text_FromWide(const wchar_t* text, std::size_t length);
	std::size_t Utf8_PrefixBytes(const char* text, std::size_t capacity);
	std::size_t Utf8_Previous(const char* text, std::size_t position);
	std::size_t Utf8_Next(const char* text, std::size_t position);
	void Text_Copy(char* destination, std::size_t capacity, const char* text);
}
