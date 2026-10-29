#include "C2HumanJobAccess.h"
#include "C2HumanMessageAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapMoveableAccess.h"
#include "C2ObjectLayout.h"
#include "C2UserMessageAccess.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_0042c679; a vehicle attachment includes allocation and revision.
	bool Human_IsAttachedToVehicle(const void* human)
	{
		const auto id = Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleId);

		if (id == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* vehicle = CVehicleArray::mStaticVars + id * CVehicleArray::ElementSize;

		return Read<std::uint8_t>(vehicle, NC2Layout::VehicleOffset::AllocatedFlag) != 0 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::AttachedVehicleRevision) == Read<std::uint32_t>(vehicle, NC2Layout::VehicleOffset::UniqueId);
	}

	// Original function: FUN_0042c900.
	bool Human_IsJobNeedingSocial(const void* human)
	{
		const auto job = Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);

		return !((job >= 1 && job <= 4) || (job >= 31 && job <= 41) || (job >= 42 && job <= 47));
	}

	// Original function: FUN_0042c928.
	bool Human_IsJobNeedingReligion(const void* human)
	{

		return Read<std::uint8_t>(gStaticJobTypeData, Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::NeedsReligionUnsigned) != 0;
	}

	namespace
	{
		// Original function: FUN_0042c956.
		bool Human_IsDoingSomething(const void* human)
		{

			return Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 1 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 1;
		}

		// Original function: FUN_0042c9a2.
		bool Human_IsAboutToWalk(const void* human)
		{
			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) != 1)
				return false;

			const auto* moveable = gMapMoveableArrayStorage.entries + Read<std::uint32_t>(human, NC2Layout::HumanOffset::MapMoveableId) * NC2Layout::RecordSize::MapMoveable;

			return Read<std::uint32_t>(MapMoveable_Pathfinder_GetPathfinderPtr(moveable), NC2Layout::MapPathfinderOffset::SearchState) == 1;
		}

		// Original function: FUN_0042c9f0.
		bool Human_IsWorking(const void* human)
		{

			return Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) == 8 && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 1;
		}

	}

	// Original function: FUN_0042ca88.
	bool Human_IsAboutToMarry(const void* human)
	{

		return Read<std::uint32_t>(human, NC2Layout::HumanOffset::WaitingHumanId) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0045c37f; complete Windows selector table 0045c5cd..0045c607.
	bool Human_UserMessage_IsInvalid(const void* human, const void* message)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::PlayerId) != Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId))
			return false;

		if (Human_GetPlayerType(human) == 2 || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & NXBasics::BitMask::Bit4) != 0 || gGameTime.tick < 1u)
			return true;

		switch (Read<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type))
		{
			case SwitchIds::UserMessage::NoWork:
			case SwitchIds::UserMessage::NoWay:
			case SwitchIds::UserMessage::CannotFindGood:
			case SwitchIds::UserMessage::CannotFindBuildGood:
			case SwitchIds::UserMessage::CannotFindHomeGood:
			case SwitchIds::UserMessage::NoTargetHuman:
			case SwitchIds::UserMessage::NoWorkPoint:
				return Human_IsDoingSomething(human) && !Human_IsAboutToWalk(human) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTask) != 0x17;
			case SwitchIds::UserMessage::NoVehicleBuildPlace:
			case SwitchIds::UserMessage::VehicleBuildPlaceOccupied:
			case SwitchIds::UserMessage::TaskCannotBeCompleted:
			case SwitchIds::UserMessage::MustWaitForGood:
			case SwitchIds::UserMessage::ProductionStockFull:
				return Human_IsWorking(human);
			case SwitchIds::UserMessage::CannotAffordTraining:
				return (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 89 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 90) && Read<std::uint32_t>(human, NC2Layout::HumanOffset::CurrentTaskStatus) == 2;
			case SwitchIds::UserMessage::VehicleRequired:
				return Human_IsAttachedToVehicle(human);
			case SwitchIds::UserMessage::TradeAgreementRequired:
				return !Human_IsJobTrader(human) || Read<std::uint32_t>(HumanMerchantArray_GetMerchantDataPtr(gHumanMerchantArray, Read<std::int32_t>(human, NC2Layout::HumanOffset::MerchantId)), NC2Layout::MerchantOffset::TradeAgreementId) != NXBasics::IntegerRange::UInt32Maximum;
			case SwitchIds::UserMessage::Hungry:
				if (Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) <= 0 || Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) <= Human_GetHumanAttributeMargin_Hitpoints(human))
					return true;
				// Read-only Windows thresholds: food fulfillment 004f3b58 = 2000, critical food 004f3b68 = 1000.
				return (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 10 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 11) && Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) > 2000 && Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) > 1000;
			case SwitchIds::UserMessage::Tired:
				// Read-only stamina fulfillment threshold at 004f3b64 = 2000.
				return Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 8 && Read<std::int32_t>(human, NC2Layout::HumanOffset::Stamina) > 2000;
			case SwitchIds::UserMessage::Bored:
				// Read-only social fulfillment threshold at 004f3b60 = 2000.
				return (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 14 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 15) && (Read<std::int32_t>(human, NC2Layout::HumanOffset::Social) > 2000 || !Human_IsJobNeedingSocial(human));
			case SwitchIds::UserMessage::WantsToPray:
				// Read-only religion fulfillment threshold at 004f3b5c = 2000.
				return Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 12 && (Read<std::int32_t>(human, NC2Layout::HumanOffset::Religion) > 2000 || !Human_IsJobNeedingReligion(human));
			case SwitchIds::UserMessage::Starving:
				return (Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 10 || Read<std::uint32_t>(human, NC2Layout::HumanOffset::AtomicAction) == 11) && Read<std::int32_t>(human, NC2Layout::HumanOffset::Food) > 0;
			case SwitchIds::UserMessage::WillDieSoon:
				return Read<std::int32_t>(human, NC2Layout::HumanOffset::Hitpoints) > Human_GetHumanAttributeMargin_Hitpoints(human) || (Read<std::uint32_t>(human, NC2Layout::HumanOffset::MiscFlags) & 8u) != 0;
			case SwitchIds::UserMessage::NoMarriagePartner:
			case SwitchIds::UserMessage::NoWayToMarriagePartner:
				return Human_IsAboutToMarry(human) || Human_IsMarried(human);
			default:

				return false;
		}
	}

	// Original function: FUN_0045bfef.
	void Human_UserMessage_SendToUserMessage_MustWaitForGood(void* human)
	{
		const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType);

		if (Read<std::uint32_t>(gGameControl, NC2Layout::GameControlOffset::DifficultyLevel) == 2 && (good == 16 || good == 17 || good == 19 || good == 20 || good == 21 || good == 23))
		{

			return;
		}

		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 13);
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, good);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c608.
	void Human_UserMessage_SendToUserMessage_DidProduceOneGood(void* human, std::uint32_t good)
	{
		if (good == 0)
		{

			return;
		}

		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 24);
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, good);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c667.
	void Human_UserMessage_SendToUserMessage_DidProduceAllGoods(void* human)
	{
		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 25);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c6b5.
	void Human_UserMessage_SendToUserMessage_CouldNotProduceOneGood(void* human, std::uint32_t good)
	{
		if (good == 0)
		{

			return;
		}

		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 26);
		Write(message, NC2Layout::ToUserMessageOffset::FirstPayloadItem, good);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

	// Original function: FUN_0045c714.
	void Human_UserMessage_SendToUserMessage_CouldNotProduceAnyGoods(void* human)
	{
		std::byte message[772];
		ToUserMessage_Construct(message);
		Write(message, NC2Layout::ToUserMessageOffset::HumanId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::HumanId));
		Write(message, NC2Layout::ToUserMessageOffset::PlayerId, Read<std::uint32_t>(human, NC2Layout::HumanOffset::PlayerId));
		Write<std::uint32_t>(message, NC2Layout::ToUserMessageOffset::Type, 27);
		ToUserMessage_FinishMessage(message);
		NC2LogicExternals::External_Misc_SendToUserMessage(message);
	}

}