#include "C2GameState.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2MissionScriptData.h"
#include "C2ObjectLayout.h"
#include "C2StaticTribeTypeData.h"
#include "SwitchIdentifiers.h"
#include "XBTokenList.h"
#include "XCrtFileAccess.h"
#include "XIniFileReader.h"
#include <cstring>

namespace NC2Logic_MissionTool
{
	namespace
	{
		struct SMissionParameters
		{
			std::int32_t count;
			std::uint32_t types[10];
		};

		static_assert(sizeof(SMissionParameters) == 44);
		// Complete Windows goal name region: 0050125C..00501357.
		constexpr const char* goalStringPtr[63] = {
			"True",
			"BuildVehicles",
			"BuildHumans",
			"BuildHouses",
			"GoodsInVehicles",
			"GoodsInHouses",
			"GoodsGlobal",
			"FindPos",
			"FindHumans",
			"FindVehicles",
			"FindHouses",
			"FindPosByHumans",
			"FindPosByVehicles",
			"FindHumansByHumans",
			"FindHumansByVehicles",
			"FindVehiclesByVehicles",
			"FindHousesByHumans",
			"FindHousesByVehicles",
			"HumansDied",
			"VehiclesDied",
			"HousesDied",
			"PlayerDied",
			"FindPosByPlayersMapMoveable",
			"BuildHouseOnContinent",
			"DiplomacyState",
			"HumansWithHome",
			"NumberOfSoldiers",
			"DetectGuide",
			"TimeGone",
			"CheckMission",
			"PayTribute",
			"Population",
			"PlayerSeen",
			"IsHumanInVehicle",
			"FindHumansByPlayersMM",
			"SoldiersDied",
			"AnimalsDied",
			"PlayerAttackedByPlayer",
			"JobEnabled",
			"GoodProduceable",
			"NumberOfHumansDied",
			"FindAnimals",
			"NumberOfGoodsTraded",
			"HumanAttachedToWorkHouse",
			"CheckNumberOfWildAnimals",
			"RandomTimeGone",
			"IfMissionIsActive",
			"NumberOfAnimals",
			"NumberOfHumansKilled",
			"HumanIsOnContinent",
			"IsMissionDone",
			"NumberOfSoldiersNearPos",
			"NumberOfCivilainsNearPos",
			"ChestNearPos",
			"NumberOfGoodsInArea",
			"NumberOfHousesInArea",
			"NumberOfGoodsInHousesInArea",
			"NumberOfVehiclesInArea",
			"NumberOfAnimalsInArea",
			"CheckHumanJob",
			"NumberOfGoodsInVehiclesInArea",
			"IsAnyLandscapeOnPoint",
			"IsLandscapePlayer10ConstructionSignOnPoint",
		};
		// Complete Windows goal parameter region: 00501358..00501E2B.
		constexpr SMissionParameters goalParameter[63] = {
			{ 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 0: True
			{ 4, { 1, 5, 7, 12, 0, 0, 0, 0, 0, 0 } },    // 1: BuildVehicles
			{ 4, { 1, 4, 7, 10, 0, 0, 0, 0, 0, 0 } },    // 2: BuildHumans
			{ 4, { 1, 15, 7, 14, 0, 0, 0, 0, 0, 0 } },   // 3: BuildHouses
			{ 3, { 12, 6, 7, 0, 0, 0, 0, 0, 0, 0 } },    // 4: GoodsInVehicles
			{ 3, { 14, 6, 7, 0, 0, 0, 0, 0, 0, 0 } },    // 5: GoodsInHouses
			{ 3, { 1, 6, 7, 0, 0, 0, 0, 0, 0, 0 } },     // 6: GoodsGlobal
			{ 3, { 1, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },   // 7: FindPos
			{ 2, { 1, 10, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 8: FindHumans
			{ 2, { 1, 12, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 9: FindVehicles
			{ 2, { 1, 14, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 10: FindHouses
			{ 4, { 10, 16, 17, 9, 0, 0, 0, 0, 0, 0 } },  // 11: FindPosByHumans
			{ 4, { 12, 16, 17, 9, 0, 0, 0, 0, 0, 0 } },  // 12: FindPosByVehicles
			{ 3, { 10, 11, 9, 0, 0, 0, 0, 0, 0, 0 } },   // 13: FindHumansByHumans
			{ 3, { 12, 10, 9, 0, 0, 0, 0, 0, 0, 0 } },   // 14: FindHumansByVehicles
			{ 3, { 12, 13, 9, 0, 0, 0, 0, 0, 0, 0 } },   // 15: FindVehiclesByVehicles
			{ 3, { 10, 14, 9, 0, 0, 0, 0, 0, 0, 0 } },   // 16: FindHousesByHumans
			{ 3, { 12, 14, 9, 0, 0, 0, 0, 0, 0, 0 } },   // 17: FindHousesByVehicles
			{ 1, { 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 18: HumansDied
			{ 1, { 12, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 19: VehiclesDied
			{ 1, { 14, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 20: HousesDied
			{ 1, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 21: PlayerDied
			{ 4, { 1, 16, 17, 9, 0, 0, 0, 0, 0, 0 } },   // 22: FindPosByPlayersMapMoveable
			{ 4, { 1, 16, 17, 15, 0, 0, 0, 0, 0, 0 } },  // 23: BuildHouseOnContinent
			{ 3, { 1, 2, 25, 0, 0, 0, 0, 0, 0, 0 } },    // 24: DiplomacyState
			{ 2, { 1, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 25: HumansWithHome
			{ 2, { 1, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 26: NumberOfSoldiers
			{ 4, { 1, 16, 17, 9, 0, 0, 0, 0, 0, 0 } },   // 27: DetectGuide
			{ 1, { 35, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 28: TimeGone
			{ 1, { 21, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 29: CheckMission
			{ 1, { 28, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 30: PayTribute
			{ 2, { 1, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 31: Population
			{ 2, { 1, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 32: PlayerSeen
			{ 2, { 10, 12, 0, 0, 0, 0, 0, 0, 0, 0 } },   // 33: IsHumanInVehicle
			{ 3, { 10, 1, 9, 0, 0, 0, 0, 0, 0, 0 } },    // 34: FindHumansByPlayersMM
			{ 2, { 1, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 35: SoldiersDied
			{ 1, { 14, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 36: AnimalsDied
			{ 2, { 2, 1, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 37: PlayerAttackedByPlayer
			{ 3, { 1, 3, 4, 0, 0, 0, 0, 0, 0, 0 } },     // 38: JobEnabled
			{ 3, { 1, 3, 6, 0, 0, 0, 0, 0, 0, 0 } },     // 39: GoodProduceable
			{ 2, { 1, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 40: NumberOfHumansDied
			{ 2, { 1, 14, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 41: FindAnimals
			{ 3, { 1, 2, 7, 0, 0, 0, 0, 0, 0, 0 } },     // 42: NumberOfGoodsTraded
			{ 3, { 1, 4, 7, 0, 0, 0, 0, 0, 0, 0 } },     // 43: HumanAttachedToWorkHouse
			{ 2, { 3, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 44: CheckNumberOfWildAnimals
			{ 1, { 35, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 45: RandomTimeGone
			{ 1, { 21, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 46: IfMissionIsActive
			{ 3, { 1, 3, 7, 0, 0, 0, 0, 0, 0, 0 } },     // 47: NumberOfAnimals
			{ 2, { 1, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 48: NumberOfHumansKilled
			{ 3, { 10, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },  // 49: HumanIsOnContinent
			{ 1, { 21, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 50: IsMissionDone
			{ 5, { 1, 16, 17, 9, 7, 0, 0, 0, 0, 0 } },   // 51: NumberOfSoldiersNearPos
			{ 5, { 1, 16, 17, 9, 7, 0, 0, 0, 0, 0 } },   // 52: NumberOfCivilainsNearPos
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },   // 53: ChestNearPos
			{ 6, { 1, 6, 7, 16, 17, 9, 0, 0, 0, 0 } },   // 54: NumberOfGoodsInArea
			{ 6, { 1, 15, 7, 16, 17, 9, 0, 0, 0, 0 } },  // 55: NumberOfHousesInArea
			{ 6, { 1, 6, 7, 16, 17, 9, 0, 0, 0, 0 } },   // 56: NumberOfGoodsInHousesInArea
			{ 6, { 1, 5, 7, 16, 17, 9, 36, 37, 0, 0 } }, // 57: NumberOfVehiclesInArea
			{ 6, { 1, 3, 7, 16, 17, 9, 0, 0, 0, 0 } },   // 58: NumberOfAnimalsInArea
			{ 2, { 10, 4, 0, 0, 0, 0, 0, 0, 0, 0 } },    // 59: CheckHumanJob
			{ 7, { 1, 5, 6, 7, 16, 17, 9, 0, 0, 0 } },   // 60: NumberOfGoodsInVehiclesInArea
			{ 2, { 16, 17, 0, 0, 0, 0, 0, 0, 0, 0 } },   // 61: IsAnyLandscapeOnPoint
			{ 2, { 16, 17, 0, 0, 0, 0, 0, 0, 0, 0 } },   // 62: IsLandscapePlayer10ConstructionSignOnPoint
		};
		// Complete Windows result name region: 005010C0..0050125B.
		constexpr const char* resultStringPtr[103] = {
			"None",
			"SetHuman",
			"SetVehicle",
			"SetHouse",
			"SetLandscape",
			"RemoveHumans",
			"RemoveVehicles",
			"RemoveHouses",
			"RemoveLandscape",
			"PlayCutscene",
			"ActivateMission",
			"DeactivateMission",
			"MissionWon",
			"MissionFailed",
			"AllowMap",
			"CloseMap",
			"ExploreArea",
			"Exit",
			"SetExternalFlag",
			"SetDiplomacy",
			"SetVisible",
			"ChangeHumanPlayerId",
			"ChangePlayerPlayerId",
			"SendHuman",
			"SendVehicle",
			"DockVehicle",
			"PlaySound",
			"CreateTribute",
			"AddTributeGoods",
			"AllowJob",
			"AllowHouse",
			"AddGoodsToHouses",
			"StartSubMission",
			"EndSubMission",
			"ChangeVehiclesPlayerId",
			"ChangeHousesPlayerId",
			"SetImportHumanFlag",
			"EnableJob",
			"EnableHouse",
			"DisableAll",
			"AddGoodsToVehicle",
			"AddGoodsToAnyStock",
			"AllowGood",
			"EnableGood",
			"ChangePlayerIdInArea",
			"SetDiplomacyNotChangeableFlag",
			"RemoveFXWaveLandscapeInArea",
			"1 Open/0 CloseWallGate",
			"Mission quit and play video",
			"SetPlayerBehaviourFlag",
			"SetHumanBehaviourFlag",
			"SetRandomChestOnRandomPos",
			"RemoveHumansNearPos",
			"StopHumanByPlayerId",
			"SetAnimal",
			"RemoveAnimals",
			"ChangeHumanObjectIdInArea",
			"HealHumansInArea",
			"ClearTribute",
			"SetGuiMarker",
			"SetHumanName",
			"SetWeather",
			"StartEarthquake",
			"SelectHuman",
			"AddGoodsToMapArea",
			"RemoveGoodsFromMapArea",
			"ChangeMissionIdOfHumanInRange",
			"ChangeMissionIdOfPlayer",
			"SetVertexColor",
			"RemoveLandscapesInArea",
			"MoveUnitsInArea",
			"SetHouseExtensionLevel",
			"InfoClear",
			"InfoShowString",
			"InfoCountGoodsInArea",
			"InfoCountHousesInArea",
			"InfoCountHumenInArea",
			"InfoCountSoldiersInArea",
			"InfoCountAnimalsInArea",
			"RemoveFXSmokeLandscapeInArea",
			"ChangeAnimalPlayerIdInArea",
			"RemoveHPsOfHousesInArea",
			"RemoveBlockerLandscapeInArea",
			"RemoveFX1LandscapeInArea",
			"RemoveFX2LandscapeInArea",
			"SetCameraPosition",
			"SetHumanX",
			"RemoveHPsOfHousesInAreaX",
			"SetHouseOverlayState",
			"AttachHumanToVehicle",
			"DetachHumanFromVehicle",
			"SetHouseBuildForbiddenArea",
			"MoveHuman",
			"SetHouseBehaviourFlag",
			"ChangeMissionIdOfVehiclesInRange",
			"RemoveVehiclesWithMissionId",
			"SetImportLandscapeMarker",
			"SetVertexColorOnLand",
			"ChangeMissionIdOfPlayersVehiclesOnContinent",
			"ChangeMissionIdOfVehicles",
			"SetRandomChestOnPosition",
			"SetMapAreaMarker",
			"SetMapAreaMarkerMagic",
		};
		// Complete Windows result parameter region: 00501E30..00502FE3.
		constexpr SMissionParameters resultParameter[103] = {
			{ 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 0: None
			{ 7, { 1, 3, 4, 16, 17, 10, 29, 0, 0, 0 } },   // 1: SetHuman
			{ 7, { 1, 3, 5, 16, 17, 12, 30, 0, 0, 0 } },   // 2: SetVehicle
			{ 7, { 1, 19, 8, 20, 16, 17, 14, 0, 0, 0 } },  // 3: SetHouse
			{ 5, { 16, 17, 18, 8, 32, 0, 0, 0, 0, 0 } },   // 4: SetLandscape
			{ 1, { 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 5: RemoveHumans
			{ 1, { 12, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 6: RemoveVehicles
			{ 1, { 14, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 7: RemoveHouses
			{ 2, { 16, 17, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 8: RemoveLandscape
			{ 2, { 34, 33, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 9: PlayCutscene
			{ 1, { 21, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 10: ActivateMission
			{ 1, { 21, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 11: DeactivateMission
			{ 1, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 12: MissionWon
			{ 1, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 13: MissionFailed
			{ 2, { 31, 22, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 14: AllowMap
			{ 2, { 31, 22, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 15: CloseMap
			{ 4, { 1, 16, 17, 9, 0, 0, 0, 0, 0, 0 } },     // 16: ExploreArea
			{ 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 17: Exit
			{ 3, { 1, 24, 32, 0, 0, 0, 0, 0, 0, 0 } },     // 18: SetExternalFlag
			{ 3, { 1, 2, 25, 0, 0, 0, 0, 0, 0, 0 } },      // 19: SetDiplomacy
			{ 2, { 21, 32, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 20: SetVisible
			{ 2, { 10, 1, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 21: ChangeHumanPlayerId
			{ 2, { 1, 2, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 22: ChangePlayerPlayerId
			{ 3, { 10, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },    // 23: SendHuman
			{ 3, { 12, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },    // 24: SendVehicle
			{ 3, { 12, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },    // 25: DockVehicle
			{ 3, { 26, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },    // 26: PlaySound
			{ 4, { 28, 1, 2, 27, 0, 0, 0, 0, 0, 0 } },     // 27: CreateTribute
			{ 3, { 28, 6, 7, 0, 0, 0, 0, 0, 0, 0 } },      // 28: AddTributeGoods
			{ 3, { 1, 3, 4, 0, 0, 0, 0, 0, 0, 0 } },       // 29: AllowJob
			{ 3, { 1, 3, 15, 0, 0, 0, 0, 0, 0, 0 } },      // 30: AllowHouse
			{ 3, { 14, 6, 7, 0, 0, 0, 0, 0, 0, 0 } },      // 31: AddGoodsToHouses
			{ 2, { 31, 22, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 32: StartSubMission
			{ 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 33: EndSubMission
			{ 2, { 12, 1, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 34: ChangeVehiclesPlayerId
			{ 2, { 14, 1, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 35: ChangeHousesPlayerId
			{ 2, { 10, 32, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 36: SetImportHumanFlag
			{ 3, { 1, 3, 4, 0, 0, 0, 0, 0, 0, 0 } },       // 37: EnableJob
			{ 3, { 1, 3, 15, 0, 0, 0, 0, 0, 0, 0 } },      // 38: EnableHouse
			{ 0, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 39: DisableAll
			{ 3, { 12, 6, 7, 0, 0, 0, 0, 0, 0, 0 } },      // 40: AddGoodsToVehicle
			{ 3, { 1, 6, 7, 0, 0, 0, 0, 0, 0, 0 } },       // 41: AddGoodsToAnyStock
			{ 3, { 1, 3, 6, 0, 0, 0, 0, 0, 0, 0 } },       // 42: AllowGood
			{ 3, { 1, 3, 6, 0, 0, 0, 0, 0, 0, 0 } },       // 43: EnableGood
			{ 5, { 1, 2, 16, 17, 9, 0, 0, 0, 0, 0 } },     // 44: ChangePlayerIdInArea
			{ 3, { 1, 2, 32, 0, 0, 0, 0, 0, 0, 0 } },      // 45: SetDiplomacyNotChangeableFlag
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 46: RemoveFXWaveLandscapeInArea
			{ 4, { 1, 16, 17, 32, 0, 0, 0, 0, 0, 0 } },    // 47: 1 Open/0 CloseWallGate
			{ 1, { 7, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 48: Mission quit and play video
			{ 3, { 1, 7, 32, 0, 0, 0, 0, 0, 0, 0 } },      // 49: SetPlayerBehaviourFlag
			{ 3, { 10, 7, 32, 0, 0, 0, 0, 0, 0, 0 } },     // 50: SetHumanBehaviourFlag
			{ 1, { 7, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 51: SetRandomChestOnRandomPos
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 52: RemoveHumansNearPos
			{ 1, { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },       // 53: StopHumanByPlayerId
			{ 7, { 1, 3, 4, 16, 17, 14, 29, 0, 0, 0 } },   // 54: SetAnimal
			{ 1, { 14, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 55: RemoveAnimals
			{ 5, { 1, 16, 17, 9, 10, 0, 0, 0, 0, 0 } },    // 56: ChangeHumanObjectIdInArea
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 57: HealHumansInArea
			{ 1, { 28, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 58: ClearTribute
			{ 3, { 14, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },    // 59: SetGuiMarker
			{ 2, { 10, 27, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 60: SetHumanName
			{ 5, { 16, 17, 9, 32, 7, 0, 0, 0, 0, 0 } },    // 61: SetWeather
			{ 1, { 35, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 62: StartEarthquake
			{ 2, { 10, 32, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 63: SelectHuman
			{ 7, { 6, 7, 16, 17, 9, 32, 1, 0, 0, 0 } },    // 64: AddGoodsToMapArea
			{ 7, { 6, 7, 16, 17, 9, 32, 1, 0, 0, 0 } },    // 65: RemoveGoodsFromMapArea
			{ 5, { 1, 10, 16, 17, 9, 0, 0, 0, 0, 0 } },    // 66: ChangeMissionIdOfHumanInRange
			{ 2, { 1, 10, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 67: ChangeMissionIdOfPlayer
			{ 4, { 16, 17, 9, 7, 0, 0, 0, 0, 0, 0 } },     // 68: SetVertexColor
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 69: RemoveLandscapesInArea
			{ 6, { 1, 16, 17, 9, 36, 37, 0, 0, 0, 0 } },   // 70: MoveUnitsInArea
			{ 2, { 14, 7, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 71: SetHouseExtensionLevel
			{ 2, { 1, 36, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 72: InfoClear
			{ 3, { 1, 36, 27, 0, 0, 0, 0, 0, 0, 0 } },     // 73: InfoShowString
			{ 8, { 1, 36, 27, 6, 16, 17, 9, 37, 0, 0 } },  // 74: InfoCountGoodsInArea
			{ 8, { 1, 36, 27, 15, 16, 17, 9, 37, 0, 0 } }, // 75: InfoCountHousesInArea
			{ 7, { 1, 36, 27, 16, 17, 9, 37, 0, 0, 0 } },  // 76: InfoCountHumenInArea
			{ 7, { 1, 36, 27, 16, 17, 9, 37, 0, 0, 0 } },  // 77: InfoCountSoldiersInArea
			{ 8, { 1, 36, 27, 3, 16, 17, 9, 37, 0, 0 } },  // 78: InfoCountAnimalsInArea
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 79: RemoveFXSmokeLandscapeInArea
			{ 7, { 1, 3, 16, 17, 9, 7, 2, 0, 0, 0 } },     // 80: ChangeAnimalPlayerIdInArea
			{ 5, { 1, 16, 17, 9, 7, 0, 0, 0, 0, 0 } },     // 81: RemoveHPsOfHousesInArea
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 82: RemoveBlockerLandscapeInArea
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 83: RemoveFX1LandscapeInArea
			{ 3, { 16, 17, 9, 0, 0, 0, 0, 0, 0, 0 } },     // 84: RemoveFX2LandscapeInArea
			{ 2, { 16, 17, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 85: SetCameraPosition
			{ 8, { 1, 3, 4, 16, 17, 10, 29, 7, 0, 0 } },   // 86: SetHumanX
			{ 6, { 1, 16, 17, 9, 7, 14, 0, 0, 0, 0 } },    // 87: RemoveHPsOfHousesInAreaX
			{ 3, { 14, 37, 32, 0, 0, 0, 0, 0, 0, 0 } },    // 88: SetHouseOverlayState
			{ 2, { 10, 12, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 89: AttachHumanToVehicle
			{ 1, { 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },      // 90: DetachHumanFromVehicle
			{ 4, { 16, 17, 9, 32, 0, 0, 0, 0, 0, 0 } },    // 91: SetHouseBuildForbiddenArea
			{ 3, { 10, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },    // 92: MoveHuman
			{ 3, { 14, 36, 32, 0, 0, 0, 0, 0, 0, 0 } },    // 93: SetHouseBehaviourFlag
			{ 5, { 1, 12, 16, 17, 9, 0, 0, 0, 0, 0 } },    // 94: ChangeMissionIdOfVehiclesInRange
			{ 2, { 12, 32, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 95: RemoveVehiclesWithMissionId
			{ 3, { 16, 17, 32, 0, 0, 0, 0, 0, 0, 0 } },    // 96: SetImportLandscapeMarker
			{ 4, { 16, 17, 9, 7, 0, 0, 0, 0, 0, 0 } },     // 97: SetVertexColorOnLand
			{ 4, { 1, 16, 17, 12, 0, 0, 0, 0, 0, 0 } },    // 98: ChangeMissionIdOfPlayersVehiclesOnContinent
			{ 2, { 12, 36, 0, 0, 0, 0, 0, 0, 0, 0 } },     // 99: ChangeMissionIdOfVehicles
			{ 3, { 7, 16, 17, 0, 0, 0, 0, 0, 0, 0 } },     // 100: SetRandomChestOnPosition
			{ 5, { 16, 17, 9, 32, 36, 0, 0, 0, 0, 0 } },   // 101: SetMapAreaMarker
			{ 5, { 16, 17, 9, 32, 36, 0, 0, 0, 0, 0 } },   // 102: SetMapAreaMarkerMagic
		};
	}

	// Original function: FUN_00449d37.
	std::uint32_t Debug_Tool_Mission_GetGoalType(const char* name)
	{
		for (std::uint32_t index = 0; index < 63; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(goalStringPtr[index], name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_00449d66.
	std::uint32_t Debug_Tool_Mission_GetResultType(const char* name)
	{
		for (std::uint32_t index = 0; index < 103; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(resultStringPtr[index], name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_00449d95.
	std::uint32_t Mission_Tool_GetParameter(NXBasics::CIniFileReader& reader, std::uint32_t parameterType)
	{
		if (reader.GetTypeOfNextParameter() == 3)
			return static_cast<std::uint32_t>(reader.GetInt());

		if (reader.GetTypeOfNextParameter() == 2)
		{
			switch (parameterType)
			{
				case SwitchIds::MissionParameterType::Tribe:
					return NC2Logic::Logic_EntryPoint_GetTribeType(reader.GetString());
				case SwitchIds::MissionParameterType::Job:
					return NC2Logic::Logic_EntryPoint_GetJobType(reader.GetString());
				case SwitchIds::MissionParameterType::Vehicle:
					return NC2Logic::Logic_EntryPoint_GetVehicleType(reader.GetString());
				case SwitchIds::MissionParameterType::Good:
					return NC2Logic::Logic_EntryPoint_GetGoodType(reader.GetString());
				case SwitchIds::MissionParameterType::House:
					return NC2Logic::Logic_EntryPoint_GetHouseType(reader.GetString());
				case SwitchIds::MissionParameterType::DiplomacyState:
					return NC2Logic::Logic_EntryPoint_GetDiplomacyState(reader.GetString());
			}
		}
		// Unrecognized string parameter kinds are not consumed by the original.

		return 0;
	}

	// Original function: FUN_00449e34.
	void Mission_Tool_ReadGoalData(NXBasics::CIniFileReader& reader, NC2Logic::SMissionGoal& goal)
	{
		goal.type = reader.GetTypeOfNextParameter() == 2 ? Debug_Tool_Mission_GetGoalType(reader.GetString()) : static_cast<std::uint32_t>(reader.GetInt());
		const auto& descriptor = goalParameter[goal.type];

		for (std::int32_t index = 0; index < descriptor.count; ++index)
		{
			goal.parameters[index] = Mission_Tool_GetParameter(reader, descriptor.types[index]);
		}
	}

	// Original function: FUN_00449e9d.
	void Mission_Tool_ReadResultData(NXBasics::CIniFileReader& reader, NC2Logic::SResult& result)
	{
		result.type = reader.GetTypeOfNextParameter() == 2 ? Debug_Tool_Mission_GetResultType(reader.GetString()) : static_cast<std::uint32_t>(reader.GetInt());
		const auto& descriptor = resultParameter[result.type];

		for (std::int32_t index = 0; index < descriptor.count; ++index)
		{
			const auto parameterType = descriptor.types[index];

			if (parameterType == 18 || parameterType == 19)
			{
				std::strcpy(result.text, reader.GetString());
			}
			else
			{
				result.parameters[index] = Mission_Tool_GetParameter(reader, parameterType);
			}
		}
	}
}

namespace NC2Logic
{
	// Original function: FUN_0040af06.
	std::uint32_t Logic_EntryPoint_GetTribeType(const char* name)
	{
		for (std::uint32_t index = 0; index < 42; ++index)
		{
			if (Tool_AllowedItems_IsTribeTypeAllowed(index) && NXBasics::Runtime_CompareStringsIgnoreCase(reinterpret_cast<const char*>(gStaticTribeTypeData) + index * 0x1AE08u, name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_0040af46.
	std::uint32_t Logic_EntryPoint_GetJobType(const char* name)
	{
		for (std::uint32_t index = 0; index < 56; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(static_cast<const char*>(gStaticJobTypeData) + index * NC2Layout::RecordSize::JobType, name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_0040af7b.
	std::uint32_t Logic_EntryPoint_GetHouseType(const char* name)
	{
		for (std::uint32_t index = 0; index < 55; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(reinterpret_cast<const char*>(gStaticHouseTypeData) + index * 0x624u + 0x5ECu, name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_0040afb5.
	std::uint32_t Logic_EntryPoint_GetVehicleType(const char* name)
	{
		for (std::uint32_t index = 0; index < 7; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(reinterpret_cast<const char*>(gStaticVehicleTypeData) + index * 0x1DCu, name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_0040afea.
	std::uint32_t Logic_EntryPoint_GetGoodType(const char* name)
	{
		for (std::uint32_t index = 0; index < 66; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(reinterpret_cast<const char*>(gStaticGoodTypeData) + index * 0xD0u, name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_0040b10b.
	std::uint32_t Logic_EntryPoint_GetDiplomacyState(const char* name)
	{
		// Complete Windows string-pointer region: 004F3644..004F3657.
		static constexpr const char* diplomacyStateStringPtr[5] = { "none", "friend", "neutral", "enemy", "self" };

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			if (NXBasics::Runtime_CompareStringsIgnoreCase(diplomacyStateStringPtr[index], name) == 0)
				return index;
		}

		return 0;
	}

	// Original function: FUN_00449f2d.
	void MissionData_SetMissionActiveFlag(void* mission, bool active)
	{
		using namespace NC2Lifetime;

		if (Read<std::uint8_t>(mission, NC2Layout::MissionDataOffset::ActiveFlag) == static_cast<std::uint8_t>(active))
		{

			return;
		}

		Write<std::uint8_t>(mission, NC2Layout::MissionDataOffset::ActiveFlag, active);

		if (active)
			Write<std::uint32_t>(mission, NC2Layout::MissionDataOffset::ActivationTick, gGameTime.tick);
	}

	// Original function: FUN_0041d0b6.
	void __fastcall MissionManager_l_LoadMissionData(void* manager, void* /*unusedEdx*/, NXBasics::CIniFileReader* reader)
	{
		using namespace NC2Lifetime;
		// Complete Windows token region: 004F38B8..004F38F7, including the terminator.
		static constexpr NXBasics::SToken sMissionTokenArray[] = { { "description", SwitchIds::MissionScriptToken::Description }, { "active", SwitchIds::MissionScriptToken::Active }, { "successfullif", SwitchIds::MissionScriptToken::Successfullif }, { "goal", SwitchIds::MissionScriptToken::Goal }, { "result", SwitchIds::MissionScriptToken::Result }, { "visible", SwitchIds::MissionScriptToken::Visible }, { "debuginfo", SwitchIds::MissionScriptToken::DebugInfo }, { nullptr, 0 } };
		std::memset(manager, 0, NC2Layout::ObjectSize::MissionManager);

		for (auto* group = reader->GetFirstGroup("MissionData"); group != nullptr; group = reader->GetNextGroup("MissionData"))
		{
			const auto index = Read<std::uint32_t>(manager, NC2Layout::MissionManagerOffset::MissionCount);
			Write<std::uint32_t>(manager, NC2Layout::MissionManagerOffset::MissionCount, index + 1u);
			auto* mission = static_cast<std::byte*>(manager) + NC2Layout::MissionManagerOffset::Missions + index * NC2Layout::RecordSize::MissionDataUnsigned;

			while (reader->GetNextCommandLineInGroup() != nullptr)
			{
				switch (NXBasics::XB_Token_FindTokenInArray(sMissionTokenArray, reader->GetCurrentName()))
				{
					case SwitchIds::MissionScriptToken::Active:
					{
						const bool active = reader->GetInt() != 0;
						Write<std::uint8_t>(mission, NC2Layout::MissionDataOffset::InitiallyActiveFlag, active);
						MissionData_SetMissionActiveFlag(mission, active);
						break;
					}
					case SwitchIds::MissionScriptToken::DebugInfo:
						std::strncpy(reinterpret_cast<char*>(mission), reader->GetString(), 30);
						break;
					case SwitchIds::MissionScriptToken::Description:
						Write<std::int32_t>(mission, NC2Layout::MissionDataOffset::DescriptionId, reader->GetInt());
						break;
					case SwitchIds::MissionScriptToken::Goal:
					{
						SMissionGoal goal{ };
						NC2Logic_MissionTool::Mission_Tool_ReadGoalData(*reader, goal);
						const auto count = Read<std::uint32_t>(mission, NC2Layout::MissionDataOffset::GoalCount);
						std::memcpy(mission + NC2Layout::MissionDataOffset::Goals + count * NC2Layout::RecordSize::MissionGoalUnsigned, &goal, NC2Layout::RecordSize::MissionGoal);
						Write<std::uint32_t>(mission, NC2Layout::MissionDataOffset::GoalCount, count + 1u);
						break;
					}
					case SwitchIds::MissionScriptToken::Result:
					{
						SResult result{ };
						NC2Logic_MissionTool::Mission_Tool_ReadResultData(*reader, result);
						const auto count = Read<std::uint32_t>(mission, NC2Layout::MissionDataOffset::ResultCount);
						std::memcpy(mission + NC2Layout::MissionDataOffset::Results + count * NC2Layout::RecordSize::MissionResultUnsigned, &result, NC2Layout::RecordSize::MissionResult);
						Write<std::uint32_t>(mission, NC2Layout::MissionDataOffset::ResultCount, count + 1u);
						break;
					}
					case SwitchIds::MissionScriptToken::Successfullif:
						Write<std::int32_t>(mission, NC2Layout::MissionDataOffset::GoalMatchMode, reader->GetInt());
						break;
					case SwitchIds::MissionScriptToken::Visible:
						Write<std::uint8_t>(mission, NC2Layout::MissionDataOffset::VisibleFlag, reader->GetInt() != 0);
						break;
				}
			}
		}
	}
}