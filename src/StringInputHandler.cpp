#include "StringInputHandler.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XTextEncoding.h"
#include "XUnicodeFont.h"

#include <cstdint>
#include <cstring>

// Remaster accepts printable Unicode scalars instead of whitelist 00507468.

namespace NC2InGameGuiBaseTextInputElement
{
	// Original function: FUN_004a3a8e
	void CStringInputHandler::Init(std::uint32_t maximumLength, bool continuousEditing, bool startEditing, std::uint32_t activationKey, std::uint8_t activationCharacter)
	{
		NXBasics::Runtime_Free(mText);

		mContinuousEditing = static_cast<std::uint8_t>(continuousEditing);
		mActivationKey = activationKey;
		mActivationCharacter = activationCharacter;
		mMaximumLength = maximumLength;
		mText = nullptr;
		mActive = 1;

		mText = maximumLength <= UINT32_MAX - 2u ? static_cast<char*>(NXBasics::Runtime_AllocateWithNewHandler(static_cast<std::size_t>(maximumLength) + 2u, 1)) : nullptr;
		mCursorPosition = 0;
		mEditing = 0;

		if (mText == nullptr)
		{
			mActive = 0;

			return;
		}

		mText[0] = '\0';

		if ((startEditing || continuousEditing) && mActive != 0)
		{
			mEditing = 1;
			mCursorPosition = 0;
			mText[0] = '\0';
		}
	}

	// Original function: FUN_004a3ae8
	void CStringInputHandler::Exit()
	{
		NXBasics::Runtime_Free(mText);

		mText = nullptr;
		mActive = 0;
	}

	// Original function: FUN_004a3afd
	void CStringInputHandler::StartEditing()
	{
		if (mActive != 0)
		{
			mEditing = 1;
			mCursorPosition = 0;
			mText[0] = '\0';
		}
	}

	// Original function: FUN_004a3b10
	std::uint32_t CStringInputHandler::WorkOnKey(std::uint32_t key, std::uint32_t character)
	{
		if (mActive == 0)
		{
			return 0;
		}

		if (mEditing == 0)
		{
			const bool activationKeyMatches = mActivationKey != 0 && key == mActivationKey;
			const bool activationCharacterMatches = mActivationCharacter != 0 && character == mActivationCharacter;

			if (!activationKeyMatches && !activationCharacterMatches)
			{
				return 1;
			}

			mCursorPosition = 0;
			mEditing = 1;
			mText[0] = '\0';

			return 2;
		}

		const auto length = static_cast<std::uint32_t>(std::strlen(mText));

		if (mCursorPosition > length)
			mCursorPosition = length;

		switch (key)
		{
			case SwitchIds::VirtualKey::Backspace:
			{
				// Backspace.

				if (length != 0 && mCursorPosition != 0)
				{
					const std::uint32_t moveBytes = length - mCursorPosition + 1u;

					const auto previous = static_cast<std::uint32_t>(NXBasics::Utf8_Previous(mText, mCursorPosition));
					std::memmove(mText + previous, mText + mCursorPosition, moveBytes);
					mCursorPosition = previous;
				}

				return 2;
			}

			case SwitchIds::VirtualKey::Return:
			{
				// Return.

				if (mContinuousEditing == 0)
				{
					mEditing = 0;
				}

				return 3;
			}

			case SwitchIds::VirtualKey::Escape:
			{
				// Escape does not reset the cursor.

				if (mContinuousEditing != 0)
				{
					mText[0] = '\0';
					mCursorPosition = 0;
				}
				else
				{
					mEditing = 0;
				}

				return 4;
			}

			case SwitchIds::VirtualKey::End:
				// End.
				mCursorPosition = length;
				return 2;

			case SwitchIds::VirtualKey::Home:
				// Home.
				mCursorPosition = 0;
				return 2;

			case SwitchIds::VirtualKey::Left:
				// Left.

				if (length != 0 && mCursorPosition != 0)
				{
					mCursorPosition = static_cast<std::uint32_t>(NXBasics::Utf8_Previous(mText, mCursorPosition));
				}

				return 2;

			case SwitchIds::VirtualKey::Right:
				// Right.

				if (mCursorPosition < length)
				{
					mCursorPosition = static_cast<std::uint32_t>(NXBasics::Utf8_Next(mText, mCursorPosition));
				}

				return 2;

			case SwitchIds::VirtualKey::Delete:
			{
				if (mCursorPosition < length)
				{
					const auto next = static_cast<std::uint32_t>(NXBasics::Utf8_Next(mText, mCursorPosition));
					std::memmove(mText + mCursorPosition, mText + next, length - next + 1u);
				}

				return 2;
			}

			default:
				break;
		}

		// Remaster: a zero-capacity input accepts no inserted characters.

		if (length >= mMaximumLength)
		{
			return 2;
		}

		if (mCursorPosition > length)
		{
			mCursorPosition = length;
		}

		if (character < 0x20 || character == 0x7F || (character >= 0x80 && character <= 0x9F))
		{
			return 2;
		}

		const auto encoded = NXBasics::Utf8_Encode(character);

		if (encoded.empty() || encoded.size() > mMaximumLength - length)
			return 2;

		std::memmove(mText + mCursorPosition + encoded.size(), mText + mCursorPosition, length - mCursorPosition + 1u);
		std::memcpy(mText + mCursorPosition, encoded.data(), encoded.size());
		mCursorPosition += static_cast<std::uint32_t>(encoded.size());

		return 2;
	}

	// Original function: FUN_004a3cc2
	std::uint32_t CStringInputHandler::GetCursorPixelPosition(const NXBasics::CFont* font)
	{
		if (mActive == 0)
		{
			return 0;
		}

		std::string legacy;

		if (!NXBasics::Font_TryLegacyText(font, mText, legacy))
			return static_cast<std::uint32_t>(NXBasics::UnicodeFont_CursorPosition(font, mText, mCursorPosition));

		const char savedCharacter = mText[mCursorPosition];
		mText[mCursorPosition] = '\0';

		const std::uint32_t width = static_cast<std::uint32_t>(NXBasics::Font_GetPixelWidth(font, mText));

		mText[mCursorPosition] = savedCharacter;

		return width;
	}

	// Original function: FUN_004a3cf6
	void CStringInputHandler::SetString(const char* text)
	{
		if (mActive == 0)
		{

			return;
		}

		NXBasics::Text_Copy(mText, static_cast<std::size_t>(mMaximumLength) + 1, text);
		mCursorPosition = static_cast<std::uint32_t>(std::strlen(mText));
	}

	// Original function: FUN_004dffa4
	CStringInputHandler::CStringInputHandler()
	{
		std::memset(this, 0, sizeof(*this));
	}
}