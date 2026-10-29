#include "C2EngineDisplayAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2UserMessageAccess.h"
#include "C2VehicleAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiSupportAccess.h"
#include "WorldDisplayElementAccess.h"
#include "WorldDisplayElementAccess.h"
#include "XNumericConstants.h"
#include <cstring>

namespace NC2LogicExternals
{
	// Original function: FUN_004a30ec; failed or unchanged selection prevents starting follow mode.
	void External_Gui_SelectHuman(std::int32_t humanId, bool followOnly)
	{
		using namespace NC2InGameGuiInputManager;

		if (!followOnly && (gInputManager == nullptr || !Selection_SelectSingleHuman(gInputManager, humanId)))
			return;

		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		if (manager == nullptr)
		{

			return;
		}

		void* world = NC2Lifetime::Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);

		if (world != nullptr)
			NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_FollowHuman(world, humanId);
	}

	// Original function: FUN_004a30c0; Windows ignores the second stack argument.
	void External_Misc_SendToUserStringMessage(const char* text, bool /*unused*/)
	{
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		if (manager != nullptr)
			manager->MiscWindows_StringOutputWindow_PrintString(text);
	}
}

namespace NC2Logic
{
	// Original function: FUN_0045c1ea.
	void Human_UserMessage_SendToUserMessage_CanEquipNewItem(void* human, std::uint32_t item)
	{
		using namespace NC2Lifetime;
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 23);
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, item);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c2e3.
	void Human_UserMessage_SendToUserMessage_CannotAttachVehicle(void* human)
	{
		using namespace NC2Lifetime;
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 41);
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c243; Windows ignores the requested attachment mask.
	void Human_UserMessage_SendToUserMessage_CannotAttachHouse(void* human, std::uint32_t mask)
	{
		using namespace NC2Lifetime;
		(void)mask;
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 41);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c138.
	void Human_UserMessage_SendToUserMessage_CanProduceNewGood(void* human, std::uint32_t good)
	{
		using namespace NC2Lifetime;
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 19);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, good);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c191.
	void Human_UserMessage_SendToUserMessage_CanDoNewJob(void* human, std::uint32_t job)
	{
		using namespace NC2Lifetime;
		std::byte message[NC2Layout::RecordSize::ToUserMessage];
		ToUserMessage_Construct(message);
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 20);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write(message, NC2Layout::ToUserMessageOffset::JobItems, job);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0042bf7e; complete 304h-byte SToUserMessage construction.
	void ToUserMessage_Construct(void* message)
	{
		using namespace NC2Lifetime;
		Write<std::uint16_t>(message, NC2Layout::ToUserMessageOffset::MapPositionY, 0);
		Write<std::uint16_t>(message, NC2Layout::ToUserMessageOffset::MapPositionX, 0);
		std::memset(message, 0, NC2Layout::RecordSize::ToUserMessage);

		for (const auto offset : { NC2Layout::ToUserMessageOffset::FirstPayloadItemUnsigned, NC2Layout::ToUserMessageOffset::JobItemsUnsigned, NC2Layout::ToUserMessageOffset::HouseItemsUnsigned, NC2Layout::ToUserMessageOffset::EquipmentItemsUnsigned })
		{
			Write<std::uint32_t>(message, offset, 0);
		}

		for (const auto offset : { NC2Layout::ToUserMessageOffset::PlayerIdUnsigned, NC2Layout::ToUserMessageOffset::HumanIdUnsigned, NC2Layout::ToUserMessageOffset::HouseIdUnsigned, NC2Layout::ToUserMessageOffset::VehicleIdUnsigned })
		{
			Write<std::uint32_t>(message, offset, NXBasics::IntegerRange::UInt32Maximum);
		}

		Write<std::uint16_t>(message, NC2Layout::ToUserMessageOffset::MapPositionX, NXBasics::IntegerRange::UInt16MaximumSigned);
		Write<std::uint16_t>(message, NC2Layout::ToUserMessageOffset::MapPositionY, NXBasics::IntegerRange::UInt16MaximumSigned);
	}

	// Original function: FUN_0042bfcc; referenced object IDs are deliberately unchecked.
	void ToUserMessage_FinishMessage(void* message)
	{
		using namespace NC2Lifetime;
		const auto human = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanId);

		if (human != NXBasics::IntegerRange::UInt32Maximum)
			Write(message, NC2Layout::ToUserMessageOffset::HumanUniqueId, Read<std::uint32_t>(CHumanArray::mStaticVars, human * CHumanArray::ElementSize + NC2Layout::ActorIdentityOffset::UniqueId));

		const auto house = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseId);

		if (house != NXBasics::IntegerRange::UInt32Maximum)
			Write(message, NC2Layout::ToUserMessageOffset::HouseUniqueId, Read<std::uint32_t>(CHouseArray::mStaticVars, house * CHouseArray::ElementSize + NC2Layout::ActorIdentityOffset::UniqueId));

		const auto vehicle = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::VehicleId);

		if (vehicle != NXBasics::IntegerRange::UInt32Maximum)
			Write(message, NC2Layout::ToUserMessageOffset::VehicleUniqueId, Read<std::uint32_t>(CVehicleArray::mStaticVars, vehicle * CVehicleArray::ElementSize + NC2Layout::ActorIdentityOffset::UniqueId));

		Write<std::uint8_t>(message, NC2Layout::ToUserMessageOffset::FinishedFlag, 1);
	}

	// Original function: FUN_0042c022; object IDs remain unchecked as in Windows.
	bool ToUserMessage_IsValid(const void* message, bool checkHuman)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint8_t>(message, NC2Layout::ToUserMessageOffset::FinishedFlag) == 0)
			return false;

		const auto humanId = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanId);

		if (humanId != NXBasics::IntegerRange::UInt32Maximum)
		{
			auto* human = CHumanArray::mStaticVars + humanId * CHumanArray::ElementSize;

			if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) != Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanUniqueId))
				return false;

			if (checkHuman && Human_UserMessage_IsInvalid(human, message))
				return false;
		}

		const auto houseId = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseId);

		if (houseId != NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto* house = CHouseArray::mStaticVars + houseId * CHouseArray::ElementSize;

			if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(house, NC2Layout::HouseOffset::UniqueId) != Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HouseUniqueId))
				return false;
		}

		const auto vehicleId = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::VehicleId);

		if (vehicleId != NXBasics::IntegerRange::UInt32Maximum)
		{
			const auto* vehicle = CVehicleArray::mStaticVars + vehicleId * CVehicleArray::ElementSize;

			if (Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId) != Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::VehicleUniqueId))
				return false;
		}

		return true;
	}

	// Original function: FUN_0042c0ae; padding bytes participate in equality.
	bool ToUserMessage_IsEqualTo(const void* message, const void* other)
	{

		return std::memcmp(message, other, 0x304) == 0;
	}

	// Original function: FUN_0042c0c8; original spelling retained from the named reference.
	bool ToUserMessage_IsSimmilarTo(const void* message, const void* other)
	{
		using namespace NC2Lifetime;
		const auto type = Read<std::int32_t>(other, NC2Layout::ToUserMessageOffset::Type);

		if (type < 4 || (type > 6 && type != 0x2C) || Read<std::int32_t>(message, NC2Layout::ToUserMessageOffset::Type) != type)
			return false;

		const auto player = Read<std::uint32_t>(other, NC2Layout::ToUserMessageOffset::PlayerId);

		if (player == NXBasics::IntegerRange::UInt32Maximum || player != Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::PlayerId) || Read<std::uint32_t>(other, NC2Layout::ToUserMessageOffset::FirstPayloadItem) != Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem))
			return false;

		const auto otherId = Read<std::uint32_t>(other, NC2Layout::ToUserMessageOffset::HumanId);

		if (otherId == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* otherHuman = CHumanArray::mStaticVars + otherId * CHumanArray::ElementSize;

		if (Read<std::uint8_t>(otherHuman, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(otherHuman, NC2Layout::HumanOffset::UniqueId) != Read<std::uint32_t>(other, NC2Layout::ToUserMessageOffset::HumanUniqueId))
			return false;

		const auto humanId = Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanId);

		if (humanId == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* human = CHumanArray::mStaticVars + humanId * CHumanArray::ElementSize;

		if (Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) == 0 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) != Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::HumanUniqueId))
			return false;

		std::uint32_t otherPoint = 0, point = 0;
		Human_GetMapPosition(otherHuman, &otherPoint);
		Human_GetMapPosition(human, &point);

		if (!MapMigPoint_IsInsideMap(&otherPoint) || !MapMigPoint_IsInsideMap(&point))
			return false;

		return static_cast<std::uint32_t>(VE_HexagonDirection_GetDistance(static_cast<std::int16_t>(otherPoint), static_cast<std::int16_t>(otherPoint >> 16), static_cast<std::int16_t>(point), static_cast<std::int16_t>(point >> 16))) <= 20u;
	}

	// Original function: FUN_004b3f37; the copy constructor leaves bytes 5..7 untouched.
	void ToUserMessage_CopyConstruct(void* message, const void* source)
	{
		NC2Lifetime::Write(message, NC2Layout::ToUserMessageOffset::Type, NC2Lifetime::Read<std::uint32_t>(source, NC2Layout::ToUserMessageOffset::Type));
		NC2Lifetime::Write(message, NC2Layout::ToUserMessageOffset::FinishedFlag, NC2Lifetime::Read<std::uint8_t>(source, NC2Layout::ToUserMessageOffset::FinishedFlag));
		std::memcpy(static_cast<std::byte*>(message) + 8, static_cast<const std::byte*>(source) + 8, 764);
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_004a30a3.
	void External_Misc_RemoveUserMessage(std::uint32_t humanId, std::uint32_t type)
	{
		if (NInGameGuiManager::CInGameGuiManager::sTheObjectPtr != nullptr)
			static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->Message_RemoveHumanMessage(humanId, type);
	}

	// Original function: FUN_004a307c; only the local player's or unowned messages pass.
	void External_Misc_SendToUserMessage(const void* message)
	{
		const auto* gui = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		if (gui == nullptr)
			return;

		const auto player = NC2Lifetime::Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::PlayerId);

		if (player != NXBasics::IntegerRange::UInt32Maximum && player != NC2Lifetime::Read<std::uint32_t>(gui, NC2Layout::InGameGuiManagerOffset::LocalPlayerId))
		{

			return;
		}

		static_cast<NC2InGameGuiMessages::CMessageManager*>(NC2InGameGuiMessages::gMessageManager)->Message_Add(message);
	}
}

namespace NC2LogicExternals
{
	// Original function: FUN_004a30d4.
	void External_Gui_SetMarker(std::uint32_t index, const std::uint32_t* point)
	{
		if (NInGameGuiManager::CInGameGuiManager::sTheObjectPtr != nullptr)
			NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GuiMarker_Set(index, point);
	}

	// Original function: FUN_004a3149; ending follow mode precedes the second manager lookup.
	void External_Camera_SetToPosition(const std::uint32_t* point)
	{
		using namespace NC2Lifetime;
		auto* gui = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

		if (gui == nullptr)
			return;

		auto* world = Read<void*>(gui, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);

		if (world == nullptr || Read<void*>(world, NC2Layout::WorldGuiElementOffset::Display) == nullptr)
		{

			return;
		}

		NC2InGameGuiWorldElement::WorldDisplayElement_FollowMode_End(world);
		world = Read<void*>(NInGameGuiManager::CInGameGuiManager::sTheObjectPtr, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);
		NC2E2::DE_SetWantedPositionToMapPoint(Read<void*>(world, NC2Layout::WorldGuiElementOffset::Display), point);
	}
}