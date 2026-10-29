#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "InGameGuiKeyAssignmentManager.h"
#include "X2DApplication.h"
#include "XMKeyManager.h"
#include "XNumericConstants.h"
#include <cstring>
#include <windows.h>

namespace NC2InGameGuiKeyAssignmentManager
{
	using NC2Lifetime::Read;
	using NC2Lifetime::Write;

	struct SDefaultKey
	{
		std::int32_t assignment;
		std::uint32_t virtualKey;
		std::uint8_t auxiliaryCode;
		std::uint8_t shift;
		std::uint8_t control;
		std::byte padding0B;
	};

	static_assert(sizeof(SDefaultKey) == 12);
	// Complete Windows default table, 004f4e38..004f524b, including terminator.
	static constexpr SDefaultKey gDefaultKeys[] = {
		{ 1, 27, 0, 0, 0, { } },
		{ 2, 112, 0, 0, 0, { } },
		{ 3, 113, 0, 0, 0, { } },
		{ 13, 114, 0, 0, 0, { } },
		{ 14, 115, 0, 0, 0, { } },
		{ 4, 116, 0, 0, 0, { } },
		{ 5, 117, 0, 0, 0, { } },
		{ 6, 118, 0, 0, 0, { } },
		{ 7, 119, 0, 0, 0, { } },
		{ 8, 0, 66, 1, 0, { } },
		{ 9, 0, 101, 0, 0, { } },
		{ 9, 0, 65, 1, 0, { } },
		{ 15, 120, 0, 0, 0, { } },
		{ 10, 121, 0, 0, 0, { } },
		{ 11, 0, 111, 0, 0, { } },
		{ 12, 13, 0, 0, 0, { } },
		{ 22, 0, 112, 0, 0, { } },
		{ 22, 0, 80, 1, 0, { } },
		{ 23, 0, 108, 0, 0, { } },
		{ 29, 0, 46, 0, 0, { } },
		{ 30, 0, 44, 0, 0, { } },
		{ 31, 0, 121, 0, 0, { } },
		{ 32, 0, 120, 0, 0, { } },
		{ 33, 0, 102, 0, 0, { } },
		{ 33, 0, 88, 1, 0, { } },
		{ 34, 0, 89, 1, 0, { } },
		{ 16, 0, 98, 0, 0, { } },
		{ 17, 0, 115, 0, 0, { } },
		{ 18, 0, 100, 0, 0, { } },
		{ 19, 0, 68, 1, 0, { } },
		{ 20, 0, 118, 0, 0, { } },
		{ 21, 0, 86, 1, 0, { } },
		{ 24, 38, 0, 0, 0, { } },
		{ 25, 40, 0, 0, 0, { } },
		{ 26, 37, 0, 0, 0, { } },
		{ 27, 39, 0, 0, 0, { } },
		{ 24, 103, 0, 0, 0, { } },
		{ 24, 104, 0, 0, 0, { } },
		{ 24, 105, 0, 0, 0, { } },
		{ 25, 97, 0, 0, 0, { } },
		{ 25, 98, 0, 0, 0, { } },
		{ 25, 99, 0, 0, 0, { } },
		{ 26, 97, 0, 0, 0, { } },
		{ 26, 100, 0, 0, 0, { } },
		{ 26, 103, 0, 0, 0, { } },
		{ 27, 99, 0, 0, 0, { } },
		{ 27, 102, 0, 0, 0, { } },
		{ 27, 105, 0, 0, 0, { } },
		{ 35, 0, 122, 0, 0, { } },
		{ 36, 0, 104, 0, 0, { } },
		{ 37, 0, 32, 0, 0, { } },
		{ 37, 6, 0, 0, 0, { } },
		{ 38, 0, 99, 0, 0, { } },
		{ 39, 0, 97, 0, 0, { } },
		{ 40, 0, 119, 0, 0, { } },
		{ 41, 49, 0, 0, 0, { } },
		{ 42, 50, 0, 0, 0, { } },
		{ 43, 51, 0, 0, 0, { } },
		{ 44, 52, 0, 0, 0, { } },
		{ 45, 53, 0, 0, 0, { } },
		{ 46, 54, 0, 0, 0, { } },
		{ 47, 55, 0, 0, 0, { } },
		{ 48, 56, 0, 0, 0, { } },
		{ 49, 57, 0, 0, 0, { } },
		{ 50, 48, 0, 0, 0, { } },
		{ 51, 49, 0, 0, 1, { } },
		{ 52, 50, 0, 0, 1, { } },
		{ 53, 51, 0, 0, 1, { } },
		{ 54, 52, 0, 0, 1, { } },
		{ 55, 53, 0, 0, 1, { } },
		{ 56, 54, 0, 0, 1, { } },
		{ 57, 55, 0, 0, 1, { } },
		{ 58, 56, 0, 0, 1, { } },
		{ 59, 57, 0, 0, 1, { } },
		{ 60, 48, 0, 0, 1, { } },
		{ 61, 49, 0, 1, 0, { } },
		{ 62, 50, 0, 1, 0, { } },
		{ 63, 51, 0, 1, 0, { } },
		{ 64, 52, 0, 1, 0, { } },
		{ 65, 53, 0, 1, 0, { } },
		{ 66, 54, 0, 1, 0, { } },
		{ 67, 55, 0, 1, 0, { } },
		{ 68, 56, 0, 1, 0, { } },
		{ 69, 57, 0, 1, 0, { } },
		{ 70, 48, 0, 1, 0, { } },
		{ 28, 9, 0, 0, 0, { } },
		{ 0, 0, 0, 0, 0, { } },
	};

	// Original function: FUN_004a3d47.
	CKeyAssignmentManager::CKeyAssignmentManager()
	{
		sTheObjectPtr = this;
		std::memset(mStorage, 0, sizeof(mStorage));

		for (const auto& entry : gDefaultKeys)
		{
			if (entry.assignment == 0)
				break;

			AddKey(entry.assignment, entry.virtualKey, entry.auxiliaryCode, entry.shift, entry.control);
		}
	}

	// Original function: FUN_004a3d9a.
	CKeyAssignmentManager::~CKeyAssignmentManager()
	{
		sTheObjectPtr = nullptr;
	}

	// Original function: FUN_004a3da2.
	bool CKeyAssignmentManager::CheckKey(std::int32_t assignment, std::uint32_t virtualKey, std::uint8_t auxiliaryCode) const
	{
		if (assignment >= 71)
			return false;

		const auto offset = static_cast<std::uint32_t>(assignment) * NC2Layout::RecordSize::KeyAssignmentUnsigned;
		const auto count = Read<std::int32_t>(mStorage, offset);

		if (count <= 0)
			return false;

		const auto* keyManager = NXSysKeyManager::CKeyManager::sTheObjectPtr;

		for (std::int32_t index = 0; index < count; ++index)
		{
			const auto entry = offset + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::KeyBindingUnsigned;

			if (Read<std::uint32_t>(mStorage, entry + NC2Layout::KeyAssignmentViewOffset::FirstVirtualKeyUnsigned) != virtualKey || Read<std::uint8_t>(mStorage, entry + NC2Layout::KeyAssignmentViewOffset::FirstAuxiliaryCodeUnsigned) != auxiliaryCode)
				continue;

			if (Read<std::uint8_t>(mStorage, entry + NC2Layout::KeyAssignmentViewOffset::FirstShiftFlagUnsigned) != (Read<std::uint8_t>(keyManager, NC2Layout::KeyManagerOffset::ModifierFlags) & 1u))
				continue;

			if (Read<std::uint8_t>(mStorage, entry + NC2Layout::KeyAssignmentViewOffset::FirstControlFlagUnsigned) == ((Read<std::uint32_t>(keyManager, NC2Layout::KeyManagerOffset::ModifierFlags) >> 1) & 1u))
				return true;
		}

		return false;
	}

	// Original function: FUN_004a3e03.
	bool CKeyAssignmentManager::CheckDirectKeyVirtualOnly(std::int32_t assignment) const
	{
		if (!NXSys2DApp::CApplication::sTheObjectPtr->IsApplicationActive() || assignment >= 71)
			return false;

		const auto offset = static_cast<std::uint32_t>(assignment) * NC2Layout::RecordSize::KeyAssignmentUnsigned;

		if (Read<std::int32_t>(mStorage, offset) <= 0)
			return false;

		for (std::int32_t index = 0; index < Read<std::int32_t>(mStorage, offset); ++index)
		{
			const auto key = Read<std::uint32_t>(mStorage, offset + NC2Layout::KeyAssignmentViewOffset::FirstVirtualKeyUnsigned + static_cast<std::uint32_t>(index) * NC2Layout::RecordSize::KeyBindingUnsigned);

			if (key != 0 && (GetKeyState(static_cast<int>(key)) & NXBasics::BitMask::Bit15Signed) != 0)
				return true;
		}

		return false;
	}

	// Original function: FUN_004a3e5b.
	bool CKeyAssignmentManager::AddKey(std::int32_t assignment, std::uint32_t virtualKey, std::uint8_t auxiliaryCode, std::uint8_t shift, std::uint8_t control)
	{
		if (assignment >= 71)
			return false;

		const auto offset = static_cast<std::uint32_t>(assignment) * NC2Layout::RecordSize::KeyAssignmentUnsigned;
		const auto count = Read<std::uint32_t>(mStorage, offset);

		if (count == 4)
			return false;

		if (virtualKey != 0 || auxiliaryCode != 0)
		{
			Write(mStorage, offset + NC2Layout::KeyAssignmentViewOffset::FirstVirtualKeyUnsigned + count * NC2Layout::RecordSize::KeyBindingUnsigned, virtualKey);
			Write(mStorage, offset + NC2Layout::KeyAssignmentViewOffset::FirstAuxiliaryCodeUnsigned + Read<std::uint32_t>(mStorage, offset) * NC2Layout::RecordSize::KeyBindingUnsigned, auxiliaryCode);
			Write(mStorage, offset + NC2Layout::KeyAssignmentViewOffset::FirstShiftFlagUnsigned + Read<std::uint32_t>(mStorage, offset) * NC2Layout::RecordSize::KeyBindingUnsigned, shift);
			Write(mStorage, offset + NC2Layout::KeyAssignmentViewOffset::FirstControlFlagUnsigned + Read<std::uint32_t>(mStorage, offset) * NC2Layout::RecordSize::KeyBindingUnsigned, control);
			Write(mStorage, offset, Read<std::uint32_t>(mStorage, offset) + 1u);
		}

		return true;
	}
}