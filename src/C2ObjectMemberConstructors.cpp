#include "C2LifetimeAccess.h"
#include "C2ObjectLayout.h"
#include "C2ObjectMemberConstructors.h"
#include <cstring>

namespace NC2Logic
{
	using namespace NC2Lifetime;

	// Original function: FUN_004231bd; 250 map records, stride 16, trailing point at +8.
	void* __fastcall GameMapStaticVars_ConstructMembers(void* data, void*)
	{
		for (std::uint32_t index = 0; index < 250; ++index)
		{
			Write<std::uint16_t>(data, index * NC2Layout::RecordSize::GameMapContinentUnsigned + NC2Layout::GameMapContinentArrayViewOffset::FirstSeedPointYUnsigned, 0);
			Write<std::uint16_t>(data, index * NC2Layout::RecordSize::GameMapContinentUnsigned + NC2Layout::GameMapContinentArrayViewOffset::FirstSeedPointXUnsigned, 0);
		}

		return data;
	}

	// Original function: FUN_0040aad7; only the two embedded positions are initialized.
	void* __fastcall House_ConstructMembers(void* house, void* /*unusedEdx*/)
	{
		Write<std::uint16_t>(house, NC2Layout::HouseOffset::MapPositionY, 0);
		Write<std::uint16_t>(house, NC2Layout::HouseOffset::MapPositionX, 0);
		Write<std::uint16_t>(house, NC2Layout::HouseOffset::DoorEntryPointY, 0);
		Write<std::uint16_t>(house, NC2Layout::HouseOffset::DoorEntryPointX, 0);

		return house;
	}

	// Original function: FUN_0040b324.
	void* __fastcall Human_ConstructMembers(void* human, void* /*unusedEdx*/)
	{
		auto* data = static_cast<std::byte*>(human);
		std::memset(data + NC2Layout::HumanOffset::AllowedJobFlagsSigned, 0, 56);
		Human_Task_ConstructMembers(data + NC2Layout::HumanOffset::CurrentTask, nullptr);
		ConstructPointMembers(data + NC2Layout::HumanOffset::InteractionTarget, nullptr);
		ConstructPointMembers(data + NC2Layout::HumanOffset::SecondaryInteractionTarget, nullptr);
		Write<std::uint16_t>(human, NC2Layout::HumanOffset::WorkCenterY, 0);
		Write<std::uint16_t>(human, NC2Layout::HumanOffset::WorkCenterX, 0);
		Human_ConstructMilitaryMembers(data + NC2Layout::HumanOffset::MilitaryState, nullptr);

		return human;
	}

	// Original function: FUN_0040b380; retain each of the 87 native per-task clears.
	void* __fastcall Human_Task_ConstructMembers(void* task, void* /*unusedEdx*/)
	{
		for (std::uint32_t index = 0; index < 87; ++index)
		{
			std::memset(static_cast<std::byte*>(task) + NC2Layout::HumanTaskOffset::UserCommandQueue + index * NC2Layout::RecordSize::UserNetworkCommand, 0, 20);
		}

		return task;
	}

	// Original function: FUN_0040b3a4; the leading mode fields are not cleared here.
	void* __fastcall Human_ConstructMilitaryMembers(void* data, void* /*unusedEdx*/)
	{
		for (std::uint32_t index = 0; index < 2; ++index)
		{
			std::memset(static_cast<std::byte*>(data) + NC2Layout::HumanMilitaryStateOffset::Targets + index * NC2Layout::RecordSize::TargetData, 0, NC2Layout::RecordSize::TargetDataSigned);
		}

		return data;
	}

	// Original function: FUN_0040b6af.
	void* __fastcall Vehicle_ConstructMembers(void* vehicle, void* /*unusedEdx*/)
	{
		Vehicle_ConstructVars(static_cast<std::byte*>(vehicle) + NC2Layout::VehicleOffset::UserCommandQueueSigned, nullptr);
		Vehicle_Task_ConstructMembers(static_cast<std::byte*>(vehicle) + NC2Layout::VehicleOffset::PrimaryMilitaryTarget, nullptr);

		return vehicle;
	}

	// Original function: FUN_0040b6c9.
	void* __fastcall Vehicle_ConstructVars(void* data, void* /*unusedEdx*/)
	{
		for (std::uint32_t index = 0; index < 22; ++index)
		{
			std::memset(static_cast<std::byte*>(data) + index * NC2Layout::RecordSize::UserNetworkCommand, 0, 20);
		}

		Write<std::uint16_t>(data, NC2Layout::VehicleVarsOffset::DoorEntryPointY, 0);
		Write<std::uint16_t>(data, NC2Layout::VehicleVarsOffset::DoorEntryPointX, 0);
		Write<std::uint16_t>(data, NC2Layout::VehicleVarsOffset::TargetWorkPositionY, 0);
		Write<std::uint16_t>(data, NC2Layout::VehicleVarsOffset::TargetWorkPositionX, 0);
		Write<std::uint16_t>(data, NC2Layout::VehicleVarsOffset::MilitaryCenterY, 0);
		Write<std::uint16_t>(data, NC2Layout::VehicleVarsOffset::MilitaryCenterX, 0);

		return data;
	}

	// Original function: FUN_0040b71d; naming reference: CVehicle::SVars::STask.
	void* __fastcall Vehicle_Task_ConstructMembers(void* task, void* /*unusedEdx*/)
	{
		for (std::uint32_t index = 0; index < 2; ++index)
		{
			std::memset(static_cast<std::byte*>(task) + index * NC2Layout::RecordSize::TargetData, 0, NC2Layout::RecordSize::TargetDataSigned);
		}

		return task;
	}

	// Original function: FUN_0041ae3e; shared folded constructor, complete type identity unresolved.
	void* __fastcall ConstructPointMembers(void* data, void* /*unusedEdx*/)
	{
		Write<std::uint16_t>(data, NC2Layout::EmbeddedMapPointPairOffset::FirstY, 0);
		Write<std::uint16_t>(data, NC2Layout::EmbeddedMapPointPairOffset::FirstX, 0);
		Write<std::uint16_t>(data, NC2Layout::EmbeddedMapPointPairOffset::SecondY, 0);
		Write<std::uint16_t>(data, NC2Layout::EmbeddedMapPointPairOffset::SecondX, 0);

		return data;
	}

	// Original function: FUN_0041c12c; Windows passes the animal's state region at +0x18.
	void* __fastcall Animal_ConstructStateMembers(void* state, void* /*unusedEdx*/)
	{
		Write<std::uint16_t>(state, NC2Layout::AnimalStateOffset::WorkCenterY, 0);
		Write<std::uint16_t>(state, NC2Layout::AnimalStateOffset::WorkCenterX, 0);
		Write<std::uint16_t>(state, NC2Layout::AnimalStateOffset::StayPointY, 0);
		Write<std::uint16_t>(state, NC2Layout::AnimalStateOffset::StayPointX, 0);
		Write<std::uint16_t>(state, NC2Layout::AnimalStateOffset::BirthPointY, 0);
		Write<std::uint16_t>(state, NC2Layout::AnimalStateOffset::BirthPointX, 0);
		std::memset(static_cast<std::byte*>(state) + NC2Layout::AnimalStateOffset::MilitaryTarget, 0, NC2Layout::RecordSize::TargetDataSigned);
		std::memset(static_cast<std::byte*>(state) + NC2Layout::AnimalStateOffset::AttachmentTarget, 0, NC2Layout::RecordSize::TargetDataSigned);

		return state;
	}

	// Original function: FUN_0045d3e1; count is initialized separately by callers.
	void* SearchNearbyMapPoints_ConstructMembers(void* data)
	{
		for (std::uint32_t offset = 4; offset < 0x24; offset += 4)
		{
			NC2Lifetime::Write<std::uint16_t>(data, offset + NC2Layout::MapCoordinatePairOffset::Y, 0);
			NC2Lifetime::Write<std::uint16_t>(data, offset, 0);
		}

		return data;
	}

	// Original function: FUN_0045eb8c; player and count are initialized separately by callers.
	void* SearchDamagedWall_ConstructMembers(void* data)
	{
		for (std::uint32_t offset = 8; offset < 0x30; offset += 4)
		{
			NC2Lifetime::Write<std::uint16_t>(data, offset + NC2Layout::MapCoordinatePairOffset::Y, 0);
			NC2Lifetime::Write<std::uint16_t>(data, offset, 0);
		}

		return data;
	}

}