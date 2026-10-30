#include "XGuiElementAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineMoveableDrawing.h"
#include "C2EngineObjectData.h"
#include "C2HouseAccess.h"
#include "C2HumanEquipment.h"
#include "C2HumanIterator.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MissionStartObjects.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StringAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiHouseInhabitantsElement.h"
#include "InGameGuiManager.h"
#include "SwitchIdentifiers.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"
#include <array>
#include <cstring>

namespace NC2InGameGuiSelection
{
	using namespace NC2Lifetime;
	using TMessage = NXBaseGui::TGuiMessage;

	static void* House(std::int32_t id)
	{

		return NC2Logic::CHouseArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHouseArray::RuntimeElementSize;
	}

	static const void* Human(std::int32_t id)
	{

		return NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::RuntimeElementSize;
	}

	static void* Slot(void* element, std::int32_t index)
	{

		return static_cast<std::byte*>(element) + NC2Layout::HouseInhabitantsSelectionOffset::Rows + index * NC2Layout::RecordSize::HouseInhabitantsSelectionSlot;
	}

	static void* __fastcall SelectionHouseInhabitantsElement_Delete(void* element, void*, std::uint32_t flags);
	static bool __fastcall SelectionHouseInhabitantsElement_Message(void* element, void*, std::uint32_t message, std::uint32_t, std::uintptr_t, std::uintptr_t, std::uintptr_t);
	static void __fastcall SelectionHouseInhabitantsElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle);
	static bool __fastcall SelectionHouseInhabitantsElement_IsHit(void* element, void*, const std::int32_t* point);
	static bool __fastcall SelectionHouseInhabitantsElement_ToolTip_Available(void* element, void*, const char** output);
	static void __fastcall SelectionHouseInhabitantsElement_DoPressedAction(void* element);

	// Complete native table 004F79A8..004F79EF.
	static const std::array<std::uintptr_t, 18>& HouseInhabitantsTable()
	{
		static const std::array<std::uintptr_t, 18> methods = []
		{
			std::array<std::uintptr_t, 18> t{ };
			std::memcpy(t.data(), NXBaseGui::gBaseElementVirtualTable, sizeof(t));
			t[0] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseInhabitantsElement_Delete>());
			t[2] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseInhabitantsElement_Message>());
			t[3] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseInhabitantsElement_Draw>());
			t[8] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseInhabitantsElement_IsHit>());
			t[13] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectionHouseInhabitantsElement_ToolTip_Available>());
			t[14] = reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden);
			t[17] = reinterpret_cast<std::uintptr_t>(&SelectionHouseInhabitantsElement_DoPressedAction);

			return t;
		}();

		return methods;
	}

	// Original function: FUN_004cc045; native allocation 0x8E4, one hundred 0x14-byte slots.
	void* SelectionHouseInhabitantsElement_Construct(void* element, const NXBasics::SRectangle* rectangle, void* receiver, std::uint32_t selectCommand, std::uint32_t actionCommand, std::int32_t houseId)
	{
		NXBaseGui::BaseElement_Construct(element, rectangle, 3, 0x20);
		std::memset(static_cast<std::byte*>(element) + NC2Layout::HouseInhabitantsSelectionOffset::HouseId, 0, NC2Layout::RecordSize::HouseInhabitantsSelectionVars);
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, HouseInhabitantsTable().data());
		Write(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseId, houseId);

		if (houseId != -1)
		{
			const auto* house = House(houseId);

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag))
			{
				Write(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseRevision, Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId));
			}
			else
			{
				Write<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseId, -1);
			}
		}

		Write<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HitIndex, -1);
		Write(element, NC2Layout::HouseInhabitantsSelectionOffset::SelectCommand, selectCommand);
		Write(element, NC2Layout::HouseInhabitantsSelectionOffset::ActionCommand, actionCommand);
		Write(element, NC2Layout::HouseInhabitantsSelectionOffset::Receiver, receiver);
		Write(element, NC2Layout::HouseInhabitantsSelectionOffset::BaselineY, rectangle->height - 15);

		return element;
	}

	// Original function: FUN_004cc0d0.
	static void* __fastcall SelectionHouseInhabitantsElement_Delete(void* element, void*, std::uint32_t flags)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, HouseInhabitantsTable().data());
		NXBaseGui::BaseElement_Destruct(element);

		if (flags & SwitchIds::HouseAttachmentMask::HomeUnsigned)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// Original function: FUN_004cc0f2.
	static bool __fastcall SelectionHouseInhabitantsElement_Message(void* element, void*, std::uint32_t message, std::uint32_t, std::uintptr_t, std::uintptr_t, std::uintptr_t)
	{
		if (message != 0x20)
			return false;

		const auto index = Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HitIndex);

		if (index != -1)
		{
			auto* receiver = Read<void*>(element, NC2Layout::HouseInhabitantsSelectionOffset::Receiver);
			Read<TMessage*>(receiver, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](receiver, 0x800, Read<std::uint32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::ActionCommand), Read<std::uint32_t>(Slot(element, index), NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId), 0, 0);
		}

		return true;
	}

	// Original function: FUN_004cc1a8.
	static void __fastcall SelectionHouseInhabitantsElement_DoPressedAction(void* element)
	{
		const auto index = Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HitIndex);

		if (index == -1)
		{

			return;
		}

		auto* receiver = Read<void*>(element, NC2Layout::HouseInhabitantsSelectionOffset::Receiver);
		Read<TMessage*>(receiver, NC2Layout::GuiBaseElementOffset::VirtualTable)[2](receiver, 0x800, Read<std::uint32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::SelectCommand), Read<std::uint32_t>(Slot(element, index), NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId), 0, 0);
		NC2GuiToolsBase::FXTool_ClickConfirm();
	}

	// Original function: FUN_004cc4db.
	static bool l_ValidateHouse(void* element)
	{
		const auto id = Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseId);

		if (id == -1)
			return false;

		const auto* house = House(id);

		if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) && Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) == Read<std::uint32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseRevision))
			return true;

		Write<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseId, -1);
		Write<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HitIndex, -1);

		return false;
	}

	// Original function: FUN_004cc50d; attached roles, traders, then nearby player humans.
	static std::int32_t l_UpdateAttachedHumans(void* element)
	{
		auto* house = House(Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseId));
		std::int32_t count = 0;
		Write(element, NC2Layout::HouseInhabitantsSelectionOffset::Count, count);

		for (std::uint32_t index = 0; index < NC2Runtime::HouseInhabitantsSelectionLimits::HouseAttachments; ++index)
		{
			const auto* attachment = NC2Logic::House_Inhabitants_GetAttachedHumanSlotPtrBySlotId(house, index);
			const auto flags = Read<std::uint8_t>(attachment, NC2Layout::HouseAttachmentOffset::RoleMask);

			if (!flags)
				continue;

			const auto id = Read<std::int32_t>(attachment, NC2Layout::HouseAttachmentOffset::HumanId);
			const auto* human = Human(id);
			auto* slot = Slot(element, count);
			Write(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId, id);
			Write<std::uint32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::RoleMask, flags);
			std::int32_t priority = 1;

			if (flags & SwitchIds::HouseAttachmentMask::HomeUnsigned)
			{
				std::uint32_t families[NC2Runtime::HouseInhabitantsSelectionLimits::HouseAttachments];
				NC2Logic::House_Inhabitants_GetNumberOfFamiliesLivingInHome(house, families);

				if (families[index] != NXBasics::IntegerRange::UInt32Maximum)
					priority = static_cast<std::int32_t>((families[index] + 1) * 10);

				if (!NC2Logic::Human_IsAgeAdult(human))
				{
					priority += 2;
				}
				else if (NC2Logic::Human_IsSexFemale(human))
				{
					++priority;
				}
			}
			else if (flags & SwitchIds::HouseAttachmentMask::WorkerUnsigned)
			{
				priority = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == SwitchIds::HumanJob::Carrier ? 200 : 100;
			}
			else if (flags & SwitchIds::HouseAttachmentMask::BuilderUnsigned)
			{
				priority = 500;
			}

			Write(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority, priority);
			Write(element, NC2Layout::HouseInhabitantsSelectionOffset::Count, ++count);

			if (count == NC2Runtime::HouseInhabitantsSelectionLimits::Humans)
				break;
		}

		if (count < NC2Runtime::HouseInhabitantsSelectionLimits::Humans)
		{
			NC2Logic::CHumanIterator iterator;

			for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId)); id != -1 && count < NC2Runtime::HouseInhabitantsSelectionLimits::Humans; id = iterator.GetNextHumanId())
			{
				const auto* human = Human(id);

				if (!NC2Logic::Human_IsJobTrader(human))
					continue;

				auto* merchant = NC2Logic::HumanMerchantArray_GetMerchantDataPtr(NC2Logic::gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId));

				if (merchant == nullptr)
					continue;

				for (std::uint32_t index = 0; index < 2; ++index)
				{
					const auto* data = NC2Logic::MerchantData_GetHouseDataPtr(merchant, index);

					if (!NC2Logic::MerchantHouseData_IsValid(data) || Read<std::uint32_t>(data, NC2Layout::MerchantHouseOffset::HouseId) != Read<std::uint32_t>(house, NC2Layout::HouseOffset::HouseId))
						continue;

					auto* slot = Slot(element, count++);
					Write(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId, id);
					Write<std::uint32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::RoleMask, 0);
					Write<std::uint8_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::TraderFlag, 1);
					Write<std::int32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority, 50);
					Write(element, NC2Layout::HouseInhabitantsSelectionOffset::Count, count);

					if (count == NC2Runtime::HouseInhabitantsSelectionLimits::Humans)
						break;
				}

				if (count == NC2Runtime::HouseInhabitantsSelectionLimits::Humans)
					break;
			}
		}

		if (count < NC2Runtime::HouseInhabitantsSelectionLimits::Humans)
		{
			NC2Logic::CHumanIterator iterator;

			for (auto id = iterator.GetFirstHumanId(Read<std::uint32_t>(house, NC2Layout::HouseOffset::PlayerId), static_cast<std::byte*>(house) + NC2Layout::HouseOffset::DoorEntryPoint, 1); id != -1 && count < NC2Runtime::HouseInhabitantsSelectionLimits::Humans; id = iterator.GetNextHumanId())
			{
				std::int32_t index = 0;

				while (index < count && Read<std::int32_t>(Slot(element, index), NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId) != id)
					++index;

				if (index != count)
					continue;

				auto* slot = Slot(element, count++);
				Write(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId, id);
				Write<std::uint32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::RoleMask, 0);
				Write<std::int32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority, 1000);
				Write(element, NC2Layout::HouseInhabitantsSelectionOffset::Count, count);

				if (count == NC2Runtime::HouseInhabitantsSelectionLimits::Humans)
					break;
			}
		}

		for (auto end = count - 1; end >= 0; --end)
		{
			for (std::int32_t index = 0; index < end; ++index)
			{
				if (Read<std::int32_t>(Slot(element, index), NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority) > Read<std::int32_t>(Slot(element, index + 1), NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority))
				{
					std::byte temporary[NC2Layout::RecordSize::HouseInhabitantsSelectionSlot];
					std::memcpy(temporary, Slot(element, index), NC2Layout::RecordSize::HouseInhabitantsSelectionSlot);
					std::memcpy(Slot(element, index), Slot(element, index + 1), NC2Layout::RecordSize::HouseInhabitantsSelectionSlot);
					std::memcpy(Slot(element, index + 1), temporary, NC2Layout::RecordSize::HouseInhabitantsSelectionSlot);
				}
			}
		}

		if (count > 1)
		{
			std::int32_t gaps = 0;

			for (std::int32_t index = 0; index < count - 1; ++index)
			{
				auto delta = Read<std::int32_t>(Slot(element, index), NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority) - Read<std::int32_t>(Slot(element, index + 1), NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority);

				if (delta < 0)
					delta = -delta;

				if (delta > 3)
					++gaps;
			}

			double step = (static_cast<double>(Read<std::int32_t>(element, NC2Layout::GuiBaseElementOffset::Width) - 20) - gaps * 15.0 - 20.0) / (count - 1);

			if (step > 30.0)
				step = 30.0;

			double position = 20.0;
			// Windows accumulates both values in x87 extended precision; the numeric adapter uses double.

			for (std::int32_t index = 0; index < count; ++index)
			{
				Write(Slot(element, index), NC2Layout::HouseInhabitantsSelectionSlotOffset::X, static_cast<std::int32_t>(NXBasics::Runtime_TruncateDoubleToInt64(position)));
				position += step;

				if (index == count - 1)
					continue;

				auto delta = Read<std::int32_t>(Slot(element, index), NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority) - Read<std::int32_t>(Slot(element, index + 1), NC2Layout::HouseInhabitantsSelectionSlotOffset::SortPriority);

				if (delta < 0)
					delta = -delta;

				if (delta > 3)
					position += 15.0;
			}
		}
		else if (count == 1)
		{
			Write<std::int32_t>(Slot(element, 0), NC2Layout::HouseInhabitantsSelectionSlotOffset::X, 20);
		}

		return count;
	}

	// Original function: FUN_004cc132.
	static void __fastcall SelectionHouseInhabitantsElement_Draw(void* element, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		NC2GuiToolsBase::DrawTool_DrawFrame(element, bitmap, rectangle, 2);

		if (!l_ValidateHouse(element))
		{

			return;
		}

		l_UpdateAttachedHumans(element);

		for (std::int32_t index = 0; index < Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::Count); ++index)
		{
			const auto* slot = Slot(element, index);
			NC2E2::DrawAtom_MapMoveable_DoDraw(Read<std::uint32_t>(Human(Read<std::int32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId)), NC2Layout::HumanOffset::MapMoveableId), bitmap, Read<std::int32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::X), Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::BaselineY), nullptr, true, -1);
		}
	}

	// Original function: FUN_004cc1de.
	static bool __fastcall SelectionHouseInhabitantsElement_IsHit(void* element, void*, const std::int32_t* point)
	{
		Write<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HitIndex, -1);

		if (!l_ValidateHouse(element))
			return false;

		l_UpdateAttachedHumans(element);

		for (auto index = Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::Count) - 1; index >= 0; --index)
		{
			const auto* slot = Slot(element, index);
			NC2E2::S2DEngineDisplaySearchObjectBobData data;
			NC2E2::DrawAtom_MapMoveable_GetBobData(Read<std::uint32_t>(Human(Read<std::int32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId)), NC2Layout::HumanOffset::MapMoveableId), nullptr, &data, Read<std::int32_t>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::LocalPlayerId));
			const std::int32_t local[2]{ point[0] - Read<std::int32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::X), point[1] - Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::BaselineY) };

			for (std::uint32_t bob = 0; bob < data.count; ++bob)
			{
				if (NXBasics::BobManager_IsBobHit(data.bobs[bob].manager, data.bobs[bob].id, local))
				{
					Write(element, NC2Layout::HouseInhabitantsSelectionOffset::HitIndex, index);
					return true;
				}
			}
		}

		return false;
	}

	// Original function: FUN_004cc2c1.
	static bool __fastcall SelectionHouseInhabitantsElement_ToolTip_Available(void* element, void*, const char** output)
	{
		const auto index = Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HitIndex);

		if (index == -1)
			return false;

		const auto* slot = Slot(element, index);
		const auto id = Read<std::int32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::HumanId);
		const auto* human = Human(id);
		auto* house = House(Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseId));
		const auto flags = Read<std::uint32_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::RoleMask);
		auto* text = reinterpret_cast<char*>(element) + NC2Layout::HouseInhabitantsSelectionOffset::Tooltip;
		std::strcpy(text, reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + static_cast<std::intptr_t>(id) * NC2Layout::RecordSize::DynamicHuman + NC2Layout::DynamicHumanOffset::Name);
		std::strcat(text, " - ");
		std::strcat(text, NC2Logic::StringAccess_GetJobNamePtr(Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), 1));

		if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType))
		{
			std::strcat(text, " (");
			std::strcat(text, NC2Logic::Human_GetReallyProducedGoodType_GetGuiNamePtr(human));
			std::strcat(text, ")");
		}

		if (flags & SwitchIds::HouseAttachmentMask::WorkerUnsigned)
		{
			std::strcat(text, " - ");
			std::strcat(text, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(340));
		}

		if (flags & SwitchIds::HouseAttachmentMask::BuilderUnsigned)
		{
			std::strcat(text, " - ");
			std::strcat(text, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(341));
		}

		if (flags & SwitchIds::HouseAttachmentMask::HomeUnsigned)
		{
			std::uint32_t families[NC2Runtime::HouseInhabitantsSelectionLimits::HouseAttachments];
			NC2Logic::House_Inhabitants_GetNumberOfFamiliesLivingInHome(house, families);
			const auto slotId = NC2Logic::House_Inhabitants_GetAttachedHumanSlotIdByHumanId(house, id);

			if (slotId != -1)
			{
				char family[100];
				NXBasics::Runtime_PrintString(family, "%s %d", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(342), families[slotId] + 1);
				std::strcat(text, " - ");
				std::strcat(text, family);
			}

			const auto spouse = Read<std::int32_t>(human, NC2Layout::HumanOffset::SpouseId);

			if (spouse != -1 && Read<std::int32_t>(Human(spouse), NC2Layout::HumanOffset::HomeHouseId) == Read<std::int32_t>(element, NC2Layout::HouseInhabitantsSelectionOffset::HouseId))
				std::strcat(text, " **");
		}

		if (!flags && !Read<std::uint8_t>(slot, NC2Layout::HouseInhabitantsSelectionSlotOffset::TraderFlag))
		{
			std::strcat(text, " (");
			std::strcat(text, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(343));
			std::strcat(text, ")");
		}

		*output = text;

		return true;
	}
}