#include "C2CallbackManager.h"
#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiOverviewHandler.h"
#include "InGameGuiSupportAccess.h"
#include "SwitchIdentifiers.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XNumericConstants.h"

#include <cstring>

namespace NC2InGameGuiMessages
{
	namespace
	{
		using namespace NC2Lifetime;

		std::byte* Displayed(void* manager, std::uint32_t index)
		{

			return Read<std::byte*>(manager, NC2Layout::GuiMessageManagerOffset::DisplayedMessages) + index * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned;
		}

		std::byte* History(const void* manager, std::uint32_t index)
		{

			return Read<std::byte*>(manager, NC2Layout::GuiMessageManagerOffset::HistoryMessages) + index * NC2Layout::RecordSize::GuiMessageHistoryUnsigned;
		}

		// Original function: FUN_004b45c8; the object references are not validated here.
		bool SingleMessageDisplayed_GetMapPositionImpl(const void* record, std::uint32_t* point)
		{
			const auto human = Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanId);

			if (human != NXBasics::IntegerRange::UInt32Maximum)
			{
				*point = Read<std::uint32_t>(NC2Logic::Human_GetMapPositionRef(NC2Logic::CHumanArray::mStaticVars + human * NC2Logic::CHumanArray::ElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates);
				return true;
			}

			const auto house = Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HouseId);

			if (house != NXBasics::IntegerRange::UInt32Maximum)
			{
				*point = Read<std::uint32_t>(NC2Logic::CHouseArray::mStaticVars, house * NC2Logic::CHouseArray::ElementSize + NC2Layout::HouseOffset::MapPosition);
				return true;
			}

			const auto vehicle = Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::VehicleId);

			if (vehicle != NXBasics::IntegerRange::UInt32Maximum)
			{
				*point = Read<std::uint32_t>(NC2Logic::Vehicle_GetMapPositionRef(NC2Logic::CVehicleArray::mStaticVars + vehicle * NC2Logic::CVehicleArray::ElementSize), NC2Layout::MapCoordinatePairOffset::Coordinates);
				return true;
			}

			if (Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::Type) == 0x3B)
			{
				const auto* dead = static_cast<NC2InGameGuiDeadHumanDataManager::CDeadHumanDataManager*>(NC2InGameGuiDeadHumanDataManager::gDeadHumanDataManager)->DeadHuman_GetDataPtrByHumanUniqueId(Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanUniqueId));

				if (dead != nullptr)
				{
					*point = Read<std::uint32_t>(dead, NC2Layout::DeadHumanRecordOffset::MapPosition);
					return true;
				}
			}

			if (!NC2Logic::MapMigPoint_IsInsideMap(static_cast<const std::byte*>(record) + NC2Layout::GuiMessageEnvelopeOffset::MapPoint))
				return false;

			*point = Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::MapPoint);

			return true;
		}

		// Original function: FUN_004b465d.
		std::uint32_t SingleMessageHistory_GetValidHumanId(const void* record)
		{
			if (Read<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanId) == NXBasics::IntegerRange::UInt32Maximum || !NC2Logic::ToUserMessage_IsValid(static_cast<const std::byte*>(record) + NC2Layout::GuiMessageEnvelopeOffset::Message, false))
				return NXBasics::IntegerRange::UInt32Maximum;

			return Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanId);
		}

		// Original function: FUN_004d1144.
		void MessageMiniButton_SetXPosition(void* button, std::int32_t x)
		{
			Write<std::uint32_t>(button, NC2Layout::GuiBaseElementOffset::PositionY, 0);
			Write(button, NC2Layout::GuiBaseElementOffset::PositionX, x);
		}

		void RemoveMiniButton(void* button)
		{
			NXBaseGui::CDesktop::sTheObjectPtr->Element_Remove(static_cast<NXBaseGui::CBaseElement*>(button));
			using TDelete = void*(__thiscall*)(void*, std::uint32_t);
			Read<TDelete>(Read<void*>(button, NC2Layout::GuiBaseElementOffset::VirtualTable), NC2Layout::GuiVirtualTableOffset::DeletingDestructor)(button, 1);
		}
	}

	bool SingleMessageDisplayed_GetMapPosition(const void* record, std::uint32_t* point)
	{

		return SingleMessageDisplayed_GetMapPositionImpl(record, point);
	}

	// Original function: FUN_004b45a7.
	std::uint32_t SingleMessageDisplayed_GetValidHumanId(const void* record)
	{
		if (NC2Lifetime::Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanId) == NXBasics::IntegerRange::UInt32Maximum || !NC2Logic::ToUserMessage_IsValid(static_cast<const std::byte*>(record) + NC2Layout::GuiMessageEnvelopeOffset::Message, false))
			return NXBasics::IntegerRange::UInt32Maximum;

		return NC2Lifetime::Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanId);
	}

	// Original function: FUN_004b3c29; compaction revisits each removed slot.
	void CMessageManager::System_Update()
	{
		using namespace NC2Lifetime;
		const auto tick = NC2Logic::gGameTime.tick;
		std::uint32_t index = 0;

		while (index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount))
		{
			auto* record = Displayed(this, index);

			if (Read<std::uint32_t>(record, NC2Layout::GuiDisplayedMessageOffset::CreatedTick) + NC2Runtime::GuiMessageLimits::LifetimeTicksUnsigned < tick)
			{
				Message_Remove(Read<std::uint32_t>(record, NC2Layout::GuiDisplayedMessageOffset::UniqueId), false);
			}
			else
			{
				++index;
			}
		}

		index = 0;

		while (index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount))
		{
			if (!NC2Logic::ToUserMessage_IsValid(Displayed(this, index) + NC2Layout::GuiMessageEnvelopeOffset::Message, true))
			{
				Message_Remove(Read<std::uint32_t>(Displayed(this, index), NC2Layout::GuiDisplayedMessageOffset::UniqueId), false);
			}
			else
			{
				++index;
			}
		}

		index = 0;

		while (index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount))
		{
			std::uint32_t job = 0;

			if (SingleMessageDisplayed_GetValidHumanId(Displayed(this, index)) != NXBasics::IntegerRange::UInt32Maximum)
				job = Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, SingleMessageDisplayed_GetValidHumanId(Displayed(this, index)) * NC2Logic::CHumanArray::ElementSize + NC2Layout::HumanOffset::JobType);

			if (MessagePriority_Calculate(Read<std::uint32_t>(Displayed(this, index), NC2Layout::GuiMessageEnvelopeOffset::Type), job) < Read<std::int32_t>(this, NC2Layout::GuiMessageManagerOffset::MinimumPriority))
			{
				Message_Remove(Read<std::uint32_t>(Displayed(this, index), NC2Layout::GuiDisplayedMessageOffset::UniqueId), false);
			}
			else
			{
				++index;
			}
		}

		const auto historyTick = NC2Logic::gGameTime.tick;

		for (index = 0; index < NC2Runtime::GuiMessageLimits::Capacity; ++index)
		{
			auto* record = History(this, index);

			if (Read<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag) != 0 && (!NC2Logic::ToUserMessage_IsValid(record + NC2Layout::GuiMessageEnvelopeOffset::Message, true) || Read<std::uint32_t>(record, NC2Layout::GuiMessageHistoryOffset::CreatedTick) + NC2Runtime::GuiMessageLimits::LifetimeTicksUnsigned < historyTick))
				Write<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag, 0);
		}
	}

	// Original function: FUN_004b3d54; retained record bytes are deliberately untouched.
	void CMessageManager::System_HumanArrayDestructed()
	{
		using namespace NC2Lifetime;
		auto* window = Read<NXBaseGui::CBaseWindow*>(this, NC2Layout::GuiMessageManagerOffset::PresentationWindow);

		if (window != nullptr)
			NXBaseGui::CDesktop::sTheObjectPtr->Window_Close(window);

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount); ++index)
		{
			if (auto* button = Read<void*>(Displayed(this, index), NC2Layout::GuiDisplayedMessageOffset::MiniButton))
				RemoveMiniButton(button);
		}

		Write<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount, 0);
	}

	// Original function: FUN_004b3daa; filtered and duplicate messages still report success.
	bool CMessageManager::Message_Add(const void* message)
	{
		using namespace NC2Lifetime;

		if (l_MessageHistory_IsSameInList(message) || l_MessageHistory_IsSimmilarInList(message))
			return true;

		if (Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount) == NC2Runtime::GuiMessageLimits::Capacity)
			return false;

		std::uint32_t job = 0;
		const auto human = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanId);

		if (human != NXBasics::IntegerRange::UInt32Maximum && Read<std::uint8_t>(NC2Logic::CHumanArray::mStaticVars, human * NC2Logic::CHumanArray::ElementSize) != 0)
			job = Read<std::uint32_t>(NC2Logic::CHumanArray::mStaticVars, human * NC2Logic::CHumanArray::ElementSize + NC2Layout::HumanOffset::JobType);

		if (MessagePriority_Calculate(Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type), job) < Read<std::int32_t>(this, NC2Layout::GuiMessageManagerOffset::MinimumPriority) || NC2Logic::gGameTime.tick <= 1u)
			return true;

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount); ++index)
		{
			if (NC2Logic::ToUserMessage_IsEqualTo(Displayed(this, index) + NC2Layout::GuiMessageEnvelopeOffset::Message, message))
				return true;
		}

		alignas(4) std::byte existing[772];

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount); ++index)
		{
			NC2Logic::ToUserMessage_CopyConstruct(existing, Displayed(this, index) + NC2Layout::GuiMessageEnvelopeOffset::Message);

			if (NC2Logic::ToUserMessage_IsSimmilarTo(message, existing))
				return true;
		}

		auto* record = Displayed(this, Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount));
		std::memset(record, 0, sizeof(SGuiDisplayedMessageStorage));
		Write(record, NC2Layout::GuiDisplayedMessageOffset::CreatedTick, NC2Logic::gGameTime.tick);
		Write(record, NC2Layout::GuiDisplayedMessageOffset::UniqueId, Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::NextMessageId));
		Write(this, NC2Layout::GuiMessageManagerOffset::NextMessageId, Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::NextMessageId) + 1u);
		std::memcpy(record + NC2Layout::GuiMessageEnvelopeOffset::Message, message, 772);
		NC2Logic::ToUserMessage_FinishMessage(record + NC2Layout::GuiMessageEnvelopeOffset::Message);
		Write(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount, Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount) + 1u);
		std::uint32_t point = 0;

		if (SingleMessageDisplayed_GetMapPosition(record, &point))
			NC2InGameGuiOverviewDisplay::CWorldOverviewElementHandler::sTheObjectPtr->Marker_AddInfoMarker(Read<std::int32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::PlayerId), &point);

		const auto validHuman = SingleMessageDisplayed_GetValidHumanId(record);

		if (validHuman != NXBasics::IntegerRange::UInt32Maximum)
			NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0xE, true, validHuman, 0x30, 0);

		l_GuiElements_UpdateMiniButtons();

		return true;
	}

	// Original function: FUN_004b400e; callbacks precede element deletion and compaction.
	void CMessageManager::Message_Remove(std::uint32_t uniqueId, bool addToHistory)
	{
		using namespace NC2Lifetime;
		const auto count = Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount);
		auto* active = Read<std::byte*>(this, NC2Layout::GuiMessageManagerOffset::DisplayedMessages);
		std::uint32_t index = 0;

		while (index < count && Read<std::uint32_t>(active, index * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned + NC2Layout::GuiDisplayedMessageOffset::UniqueId) != uniqueId)
			++index;

		if (index == count)
			return;

		index = 0;

		while (index < count && Read<std::uint32_t>(active, index * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned + NC2Layout::GuiDisplayedMessageOffset::UniqueId) != uniqueId)
			++index;

		if (index == count)
		{

			return;
		}

		auto* record = active + index * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned;
		const auto human = SingleMessageDisplayed_GetValidHumanId(record);

		if (human != NXBasics::IntegerRange::UInt32Maximum)
		{
			std::uint32_t other = 0;

			while (other < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount))
			{
				if (other != index && SingleMessageDisplayed_GetValidHumanId(Displayed(this, other)) == human)
					break;

				++other;
			}

			if (other == Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount))
				NC2Logic::CCallbackManager::sTheObjectPtr->ExecuteCallback(0xE, true, human, 0x20, 0);
		}

		if (auto* button = Read<void*>(record, NC2Layout::GuiDisplayedMessageOffset::MiniButton))
			RemoveMiniButton(button);

		auto* window = Read<NXBaseGui::CBaseWindow*>(this, NC2Layout::GuiMessageManagerOffset::PresentationWindow);

		if (window != nullptr && Read<std::uint32_t>(record, NC2Layout::GuiDisplayedMessageOffset::UniqueId) == Read<std::uint32_t>(window, NC2Layout::MessagePresentationWindowOffset::UniqueId))
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(window);

		if (addToHistory)
			l_MessageHistory_Add(record + NC2Layout::GuiMessageEnvelopeOffset::Message);

		const auto remaining = Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount) - index - 1u;

		if (NXBasics::Runtime_TimeSigned32(remaining) > 0)
			std::memmove(record, record + NC2Layout::RecordSize::GuiDisplayedMessage, remaining * NC2Layout::RecordSize::GuiDisplayedMessageUnsigned);

		Write(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount, Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount) - 1u);
		l_GuiElements_UpdateMiniButtons();
	}

	// Original function: FUN_004b4144; remove only the first matching message.
	void CMessageManager::Message_RemoveHumanMessage(std::uint32_t humanId, std::uint32_t type)
	{
		using namespace NC2Lifetime;
		System_Update();
		const auto count = Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount);

		for (std::uint32_t index = 0; index < count; ++index)
		{
			const void* record = Displayed(this, index);

			if (Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::Type) == type && Read<std::uint32_t>(record, NC2Layout::GuiMessageEnvelopeOffset::HumanId) == humanId)
			{
				Message_Remove(Read<std::uint32_t>(record, NC2Layout::GuiDisplayedMessageOffset::UniqueId), false);

				return;
			}
		}
	}

	// Original function: FUN_004b41cb.
	void CMessageManager::Message_RemoveAllForHuman(std::uint32_t humanId)
	{
		using namespace NC2Lifetime;
		System_Update();
		std::uint32_t index = 0;

		while (index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount))
		{
			if (SingleMessageDisplayed_GetValidHumanId(Displayed(this, index)) == humanId)
			{
				Message_Remove(Read<std::uint32_t>(Displayed(this, index), NC2Layout::GuiDisplayedMessageOffset::UniqueId), false);
			}
			else
			{
				++index;
			}
		}

		for (index = 0; index < NC2Runtime::GuiMessageLimits::Capacity; ++index)
		{
			auto* record = History(this, index);

			if (Read<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag) != 0 && SingleMessageHistory_GetValidHumanId(record) == humanId)
				Write<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag, 0);
		}
	}

	// Original function: FUN_004b4187.
	void CMessageManager::Message_RemoveAll(bool addToHistory)
	{
		while (NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount) != 0)
			Message_Remove(NC2Lifetime::Read<std::uint32_t>(Displayed(this, 0), NC2Layout::GuiDisplayedMessageOffset::UniqueId), addToHistory);
	}

	// Original function: FUN_004b41a6; returns the first matching displayed record.
	void* CMessageManager::Message_GetPtr(std::uint32_t uniqueId) const
	{
		const auto count = NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount);
		auto* record = NC2Lifetime::Read<std::byte*>(this, NC2Layout::GuiMessageManagerOffset::DisplayedMessages);

		for (std::uint32_t index = 0; index < count; ++index, record += NC2Layout::RecordSize::GuiDisplayedMessage)
		{
			if (NC2Lifetime::Read<std::uint32_t>(record, NC2Layout::GuiDisplayedMessageOffset::UniqueId) == uniqueId)
				return record;
		}

		return nullptr;
	}

	// Original function: FUN_004b4290; deletion invalidates only the button pointer.
	void CMessageManager::MiniButton_Removed(std::uint32_t uniqueId)
	{
		if (auto* record = Message_GetPtr(uniqueId))
			NC2Lifetime::Write<void*>(record, NC2Layout::GuiDisplayedMessageOffset::MiniButton, nullptr);
	}

	// Original function: FUN_004b42c8; each horizontal step is rounded to a stored float.
	void CMessageManager::l_GuiElements_UpdateMiniButtons()
	{
		using namespace NC2Lifetime;
		const auto count = Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount);

		if (count == 0)
		{

			return;
		}

		const double quotient = 400.0 / static_cast<double>(count);
		float spacing = static_cast<float>(quotient);

		if (quotient > 54.0)
			spacing = 54.0f;

		float x = 144.0f;

		for (std::uint32_t index = 0; index < Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::DisplayedCount); ++index)
		{
			auto* record = Displayed(this, index);
			auto* button = Read<void*>(record, NC2Layout::GuiDisplayedMessageOffset::MiniButton);

			if (button == nullptr)
			{
				auto* storage = NXBasics::Runtime_AllocateWithNewHandler(sizeof(SMessageMiniButtonStorage), 1);
				button = storage == nullptr ? nullptr : MessageMiniButton_Construct(storage, Read<std::uint32_t>(record, NC2Layout::GuiDisplayedMessageOffset::UniqueId), NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(x))));
				Write(record, NC2Layout::GuiDisplayedMessageOffset::MiniButton, button);
				NC2InGameGuiBase::BaseToolDesktop_AddElement(static_cast<NXBaseGui::CBaseElement*>(button));
			}
			else
			{
				MessageMiniButton_SetXPosition(button, NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NXBasics::Runtime_TruncateDoubleToInt64(x))));
			}

			x = static_cast<float>(static_cast<double>(spacing) + static_cast<double>(x));
		}
	}

	// Original function: FUN_004b438a; an unused slot wins, otherwise retain the earliest tie.
	void CMessageManager::l_MessageHistory_Add(const void* message)
	{
		using namespace NC2Lifetime;
		auto* history = Read<std::byte*>(this, NC2Layout::GuiMessageManagerOffset::HistoryMessages);
		std::uint32_t selected = NXBasics::IntegerRange::UInt32Maximum, oldest = NXBasics::IntegerRange::UInt32Maximum;

		for (std::uint32_t index = 0; index < NC2Runtime::GuiMessageLimits::Capacity; ++index)
		{
			auto* record = history + index * NC2Layout::RecordSize::GuiMessageHistoryUnsigned;

			if (Read<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag) == 0)
			{
				selected = index;
				break;
			}

			const auto tick = Read<std::uint32_t>(record, NC2Layout::GuiMessageHistoryOffset::CreatedTick);

			if (tick < oldest)
			{
				oldest = tick;
				selected = index;
			}
		}

		if (selected == NXBasics::IntegerRange::UInt32Maximum)
		{

			return;
		}

		auto* record = history + selected * NC2Layout::RecordSize::GuiMessageHistoryUnsigned;
		std::memset(record, 0, NC2Layout::RecordSize::GuiMessageHistory);
		Write<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag, 1);
		Write(record, NC2Layout::GuiMessageHistoryOffset::CreatedTick, NC2Logic::gGameTime.tick);
		std::memcpy(record + NC2Layout::GuiMessageEnvelopeOffset::Message, message, 772);
	}

	// Original function: FUN_004b43fd.
	bool CMessageManager::l_MessageHistory_IsSameInList(const void* message) const
	{
		for (std::uint32_t index = 0; index < NC2Runtime::GuiMessageLimits::Capacity; ++index)
		{
			const auto* record = History(this, index);

			if (NC2Lifetime::Read<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag) != 0 && NC2Logic::ToUserMessage_IsEqualTo(record + NC2Layout::GuiMessageEnvelopeOffset::Message, message))
				return true;
		}

		return false;
	}

	// Original function: FUN_004b4436.
	bool CMessageManager::l_MessageHistory_IsSimmilarInList(const void* message) const
	{
		for (std::uint32_t index = 0; index < NC2Runtime::GuiMessageLimits::Capacity; ++index)
		{
			const auto* record = History(this, index);

			if (NC2Lifetime::Read<std::uint8_t>(record, NC2Layout::GuiMessageHistoryOffset::AllocatedFlag) != 0 && NC2Logic::ToUserMessage_IsSimmilarTo(record + NC2Layout::GuiMessageEnvelopeOffset::Message, message))
				return true;
		}

		return false;
	}

	// Original function: FUN_004b446f; retain the original unsigned increment and wrap condition.
	void CMessageManager::MessagePriority_Toggle()
	{
		auto priority = NC2Lifetime::Read<std::uint32_t>(this, NC2Layout::GuiMessageManagerOffset::MinimumPriority) + 1u;

		if (priority == 3)
			priority = 0;

		NC2Lifetime::Write(this, NC2Layout::GuiMessageManagerOffset::MinimumPriority, priority);
		System_Update();
	}

	// Original function: FUN_004b4490; complete selector region 004b44ce..004b450c.
	std::int32_t CMessageManager::MessagePriority_Calculate(std::uint32_t type, std::uint32_t job)
	{
		static constexpr std::uint8_t priorityModeByType[63] = {
			0, 0, 1, 2, 0, 3, 3, 0, 0, 0, 3, 3, 3, 0, 0, 0,
			3, 1, 1, 1, 1, 1, 1, 3, 0, 0, 0, 3, 3, 3, 0, 1,
			1, 3, 3, 3, 0, 3, 3, 3, 0, 0, 0, 3, 3, 1, 0, 0,
			1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1
		};

		if (type - 1u > 62u)
			return 0;

		switch (priorityModeByType[type - 1u])
		{
			case SwitchIds::MessagePriorityMode::Normal:
				return 1;
			case SwitchIds::MessagePriorityMode::Elevated:
				return 2;
			case SwitchIds::MessagePriorityMode::ScoutOnly:
				return job == 8 ? 1 : 0;
			default:

				return 0;
		}
	}

	// Original function: FUN_004b450d.
	bool __cdecl CMessageManager::ls_Callback_Logic(std::uint32_t type, std::uintptr_t /*userData*/, std::uintptr_t callbackValueParam1, std::uintptr_t callbackValueParam2, std::uintptr_t /*param3*/)
	{
		std::uint32_t param1 = static_cast<std::uint32_t>(callbackValueParam1);
		std::uint32_t param2 = static_cast<std::uint32_t>(callbackValueParam2);
		auto* manager = static_cast<CMessageManager*>(gMessageManager);

		if (type == 1)
		{
			manager->System_Update();
		}
		else if (type == 3 && param1 == 0)
		{
			manager->System_HumanArrayDestructed();
		}
		else if (type == 4 && param2 == 0)
		{
			manager->Message_RemoveAllForHuman(param1);
		}
		else if (type == 0x16)
		{
			manager->l_GuiElements_UpdateMiniButtons();
		}
		else if (type == 0x2E)
		{
			while (NC2Lifetime::Read<std::uint32_t>(manager, NC2Layout::GuiMessageManagerOffset::DisplayedCount) != 0)
				manager->Message_Remove(NC2Lifetime::Read<std::uint32_t>(Displayed(manager, 0), NC2Layout::GuiDisplayedMessageOffset::UniqueId), false);

			std::memset(NC2Lifetime::Read<void*>(gMessageManager, NC2Layout::GuiMessageManagerOffset::HistoryMessages), 0, NC2Layout::RecordSize::GuiMessageHistoryPool);
		}

		return true;
	}
}