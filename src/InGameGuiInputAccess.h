#pragma once

#include "C2RuntimeConstants.h"

#include <cstddef>
#include <cstdint>

namespace NXBasics
{
	struct SRectangle;
}

namespace NC2InGameGuiInputManager
{
	struct SSelectedObjectReference
	{
		std::int32_t id;
		std::uint32_t uniqueId;
	};

	// The original manager continues with cursor/highlighting state after this prefix.
	struct SInputManagerSelectionData
	{
		std::uint32_t inputMode;
		std::uint32_t selectionType;
		SSelectedObjectReference singleHuman;
		std::uint32_t humanGroupCount;
		SSelectedObjectReference humanGroup[NC2Runtime::InputSelectionLimits::Humans];
		SSelectedObjectReference singleAnimal;
		SSelectedObjectReference singleVehicle;
		std::uint32_t vehicleGroupCount;
		SSelectedObjectReference vehicleGroup[NC2Runtime::InputSelectionLimits::Vehicles];
		SSelectedObjectReference house;
		std::uint32_t houseType;
		std::uint32_t houseDevelopmentStage;
		std::uint32_t houseSpecialItemType;
		std::uint32_t houseSpecialItem;
		SSelectedObjectReference guide;
		std::uint32_t landscapeType;
		std::uint32_t landscapeVariant;
		std::uint32_t landscapeMapPoint;
		std::int32_t landscapePlayerId;
		std::uint32_t roadBuildPoint;
		std::uint32_t wallBuildPoint;
		std::uint32_t selectionChangeCount;
	};

	static_assert(sizeof(SSelectedObjectReference) == 8);
	static_assert(sizeof(SInputManagerSelectionData) == 0xAB4);

	struct SInputManagerCursorState
	{
		std::uint8_t mapHighlightingEnabled;
		std::byte unresolvedByte1;
		std::int16_t cursorX;
		std::int16_t cursorY;
		std::uint8_t acceptedTargetPresent;
		std::byte unresolvedByte7;
		std::uint32_t acceptedTargetMapPoint;
	};

	struct SInputManagerTargetPointList
	{
		std::uint32_t count;
		std::uint32_t points[NC2Runtime::InputTargetPointLimits::Points];
	};

	struct SInputManagerRoadWallPointState
	{
		std::uint32_t roadStartPoint;
		std::uint32_t wallStartPoint;
		SInputManagerTargetPointList targetPoints;
	};

	// Layout documentation only; CInputManager retains its original raw storage.
	struct SInputManagerData
	{
		SInputManagerSelectionData selection;
		SInputManagerCursorState cursor;
		SInputManagerRoadWallPointState roadWallPoints;
	};

	static_assert(sizeof(SInputManagerCursorState) == 12);
	static_assert(sizeof(SInputManagerTargetPointList) == 1204);
	static_assert(sizeof(SInputManagerRoadWallPointState) == 1212);
	static_assert(sizeof(SInputManagerData) == 3964);

	// Complete singleton binding at 00569410; lifecycle is in InGameGuiSupportAccess.cpp.
	extern void* gInputManager;
	std::int32_t Selection_GetSelectedEngineHouseTypeId(const void* manager);
	std::uint32_t Selection_GetSelectedEngineHouseExtensionLevel(const void* manager);
	std::int32_t Selection_GetSelectedEngineLandscapeTypeId(const void* manager);
	std::uint32_t Selection_GetSelectedEngineLandscapeValency(const void* manager);
	const void* Selection_GetSelectedEngineLandscapeMapPointRef(void* manager);
	std::int32_t Selection_GetSelectedEngineLandscapePlayerId(const void* manager);
	const char* StringTool_GetInputModeNamePtr(const void* manager);
	bool InputMode_Set(void* manager, std::int32_t mode);
	void InputMode_Cancel(void* manager);
	bool Misc_House_IsEnemyToAttack(const void* manager, std::int32_t id);
	bool Misc_Human_IsEnemyToAttack(const void* manager, std::int32_t id);
	bool Misc_Animal_CanBeAttacked(const void* manager, std::int32_t id, bool allowOwnAnimals);
	bool Misc_Vehicle_IsEnemyToAttack(const void* manager, std::int32_t id);
	bool Misc_Vehicle_IsToEnter(const void* manager, std::int32_t id);
	void Highlighting_Humans_Update(void* manager, bool force, bool clear);
	void Highlighting_Animals_Update(void* manager, bool force, bool clear);
	void Highlighting_Map_SetState(void* manager, bool enabled);
	void Highlighting_Vehicles_Update(void* manager, bool force, bool clear);
	bool Highlighting_Map_Update(void* manager, const std::int16_t* rectangle);
	bool Action_DoInputSelect(void* manager, const void* object);
	void Selection_UnselectAll(void* manager);
	void Selection_Validate(void* manager);
	bool Selection_SelectSingleHuman(void* manager, std::int32_t id);
	bool Selection_SelectAddSingleHuman(void* manager, std::int32_t id);
	bool Selection_SelectSingleAnimal(void* manager, std::int32_t id);
	bool Selection_SelectSingleVehicle(void* manager, std::int32_t id);
	bool Selection_SelectAddSingleVehicle(void* manager, std::int32_t id);
	bool Selection_SelectHouse(void* manager, std::int32_t id);
	bool Selection_SelectEngineHouseType(void* manager, std::uint32_t type, std::uint32_t developmentStage, std::uint32_t specialItemType, std::uint32_t specialItem);
	bool Selection_SelectEngineLandscapeType(void* manager, std::uint32_t type, std::uint32_t valency, const void* point, std::int32_t player);
	void UnselectAll(void* manager);
	void* GetInputManagerPtr();
	bool Selection_SelectGuide(void* manager, std::int32_t id);
	bool Selection_SelectRoadBuildPlace(void* manager, const void* point);
	bool Selection_SelectWallBuildPlace(void* manager, const void* point);
	const void* Selection_GetSelectedRoadBuildPlacePositionRef(void* manager);
	const void* Selection_GetSelectedWallBuildPlacePositionRef(void* manager);
	void UnSelect_Human(void* manager, std::int32_t id);
	void UnSelect_Animal(void* manager, std::int32_t id);
	void UnSelect_Vehicle(void* manager, std::int32_t id);
	void UnSelect_House(void* manager, std::int32_t id);
	void UnSelect_Guide(void* manager, std::int32_t id);
	std::int32_t Selection_GetSelectedSingleHumanId(const void* manager);
	std::int32_t Selection_GetSelectedSingleAnimalId(const void* manager);
	std::int32_t Selection_GetSelectedGuideId(const void* manager);
	std::uint32_t Selection_GetSelectedMultipleHumanNumber(const void* manager);
	std::int32_t Selection_GetSelectedMultipleHumanId(const void* manager, std::uint32_t index);
	bool Selection_IsHumanSelected(const void* manager, std::int32_t id);
	bool Selection_IsVehicleSelected(const void* manager, std::int32_t id);
	std::int32_t Selection_GetSelectedHouseId(const void* manager);
	bool Selection_IsHouseSelected(const void* manager, std::int32_t id);
	void Cursor_SetPosition(void* manager, const void* point);
	bool Action_DoNormalSelect(void* manager, const void* object, bool keepSelection, bool add, bool remove);
	std::uint32_t Action_NormalSelectPossible(void* manager, const void* object, std::uint32_t* first, std::uint32_t* second, std::uint32_t* third, std::uint32_t* fourth);
	bool Action_DoDefault(void* manager, const void* object);
	bool Action_DoCancel(void* manager, const void* object);
	bool Selection_SelectHumansInArea(void* manager, const void* worldElement, NXBasics::SRectangle rectangle, std::uint32_t job, bool add, bool remove);
	std::int32_t Selection_GetSelectedSingleVehicleId(const void* manager);
	std::uint32_t Selection_GetSelectedMultipleVehiclesNumber(const void* manager);
	std::int32_t Selection_GetSelectedMultipleVehicleId(const void* manager, std::uint32_t index);
	void Misc_Tool_SendSelectedHuman(void* manager, const void* point);
	void Misc_Tool_SendSelectedVehicle(void* manager, std::uint32_t point);
	void l_Selection_FillTargetPointArray(void* manager, const void* point, std::uint32_t radius, std::uint32_t wanted);
}

namespace NC2Logic
{
	// Complete singleton binding at 00510A20; the command queue owner remains original.
	extern void* gUserNetworkCommandManager;
	void UserNetworkCommand_AddVehicleCommand(void* manager, void* command);
	void UserNetworkCommand_AddHouseCommand(void* manager, void* command);
	void UserNetworkCommand_AddHumanCommand(void* manager, void* command);
}