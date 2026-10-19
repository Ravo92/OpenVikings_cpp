#include "XTextEncoding.h"

#include <Windows.h>
#include <cstring>
#include <limits>

namespace NXBasics
{
	namespace { std::uint32_t legacyCodePage = 1252; }

	void Text_SetLegacyCodePage(std::uint32_t codePage) { legacyCodePage = codePage; }
	std::uint32_t Text_GetLegacyCodePage() { return legacyCodePage; }

	std::uint32_t Utf8_Decode(const char*& text)
	{
		const auto first = static_cast<unsigned char>(*text);
		if (first == 0) return 0;
		++text;
		if (first < 0x80) return first;
		const unsigned count = first >= 0xC2 && first <= 0xDF ? 1 : first >= 0xE0 && first <= 0xEF ? 2 : first >= 0xF0 && first <= 0xF4 ? 3 : 0;
		if (count == 0) return 0xFFFD;
		std::uint32_t value = first & (0x7Fu >> count);
		for (unsigned index = 0; index < count; ++index)
		{
			const auto byte = static_cast<unsigned char>(text[index]);
			if (byte < 0x80 || byte > 0xBF) return 0xFFFD;
			value = (value << 6) | (byte & 0x3F);
		}
		if ((count == 1 && value < 0x80) || (count == 2 && value < 0x800) || (count == 3 && value < 0x10000) || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) return 0xFFFD;
		text += count;
		return value;
	}

	bool Utf8_IsValid(const char* text)
	{
		if (text == nullptr || *text == 0) return true;
		return MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0) != 0;
	}

	std::string Text_FromWide(const wchar_t* text, std::size_t length)
	{
		if (text == nullptr || length == 0 || length > INT_MAX) return {};
		const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, static_cast<int>(length), nullptr, 0, nullptr, nullptr);
		if (count == 0) return {};
		std::string result(count, '\0');
		WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, static_cast<int>(length), result.data(), count, nullptr, nullptr);
		return result;
	}

	std::wstring Text_ToWide(const char* text)
	{
		if (text == nullptr || *text == 0) return {};
		const UINT page = Utf8_IsValid(text) ? CP_UTF8 : legacyCodePage;
		const int count = MultiByteToWideChar(page, 0, text, -1, nullptr, 0);
		if (count == 0) return {};
		std::wstring result(count, L'\0');
		MultiByteToWideChar(page, 0, text, -1, result.data(), count);
		result.pop_back();
		if (!result.empty() && result.front() == 0xFEFF) result.erase(0, 1);
		return result;
	}

	std::string Text_ToUtf8(const char* text)
	{
		if (text == nullptr) return {};
		if (Utf8_IsValid(text))
		{
			if (std::strncmp(text, "\xEF\xBB\xBF", 3) == 0) text += 3;
			return text;
		}
		const auto wide = Text_ToWide(text);
		return Text_FromWide(wide.data(), wide.size());
	}

	std::string Utf8_Encode(std::uint32_t character)
	{
		if (character == 0 || character > 0x10FFFF || (character >= 0xD800 && character <= 0xDFFF)) return {};
		wchar_t wide[2];
		if (character < 0x10000) { wide[0] = static_cast<wchar_t>(character); return Text_FromWide(wide, 1); }
		character -= 0x10000;
		wide[0] = static_cast<wchar_t>(0xD800 + (character >> 10));
		wide[1] = static_cast<wchar_t>(0xDC00 + (character & 0x3FF));
		return Text_FromWide(wide, 2);
	}

	std::size_t Utf8_PrefixBytes(const char* text, std::size_t capacity)
	{
		const char* cursor = text;
		std::size_t result = 0;
		while (*cursor != 0)
		{
			Utf8_Decode(cursor);
			const auto length = static_cast<std::size_t>(cursor - text);
			if (length > capacity) break;
			result = length;
		}
		return result;
	}

	std::size_t Utf8_Previous(const char* text, std::size_t position)
	{
		if (position == 0) return 0;
		--position;
		while (position != 0 && (static_cast<unsigned char>(text[position]) & 0xC0) == 0x80) --position;
		return position;
	}

	std::size_t Utf8_Next(const char* text, std::size_t position)
	{
		const char* cursor = text + position;
		Utf8_Decode(cursor);
		return static_cast<std::size_t>(cursor - text);
	}

	void Text_Copy(char* destination, std::size_t capacity, const char* text)
	{
		if (destination == nullptr || capacity == 0) return;
		const auto normalized = Text_ToUtf8(text);
		const auto count = Utf8_PrefixBytes(normalized.c_str(), capacity - 1);
		std::memmove(destination, normalized.data(), count);
		std::memset(destination + count, 0, capacity - count);
	}
}
