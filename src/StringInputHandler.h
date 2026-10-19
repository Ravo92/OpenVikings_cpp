#pragma once

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	class CFont;
}

namespace NC2InGameGuiBaseTextInputElement
{
	class CStringInputHandler
	{
	public:
		CStringInputHandler();

		void Init(std::uint32_t maximumLength, bool continuousEditing, bool startEditing, std::uint32_t activationKey, std::uint8_t activationCharacter);
		void Exit();
		void StartEditing();
		std::uint32_t WorkOnKey(std::uint32_t key, std::uint32_t character);
		std::uint32_t GetCursorPixelPosition(const NXBasics::CFont* font);
		void SetString(const char* text);

		char* GetString() const
		{
			return mActive != 0 ? mText : nullptr;
		}

		std::uint8_t mActive;               // +0x00
		std::byte mPadding01[3];            // +0x01
		std::uint32_t mMaximumLength;       // +0x04
		std::uint8_t mContinuousEditing;    // +0x08
		std::byte mPadding09[3];            // +0x09
		std::uint32_t mActivationKey;       // +0x0C
		std::uint8_t mActivationCharacter;  // +0x10
		std::uint8_t mEditing;              // +0x11
		std::byte mPadding12[2];            // +0x12
		std::uint32_t mCursorPosition;      // +0x14
		char* mText;                        // +0x18
	};

	static_assert(sizeof(void*) == 4);
	static_assert(sizeof(CStringInputHandler) == 28);
	static_assert(offsetof(CStringInputHandler, mMaximumLength) == 0x04);
	static_assert(offsetof(CStringInputHandler, mContinuousEditing) == 0x08);
	static_assert(offsetof(CStringInputHandler, mActivationKey) == 0x0C);
	static_assert(offsetof(CStringInputHandler, mActivationCharacter) == 0x10);
	static_assert(offsetof(CStringInputHandler, mEditing) == 0x11);
	static_assert(offsetof(CStringInputHandler, mCursorPosition) == 0x14);
	static_assert(offsetof(CStringInputHandler, mText) == 0x18);
}
