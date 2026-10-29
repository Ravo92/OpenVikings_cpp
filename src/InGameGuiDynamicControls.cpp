#include "InGameGuiSelectionWindowAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "XCrtFileAccess.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"
#include "XTextEncoding.h"
#include <cstdarg>
#include <cstring>

namespace NC2InGameGuiBase
{
	// Original function: FUN_004b9d0c; full Windows storage, including text padding.
	CStaticToolTipHandler::CStaticToolTipHandler()
	{
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_004b9d23.
	CStaticToolTipHandler::~CStaticToolTipHandler()
	{
	}

	// Original function: FUN_004b9d24.
	void CStaticToolTipHandler::ClearAll()
	{
		mCount = 0;
	}

	// Original function: FUN_004b9d28; the terminator follows the 100 copied bytes.
	bool CStaticToolTipHandler::Add(const NXBasics::SRectangle& rectangle, const char* text)
	{
		if (mCount == NC2Runtime::SelectionTooltipLimits::Entries)
			return false;

		auto& entry = mEntries[mCount];
		entry.rectangle = rectangle;
		NXBasics::Text_Copy(entry.text, NC2Runtime::SelectionTooltipLimits::CopiedTextBytes + 1u, text);
		entry.text[NC2Runtime::SelectionTooltipLimits::StoredTerminator] = '\0';
		++mCount;

		return true;
	}

	// Original function: FUN_004b9d73; the native temporary buffer holds 100 bytes.
	bool CStaticToolTipHandler::AddV(const NXBasics::SRectangle& rectangle, const char* format, ...)
	{
		char text[NC2Runtime::SelectionTooltipLimits::FormatBufferBytes];
		va_list arguments;
		va_start(arguments, format);
		NXBasics::Runtime_FormatString(text, NC2Runtime::SelectionTooltipLimits::FormatBufferBytes, format, arguments);
		va_end(arguments);
		text[NC2Runtime::SelectionTooltipLimits::FormatTerminator] = '\0';

		return Add(rectangle, text);
	}

	// Original function: FUN_004b9da3; later rectangles take precedence.
	const char* CStaticToolTipHandler::Get(const std::int32_t* point) const
	{
		for (std::uint32_t index = mCount; index != 0; --index)
		{
			const auto& entry = mEntries[index - 1];
			const auto right = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(entry.rectangle.x) + static_cast<std::uint32_t>(entry.rectangle.width) - 1u);
			const auto bottom = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(entry.rectangle.y) + static_cast<std::uint32_t>(entry.rectangle.height) - 1u);

			if (point[0] >= entry.rectangle.x && point[0] <= right && point[1] >= entry.rectangle.y && point[1] <= bottom)
				return entry.text;
		}

		return nullptr;
	}

	// Original function: FUN_004b9ddd; complete 100-entry Windows storage.
	CDynamicSubButtonsHandler::CDynamicSubButtonsHandler()
	{
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_004b9df4; the owning window survives child deletion.
	void CDynamicSubButtonsHandler::AllButtonsHaveBeenRemoved()
	{
		std::memset(mEntries, 0, sizeof(mEntries));
	}

	// Original function: FUN_004b9e08; unchanged coordinates retain the existing control.
	void CDynamicSubButtonsHandler::UpdateGfxButton(std::uint32_t command, std::uint32_t id, std::int32_t x, std::int32_t y, std::uint32_t graphic, const char* tooltip)
	{
		if (mWindow == nullptr)
			return;

		auto* entry = l_GetButtonDataPtr(command, id);

		if (entry != nullptr)
		{
			if (entry->x == x && entry->y == y)
				return;

			NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->graphic);
		}
		else
		{
			entry = l_GetFreeButtonDataPtr();
		}

		if (entry == nullptr)
		{

			return;
		}

		const NXBasics::SRectangle rectangle(x, y, 18, 16);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiGfxButton, 1);
		entry->graphic = allocation == nullptr ? nullptr : BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, graphic, tooltip, command, mWindow);
		NXBaseGui::BaseWindow_Element_Add(mWindow, entry->graphic);
		entry->x = x;
		entry->y = y;
		entry->command = command;
		entry->id = id;
		NC2Lifetime::Write<std::uint8_t>(entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag, 1);
	}

	// Original function: FUN_004b9eb7.
	void CDynamicSubButtonsHandler::UpdateTextButton(std::uint32_t command, std::uint32_t id, std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height, const char* text, bool centered, const char* tooltip)
	{
		if (mWindow == nullptr)
			return;

		auto* entry = l_GetButtonDataPtr(command, id);

		if (entry != nullptr)
		{
			if (entry->x == x && entry->y == y)
				return;

			NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->text);
		}
		else
		{
			entry = l_GetFreeButtonDataPtr();
		}

		if (entry == nullptr)
		{

			return;
		}

		const NXBasics::SRectangle rectangle(x, y, width, height);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
		entry->text = allocation == nullptr ? nullptr : BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &rectangle, text, tooltip, command, mWindow);
		NC2Lifetime::Write<std::uint8_t>(entry->text, NC2Layout::ToolTextButtonOffset::CenteredFlag, centered);
		NXBaseGui::BaseWindow_Element_Add(mWindow, entry->text);
		entry->x = x;
		entry->y = y;
		entry->command = command;
		entry->id = id;
		NC2Lifetime::Write<std::uint8_t>(entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag, 1);
	}

	// Original function: FUN_004b9f70; the two control classes have distinct offsets.
	void CDynamicSubButtonsHandler::SetButtonMiscCommandData(std::uint32_t command, std::uint32_t id, std::uint32_t first, std::uint32_t second)
	{
		if (mWindow == nullptr)
			return;

		auto* entry = l_GetButtonDataPtr(command, id);

		if (entry == nullptr)
		{

			return;
		}

		if (entry->graphic != nullptr)
		{
			NC2Lifetime::Write<std::uintptr_t>(entry->graphic, NC2Layout::ToolGfxButtonOffset::CommandData1, first);
			NC2Lifetime::Write<std::uintptr_t>(entry->graphic, NC2Layout::ToolGfxButtonOffset::CommandData2, second);
		}

		if (entry->text != nullptr)
		{
			NC2Lifetime::Write<std::uintptr_t>(entry->text, NC2Layout::ToolTextButtonOffset::CommandData1, first);
			NC2Lifetime::Write<std::uintptr_t>(entry->text, NC2Layout::ToolTextButtonOffset::CommandData2, second);
		}
	}

	// Original function: FUN_004b9fad; inactive entries retain their unused fields.
	void CDynamicSubButtonsHandler::RemoveButton(std::uint32_t command, std::uint32_t id)
	{
		if (mWindow == nullptr)
			return;

		auto* entry = l_GetButtonDataPtr(command, id);

		if (entry == nullptr)
		{

			return;
		}

		if (entry->graphic != nullptr)
			NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->graphic);

		if (entry->text != nullptr)
			NXBaseGui::BaseWindow_Element_Delete(mWindow, entry->text);

		NC2Lifetime::Write<std::uint8_t>(entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag, 0);
	}

	// Original function: FUN_004b9fef.
	CDynamicSubButtonsHandler::SEntry* CDynamicSubButtonsHandler::l_GetButtonDataPtr(std::uint32_t command, std::uint32_t id)
	{
		for (auto& entry : mEntries)
		{
			if (NC2Lifetime::Read<std::uint8_t>(&entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag) != 0 && entry.command == command && entry.id == id)
				return &entry;
		}

		return nullptr;
	}

	// Original function: FUN_004ba024.
	CDynamicSubButtonsHandler::SEntry* CDynamicSubButtonsHandler::l_GetFreeButtonDataPtr()
	{
		for (auto& entry : mEntries)
		{
			if (NC2Lifetime::Read<std::uint8_t>(&entry, NC2Layout::DynamicSubButtonEntryOffset::ActiveFlag) == 0)
				return &entry;
		}

		return nullptr;
	}
}