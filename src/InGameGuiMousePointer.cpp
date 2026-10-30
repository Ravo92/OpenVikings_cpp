#include "XGuiElementAccess.h"
#include "C2EngineDisplayAccess.h"
#include "XTextEncoding.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineLandscapeDrawing.h"
#include "C2EngineObjectData.h"
#include "C2HouseAccess.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StringAccess.h"
#include "C2WallRoadPathfinder.h"
#include "GuiToolsBase.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiMousePointer.h"
#include "InGameGuiSelectionWindowAccess.h"
#include "MasterControlProgram.h"
#include "SwitchIdentifiers.h"
#include "WorldDisplayElementAccess.h"
#include "XBasicTypes.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XFontMetrics.h"
#include "XGuiDesktop.h"
#include "XMMouseManager.h"
#include <cstring>

namespace NC2InGameGuiManager
{
	using namespace NC2Lifetime;

	// Complete lazy tooltip-shadow region 0056944C..00569453.
	struct STooltipShadowState
	{
		std::uint32_t initialized = 0;
		NXBasics::SColorRGB color{ 0, 0, 0 };
	};

	static STooltipShadowState gTooltipShadow;
	static_assert(sizeof(STooltipShadowState) == 8);

	// Original function: FUN_004bb8fb; the native color destructor is empty.
	static int __cdecl TooltipShadow_Exit()
	{

		return 0;
	}

	// Original function: FUN_004bb7c4. Outline offsets are the complete 004F7000..004F701F region.
	void gToolTip_PrintStringSingleLine(NXBasics::CBitmap* bitmap, std::int32_t* point, const char* text, bool bright)
	{
		auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		auto* font = resources.font08;
		const auto width = NXBasics::Font_GetPixelWidth(font, text);
		NXBasics::SRectangle rectangle(point[0] - width / 2 - 2, point[1], width + 4, NXBasics::Font_GetPixelHeight(font, text) + 1);
		rectangle.PlaceInside(bitmap->GetRectangle());
		bitmap->Tool_Darken(rectangle);
		{
			NXBasics::CBitmap reference(bitmap, rectangle);
			NC2GuiToolsBase::DrawTool_FillArea(nullptr, &reference, &rectangle, 2);
		}

		if ((gTooltipShadow.initialized & 1u) == 0)
		{
			gTooltipShadow.initialized |= 1u;
			gTooltipShadow.color.SetColors(0, 0, 0);
			NXBasics::Runtime_AtExit(TooltipShadow_Exit);
		}

		constexpr std::int32_t offsetX[4]{ -1, 1, 0, 0 }, offsetY[4]{ 0, 0, -1, 1 };

		for (std::uint32_t i = 0; i < 4; ++i)
		{
			bitmap->Text_Print(font, &gTooltipShadow.color, rectangle.x + 2 + offsetX[i], rectangle.y + offsetY[i], text);
		}

		const auto* palette = bright ? resources.fontWhitePalette : resources.fontDimmedPalette;
		const auto* color = reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(palette) + NC2Layout::PaletteOffset::LastColor);
		bitmap->Text_Print(font, color, rectangle.x + 2, rectangle.y, text);
		point[1] += rectangle.height;
	}

	// Original function: FUN_004bc24c. Only CRLF splits lines; the native multiline buffer is 400 bytes.
	void ls_PrintString(NXBasics::CBitmap* bitmap, std::int32_t* point, const char* text, bool bright)
	{
		if (std::strstr(text, "\r\n") == nullptr)
		{
			gToolTip_PrintStringSingleLine(bitmap, point, text, bright);

			return;
		}

		char buffer[NC2Runtime::GuiMouseTooltipLimits::MultilineBufferBytes];
		NXBasics::Text_Copy(buffer, sizeof(buffer), text);
		buffer[NC2Runtime::GuiMouseTooltipLimits::MultilineTerminator] = 0;
		char* line = buffer;

		for (;;)
		{
			char* separator = std::strstr(line, "\r\n");

			if (separator != nullptr)
				*separator = 0;

			gToolTip_PrintStringSingleLine(bitmap, point, line, bright);

			if (separator == nullptr)
				break;

			*separator = '\r';
			line = separator + 2;
		}
	}

	namespace
	{
		const char* HumanName(std::int32_t id)
		{

			return reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + static_cast<std::intptr_t>(id) * NC2Layout::RecordSize::DynamicHuman + NC2Layout::DynamicHumanOffset::Name;
		}

		// The two native scheme branches share this presentation sequence; their selection checks remain distinct.
		void PrintHumanInformation(NXBasics::CBitmap* bitmap, std::int32_t* point, std::int32_t id)
		{
			const auto* human = NC2Logic::GetHumanPtr(id);

			if (human == nullptr)
				return;

			const char* name = HumanName(id);
			const char* job = nullptr;

			if (NC2Logic::Human_IsSexMale(human) && NC2Logic::Human_IsAgeAdult(human) && !NC2Logic::Human_IsJobHero(human))
				job = NC2Logic::StringAccess_GetJobNamePtr(Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType), 1);

			const auto good = Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGood);
			const char* goodName = good != 0 ? NC2Logic::StringAccess_GetGoodNamePtr(good, 1) : nullptr;
			char text[200];
			std::strcpy(text, NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(60));
			std::strcat(text, " ");
			std::strcat(text, name != nullptr ? name : NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(61));

			if (job != nullptr)
			{
				std::strcat(text, " - ");
				std::strcat(text, job);
			}

			if (goodName != nullptr)
			{
				std::strcat(text, " (");
				std::strcat(text, goodName);
				std::strcat(text, ")");
			}

			ls_PrintString(bitmap, point, text, true);
			auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;

			if (Read<std::uint8_t>(manager, NC2Layout::InGameGuiManagerOffset::TooltipsDisabled) != 0 || !NC2Logic::Human_IsAgeAdult(human) || !NC2Logic::Human_IsSexMale(human) || NC2Logic::Human_IsJobHero(human))
			{

				return;
			}

			const auto experience = NC2Logic::Human_JobEfficiency_CalculateExperienceFactor(human);
			NXBasics::Runtime_PrintString(text, "%s: %d%%", NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(63), experience);
			ls_PrintString(bitmap, point, text, true);
		}
	}

	// Original function: FUN_004bb8fc; complete Windows tooltip and hardware/software cursor dispatch.
	void __fastcall GuiManagerMousePointer_DrawMouse(void* pointer, void*, NXBasics::CBitmap* bitmap, const std::int32_t* mousePoint)
	{
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		auto* input = NC2InGameGuiInputManager::gInputManager;
		auto* mouse = NXSysMouseManager::CMouseManager::sTheObjectPtr;
		auto* control = NMasterControlProgram::CMasterControlProgram::sTheObjectPtr;
		using TMouseCallback = bool(__cdecl*)();
		const bool software = Read<std::uint8_t>(manager, NC2Layout::InGameGuiManagerOffset::SoftwareMouse) != 0;

		if (!software)
		{
			if (Read<std::uint8_t>(control, NC2Layout::MasterControlOffset::HardwareMouseCursorEnabled) == 0)
				control->HardwareMouseCursor_Enable();

			const auto callback = Read<TMouseCallback>(mouse, NC2Layout::MouseManagerOffset::PositionOverrideCallback);
			const auto mode = callback != nullptr && callback() ? 2 : Read<std::uint8_t>(mouse, NC2Layout::MouseManagerOffset::LeftButtonDown) != 0 ? 1
			                                                                                                                                        : 0;
			control->HardwareMouseCursor_SetMode(static_cast<NMasterControlProgram::TMouseCursorModes>(mode));
		}
		else if (Read<std::uint8_t>(control, NC2Layout::MasterControlOffset::HardwareMouseCursorEnabled) != 0)
		{
			control->HardwareMouseCursor_Disable();
		}

		std::int32_t point[2]{ mousePoint[0], mousePoint[1] + 20 };
		auto* underMouse = NXBaseGui::CDesktop::sTheObjectPtr->Element_GetUnderMousePtr();

		if (underMouse != nullptr)
		{
			using TAvailable = bool(__thiscall*)(void*, const char**);
			using TImmediate = bool(__thiscall*)(void*);
			const auto* methods = Read<const std::uintptr_t*>(underMouse, NC2Layout::GuiBaseElementOffset::VirtualTable);
			const char* tooltip = nullptr;

			if (reinterpret_cast<TAvailable>(methods[NC2Runtime::GuiVirtualSlot::TooltipAvailable])(underMouse, &tooltip))
			{
				const auto now = timeGetTime();

				if (tooltip != Read<const char*>(pointer, NC2Layout::GuiMousePointerOffset::TooltipText) || mousePoint[0] != Read<std::int32_t>(pointer, NC2Layout::GuiMousePointerOffset::TooltipMouseX) || mousePoint[1] != Read<std::int32_t>(pointer, NC2Layout::GuiMousePointerOffset::TooltipMouseY))
				{
					Write(pointer, NC2Layout::GuiMousePointerOffset::TooltipDeadlineTick, now);

					if (!reinterpret_cast<TImmediate>(methods[NC2Runtime::GuiVirtualSlot::ImmediateTooltip])(underMouse))
						Write(pointer, NC2Layout::GuiMousePointerOffset::TooltipDeadlineTick, now + NC2Runtime::GuiMouseTiming::TooltipDelayMillisecondsUnsigned);

					Write(pointer, NC2Layout::GuiMousePointerOffset::TooltipText, tooltip);
					Write(pointer, NC2Layout::GuiMousePointerOffset::TooltipMouseX, mousePoint[0]);
					Write(pointer, NC2Layout::GuiMousePointerOffset::TooltipMouseY, mousePoint[1]);
				}

				if (now >= Read<std::uint32_t>(pointer, NC2Layout::GuiMousePointerOffset::TooltipDeadlineTick))
					ls_PrintString(bitmap, point, Read<const char*>(pointer, NC2Layout::GuiMousePointerOffset::TooltipText), true);
			}
			else
			{
				Write<const char*>(pointer, NC2Layout::GuiMousePointerOffset::TooltipText, nullptr);
			}
		}

		auto* world = Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement);

		if (underMouse == world)
		{
			const auto mode = Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::InputMode);

			if (mode != 0)
				ls_PrintString(bitmap, point, NC2InGameGuiInputManager::StringTool_GetInputModeNamePtr(input), true);

			if (mode == 12)
				ls_PrintString(bitmap, point, NC2Logic::StringAccess_GetHouseNamePtr(NC2InGameGuiInputManager::Selection_GetSelectedEngineHouseTypeId(input), NC2InGameGuiInputManager::Selection_GetSelectedEngineHouseExtensionLevel(input), 1), true);

			if (mode == 0)
			{
				bool showHovered = !NC2InGameGuiWorldElement::WorldDisplayElement_MouseSelectingArea_IsAreaValid(world);
				auto* target = static_cast<const std::byte*>(world) + NC2Layout::WorldGuiElementOffset::ObjectUnderMouse;

				if (showHovered)
				{
					const auto humanAction = Read<std::uint32_t>(world, NC2Layout::WorldGuiElementOffset::HumanInteractionAction);

					if (humanAction != 0)
					{
						char text[200]{ };

						if (Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::SelectionType) == 1)
						{
							const auto* name = HumanName(NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(input));

							if (name != nullptr)
							{
								std::strcpy(text, name);
								std::strcat(text, " - ");
							}
						}

						std::strcat(text, NC2GuiToolsBase::StringTool_GetLogicUserCommandsForHumanTypeNamePtr(humanAction));

						if (humanAction == 24 && Read<std::uint8_t>(world, NC2Layout::WorldGuiElementOffset::PrimaryCommand) == 21 && Read<std::uint8_t>(world, NC2Layout::WorldGuiElementOffset::SecondaryCommand) == 28)
						{
							std::strcat(text, " - ");
							std::strcat(text, NC2Logic::StringAccess_GetJobNamePtr(Read<std::uint32_t>(world, NC2Layout::WorldGuiElementOffset::PrimaryCommandParameter1), 1));
							std::strcat(text, " - ");
							std::strcat(text, NC2GuiToolsBase::StringTool_GetLogicUserCommandsForHumanTypeNamePtr(16));
						}

						ls_PrintString(bitmap, point, text, true);
						showHovered = false;
					}

					const auto vehicleAction = Read<std::uint32_t>(world, NC2Layout::WorldGuiElementOffset::VehicleInteractionAction);

					if (vehicleAction != 0)
					{
						ls_PrintString(bitmap, point, NC2GuiToolsBase::StringTool_GetLogicUserCommandsForVehicleTypeNamePtr(vehicleAction), true);
						showHovered = false;
					}

					if (showHovered)
					{
						const auto scheme = Read<std::uint32_t>(manager, NC2Layout::InGameGuiManagerOffset::MainMode);

						if (scheme == 0 || scheme == 1)
						{
							std::uint32_t first, second, third, fourth;
							const auto selection = NC2InGameGuiInputManager::Action_NormalSelectPossible(input, target, &first, &second, &third, &fourth);

							if (selection == 1)
							{
								PrintHumanInformation(bitmap, point, static_cast<std::int32_t>(first));
								showHovered = false;
							}
							else if (selection == 6)
							{
								const auto* house = NC2Logic::CHouseArray::mStaticVars + first * NC2Logic::CHouseArray::RuntimeElementSize;

								if (scheme == 0)
								{
									const char* name = Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0 ? NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1) : nullptr;
									char text[200];
									std::strcpy(text, NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(60));
									std::strcat(text, " ");
									std::strcat(text, name != nullptr ? name : NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(62));
									ls_PrintString(bitmap, point, text, true);
								}
								// Visible foreign buildings disclose their name regardless of purpose or civilization.
								else if (Read<std::uint8_t>(house, NC2Layout::HouseOffset::AllocatedFlag) != 0)
								{
									const char* name = NC2Logic::StringAccess_GetHouseNamePtr(Read<std::uint32_t>(house, NC2Layout::HouseOffset::LogicType), 1);

									if (name != nullptr)
										ls_PrintString(bitmap, point, name, false);
								}
							}
							else if (scheme == 0)
							{
								std::uint32_t stringId = 0;

								switch (selection)
								{
									case SwitchIds::SelectionType::SingleAnimal:
										stringId = 64;
										break;
									case SwitchIds::SelectionType::SingleVehicle:
										stringId = 65;
										break;
									case SwitchIds::SelectionType::Guide:
										stringId = 66;
										break;
									case SwitchIds::SelectionType::Landscape:
										stringId = 67;
										break;
									case SwitchIds::SelectionType::RoadBuildPlace:
										stringId = 68;
										break;
									case SwitchIds::SelectionType::WallBuildPlace:
										stringId = 69;
										break;
								}

								if (stringId != 0)
									ls_PrintString(bitmap, point, NC2GuiToolsBase::StringTool_GetMiscMenuStringPtr(stringId), true);
							}
						}
					}
				}

				if (Read<std::uint8_t>(manager, NC2Layout::InGameGuiManagerOffset::TooltipsDisabled) == 0 && showHovered)
				{
					const auto type = Read<std::uint32_t>(target, NC2Layout::DisplayObjectReferenceOffset::Type);

					if (type == 1 && NC2Logic::GetHumanPtr(Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId)) != nullptr)
						ls_PrintString(bitmap, point, HumanName(Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::HumanId)), true);

					if (type == 6)
					{
						char text[200];
						NC2InGameGuiSelection::Tool_LandscapeType_GetTooltipStringPtr(Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::LandscapeId), Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::LandscapeVariant), text, nullptr, nullptr);

						if (text[0] != 0)
							ls_PrintString(bitmap, point, text, true);
					}

					if (type == 2)
					{
						const auto* animal = NC2Logic::GetAnimalPtr(Read<std::int32_t>(target, NC2Layout::DisplayObjectReferenceOffset::AnimalId));

						if (animal != nullptr)
							ls_PrintString(bitmap, point, NC2Logic::StringAccess_GetTribeNamePtr(Read<std::uint32_t>(animal, NC2Layout::AnimalOffset::TribeType), 1), true);
					}
				}
			}
		}

		if (software)
		{
			// Windows reloads the callback after the first call; it may have replaced itself.
			auto callback = Read<TMouseCallback>(mouse, NC2Layout::MouseManagerOffset::PositionOverrideCallback);

			if (callback != nullptr)
				callback();

			callback = Read<TMouseCallback>(mouse, NC2Layout::MouseManagerOffset::PositionOverrideCallback);
			const auto bob = callback != nullptr && callback() ? SwitchIds::SoftwareMouseGraphic::PositionOverrideUnsigned : Read<std::uint8_t>(mouse, NC2Layout::MouseManagerOffset::LeftButtonDown) != 0 ? SwitchIds::SoftwareMouseGraphic::LeftButtonDownUnsigned
			                                                                                                                                                                                               : SwitchIds::SoftwareMouseGraphic::NormalUnsigned;
			auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
			NXBasics::BobManager_PrintBob(resources.windowBobManager, bob, bitmap, mousePoint[0], mousePoint[1], resources.iconsLeftPalette);
		}
	}

	static void* __fastcall GuiManagerMousePointer_Delete(void* pointer, void*, std::uint32_t flags);
	static const std::uintptr_t gMousePointerMethods[2]{ reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GuiManagerMousePointer_DrawMouse>()), reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&GuiManagerMousePointer_Delete>()) };

	// Original function: FUN_004bb7a7; scalar deleting wrapper.
	static void* __fastcall GuiManagerMousePointer_Delete(void* pointer, void*, std::uint32_t flags)
	{
		Write(pointer, NC2Layout::GuiMousePointerOffset::MethodTable, gMousePointerMethods);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(pointer);

		return pointer;
	}
}

// Original function: FUN_004bb78a; complete 004F7020 two-slot table, 0x14-byte object.
void* __fastcall InGameGuiManagerMousePointer_Construct(void* pointer)
{
	NC2Lifetime::Write(pointer, NC2Layout::GuiMousePointerOffset::MethodTable, NC2InGameGuiManager::gMousePointerMethods);
	std::memset(static_cast<std::byte*>(pointer) + NC2Layout::GuiMousePointerOffset::TooltipDeadlineTick, 0, NC2Layout::RecordSize::GuiMousePointerVariables);

	return pointer;
}

namespace NC2InGameGuiWorldGameCursor
{
	// Complete lazy marker-color region 005693F8..00569403.
	struct SMarkerColorState
	{
		NXBasics::SColorRGB white{ 0, 0, 0 };
		std::uint32_t initialized = 0;
		NXBasics::SColorRGB black{ 0, 0, 0 };
	};

	static SMarkerColorState gMarkerColors;
	static_assert(sizeof(SMarkerColorState) == 12);

	// Original function: FUN_004b3757.
	static int __cdecl MarkerWhite_Exit()
	{

		return 0;
	}

	// Original function: FUN_004b3758.
	static int __cdecl MarkerBlack_Exit()
	{

		return 0;
	}

	// Original function: FUN_004b3264; complete road/wall vector previews.
	bool __cdecl IGG_DrawWorldGameCursor_LogicCallback(std::uint32_t type, std::uintptr_t, std::uintptr_t, std::uintptr_t, std::uintptr_t)
	{
		using namespace NC2Lifetime;

		if (type != 0x19)
			return true;

		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		auto* display = Read<void*>(Read<void*>(manager, NC2Layout::InGameGuiManagerOffset::WorldDisplayElement), NC2Layout::WorldGuiElementOffset::Display);
		auto* input = NC2InGameGuiInputManager::gInputManager;
		const auto mode = Read<std::uint32_t>(input, NC2Layout::InputManagerOffset::InputMode);
		const auto player = manager->GetLocalPlayerId();
		const auto marker = [&](const void* position)
		{
			std::int32_t pixel[2];
			NC2E2::Display_MapMigToLocalDisplayPixelCoordinates(display, position, pixel, false);
			auto* bitmap = Read<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap);
			auto* bobManager = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.windowBobManager;
			auto* palette = NC2E2::EnginePalette_GetPlayerPalette(player);
			NXBasics::BobManager_PrintBob(bobManager, SwitchIds::WorldCursorGraphic::MarkerFirstFrameUnsigned + (timeGetTime() / NC2Runtime::GuiMouseTiming::MarkerFrameMillisecondsUnsigned) % NC2Runtime::GuiMouseWorldCursorLimits::MarkerFramesUnsigned, bitmap, pixel[0], pixel[1], palette);
		};

		for (std::uint32_t index = 0; index < NC2Runtime::GuiMouseWorldCursorLimits::Markers; ++index)
		{
			std::uint32_t position;
			manager->GuiMarker_Get(index, &position);

			if (NC2Logic::MapMigPoint_IsInsideMap(&position) && position != 0)
				marker(&position);
		}

		if (mode != 0 && Read<std::uint8_t>(input, NC2Layout::InputManagerOffset::AcceptedTargetPresent) != 0)
		{
			if ((gMarkerColors.initialized & 1u) == 0)
			{
				gMarkerColors.initialized |= 1u;
				gMarkerColors.black.SetColors(0, 0, 0);
				NXBasics::Runtime_AtExit(MarkerBlack_Exit);
			}

			if ((gMarkerColors.initialized & 2u) == 0)
			{
				gMarkerColors.initialized |= 2u;
				gMarkerColors.white.SetColors(255, 255, 255);
				NXBasics::Runtime_AtExit(MarkerWhite_Exit);
			}

			const auto* position = static_cast<const std::byte*>(input) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint;
			NC2E2::DE_Debug_DrawMicroGridMarker(display, position, &gMarkerColors.black, &gMarkerColors.white, 5, false);

			if (mode == 12)
			{
				std::int32_t pixel[2];
				NC2E2::Display_MapMigToLocalDisplayPixelCoordinates(display, position, pixel, false);
				NC2E2::DrawAtom_House_Tool_DoDrawEngineHouseType(NC2InGameGuiInputManager::Selection_GetSelectedEngineHouseTypeId(input), NC2InGameGuiInputManager::Selection_GetSelectedEngineHouseExtensionLevel(input), Read<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), pixel[0], pixel[1], -1, false, true);
			}
		}

		if (mode == 15 || mode == 17)
		{
			const auto* position = static_cast<const std::byte*>(input) + (mode == 15 ? NC2Layout::InputManagerOffset::RoadStartPoint : NC2Layout::InputManagerOffset::WallStartPoint);
			const auto graphic = mode == 15 ? NC2E2::StaticHumanMisc_RoadBuildSign_GetLandscapeId(player) : NC2E2::StaticHumanMisc_WallBuildSign_GetLandscapeId(player);
			std::int32_t pixel[2];
			NC2E2::Display_MapMigToLocalDisplayPixelCoordinates(display, position, pixel, false);
			const auto brightness = NC2E2::Display_DE_Hightlighting_GetMapLandscapeBrightness(display, position);
			NC2E2::EngineGraphic_Draw(graphic, 1, Read<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), pixel[0], pixel[1], 0, brightness, -1, false);

			if (Read<std::uint8_t>(input, NC2Layout::InputManagerOffset::AcceptedTargetPresent) != 0)
			{
				auto current = Read<std::uint32_t>(position, NC2Layout::MapCoordinatePairOffset::Coordinates);
				NC2Logic::CMapPathfinderSpecialWallRoadElement path;
				const auto* target = static_cast<const std::byte*>(input) + NC2Layout::InputManagerOffset::AcceptedTargetMapPoint;

				if (mode == 15)
				{
					NC2Logic::Tool_RoadBuildManager_GetPossibleVectorList(player, &path, &current, target);
				}
				else
				{
					NC2Logic::MapTool_Wall_Build_GetPossibleVectorList(player, &path, &current, target);
				}

				const auto drawSign = [&]
				{
					NC2E2::Display_MapMigToLocalDisplayPixelCoordinates(display, &current, pixel, false);
					const auto light = NC2E2::Display_DE_Hightlighting_GetMapLandscapeBrightness(display, &current);
					NC2E2::EngineGraphic_Draw(graphic, 1, Read<NXBasics::CBitmap*>(display, NC2Layout::EngineDisplayOffset::DrawingBitmap), pixel[0], pixel[1], 0, light, -1, false);
				};
				drawSign();
				auto previous = current;

				for (auto direction = path.GetNextDirection(); direction >= 0; direction = path.GetNextDirection())
				{
					NC2Logic::MapMigPoint_MoveCursorUnwrapped(current, static_cast<std::uint32_t>(direction));

					if (NC2Logic::MapMigPoint_IsInsideMap(&previous))
						NC2E2::DE_Debug_DrawMicroGridLine(display, &current, &previous, &gMarkerColors.black);

					drawSign();
					previous = current;
				}
			}
		}
		else if (mode == 2 || mode == 3)
		{
			const auto id = NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(input);

			if (id != -1)
				marker(NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::RuntimeElementSize + NC2Layout::HumanOffset::WorkCenter);
		}

		return true;
	}
}