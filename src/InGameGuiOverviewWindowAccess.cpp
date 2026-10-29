#include "C2ObjectLayout.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "GameProfile.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiOverviewDisplayAccess.h"
#include "InGameGuiOverviewWindowAccess.h"
#include "SwitchIdentifiers.h"
#include "X2DApplication.h"
#include "X2DDisplay.h"
#include "XBitmap.h"
#include "XBobRendering.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XGuiWindowAccess.h"
#include "XNumericConstants.h"
#include "XPalette.h"
#include "XPaletteAccess.h"

#include <cstring>

namespace NC2InGameGuiOverviewDisplay
{
	template <typename T>
	static T OverviewWindow_Read(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

		return value;
	}

	template <typename T>
	static void OverviewWindow_Write(void* object, std::size_t offset, T value)
	{
		std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
	}

	static void WorldOverviewStaticWindow_AddButtons(void* window);
	static void WorldOverviewStaticWindow_UpdateButtonGraphics(void* window);
	static void WorldOverviewStaticWindow_UpdateDisplayFlag(void* window);
	static void* __fastcall WorldOverviewStaticWindow_Delete(void* window, void*, std::uint32_t flags);

	// Original function: FUN_004c9824; draw on the root of the virtual bitmap chain.
	static void __fastcall WorldOverviewStaticWindow_Draw(void*, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle*)
	{
		std::uint32_t x = 0, y = 0;

		while (bitmap->GetSourceBitmap() != nullptr)
		{
			x += static_cast<std::uint32_t>(bitmap->GetSourceX());
			y += static_cast<std::uint32_t>(bitmap->GetSourceY());
			bitmap = bitmap->GetSourceBitmap();
		}

		const auto& resources = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources;
		// Remaster: retain the complete alpha artwork, including its outer shadow.
		const auto* bounds = static_cast<const NXBasics::SRectangle*>(NXBasics::BobManager_GetBobAreaRectanglePtr(resources.windowBobManager, 0x37));

		if (bounds == nullptr || bounds->width <= 116 || bounds->height <= 129)
		{

			return;
		}

		NXBasics::CBitmap frame(bounds->width, bounds->height, 2);
		frame.Fill(std::uint32_t{ 0xFF000000 });
		auto* presentation = NXSys2DApp::CApplication::sTheObjectPtr->GetDisplay();
		presentation->SetHudScratchBitmap(&frame);

		if (NRemaster::gGameVariant == NRemaster::EGameVariant::Cultures2)
		{
			// Remaster artwork correction: C2's frame uses red entry 212 within its
			// brown shading ramp. Change only this draw's owned palette cache.
			NXBasics::CPalette framePalette(*resources.iconsLeftPalette);
			auto* colours = const_cast<std::uint32_t*>(NXBasics::Palette_GetTrueColorTablePtr(&framePalette));

			if (colours != nullptr && (colours[212] & 0x00FFFFFFu) == 0x005B0D05u)
			{
				std::uint32_t brown = 0;

				for (unsigned shift = 0; shift < 24; shift += 8)
				{
					brown |= (((((colours[211] >> shift) & 255u) + ((colours[213] >> shift) & 255u)) / 2u) << shift);
				}

				colours[212] = brown;
			}

			NXBasics::BobManager_PrintBob_UsingShadedAlpha(resources.windowBobManager, 0x37, &frame, -bounds->x, -bounds->y, &framePalette, 0x80);
		}
		else
		{
			NXBasics::BobManager_PrintBob_UsingShadedAlpha(resources.windowBobManager, 0x37, &frame, -bounds->x, -bounds->y, resources.iconsLeftPalette, 0x80);
		}

		presentation->SetHudScratchBitmap(nullptr);
		// Expand the original 112 x 112 opening to 224 x 224 without thickening
		// the top ornament, right rail or shadow. Preserve all four pixel channels.
		NXBasics::CBitmap enlarged(bounds->width + 112, bounds->height + 112, 2);
		const std::int32_t sourceX[] = { 0, 4, 116, bounds->width };
		const std::int32_t sourceY[] = { 0, 17, 129, bounds->height };
		const std::int32_t destinationX[] = { 0, 4, 228, bounds->width + 112 };
		const std::int32_t destinationY[] = { 0, 17, 241, bounds->height + 112 };

		for (std::int32_t row = 0; row < 3; ++row)
		{
			for (std::int32_t column = 0; column < 3; ++column)
			{
				NXBasics::CBitmap sourcePart(&frame, sourceX[column], sourceY[row], sourceX[column + 1] - sourceX[column], sourceY[row + 1] - sourceY[row]);
				NXBasics::CBitmap destinationPart(&enlarged, destinationX[column], destinationY[row], destinationX[column + 1] - destinationX[column], destinationY[row + 1] - destinationY[row]);
				sourcePart.CopyIntoBitmap_FitIn(&destinationPart);
			}
		}
		// The map reaches the left/bottom screen edges; the old black padding is
		// outside the viewport instead of being enlarged along with the image.
		presentation->CompositeHudImage(&enlarged, bitmap, NXBasics::Runtime_TimeSigned32(x - 4u), NXBasics::Runtime_TimeSigned32(y));
	}

	// Original function: FUN_004c97ac; XGui_BE_Message_Handle.
	static bool __fastcall WorldOverviewStaticWindow_MessageHandle(void* window, void* unusedEdx, std::int32_t message, std::int32_t param1, std::uint32_t param2, std::uint32_t param3, std::uint32_t param4)
	{
		(void)unusedEdx;
		(void)param2;
		(void)param3;
		(void)param4;

		if (message != 0x800)
			return false;

		switch (param1)
		{
			case SwitchIds::OverviewWindowCommand::Close:
				NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<NXBaseGui::CBaseWindow*>(window));
				break;
			case SwitchIds::OverviewWindowCommand::CycleViewMode:
			{
				std::uint32_t mode = OverviewWindow_Read<std::uint32_t>(window, 0x70) + 1u;
				OverviewWindow_Write(window, 0x70, mode);

				if (mode == 4)
					OverviewWindow_Write<std::uint32_t>(window, 0x70, 0);

				WorldOverviewStaticWindow_UpdateButtonGraphics(window);
				WorldOverviewStaticWindow_UpdateDisplayFlag(window);
				break;
			}
			case SwitchIds::OverviewWindowCommand::ToggleLargeOverview:
				NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_LargeOverviewWindow_Toggle();
				break;
			case SwitchIds::OverviewWindowCommand::ZoomOut:
				WorldOverviewElement_ModifyZoom(OverviewWindow_Read<void*>(window, 0x74), -1);
				break;
			case SwitchIds::OverviewWindowCommand::ZoomIn:
				WorldOverviewElement_ModifyZoom(OverviewWindow_Read<void*>(window, 0x74), 1);
				break;
		}

		return true;
	}

	// Complete original table at 004F78FC..004F7947. Window/base destruction,
	// title drawing and inherited GUI virtual methods use their reconstructed bodies.
	static const std::uintptr_t gWorldOverviewStaticWindowVirtualTable[] = {
		reinterpret_cast<std::uintptr_t>(&WorldOverviewStaticWindow_Delete),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&WorldOverviewStaticWindow_MessageHandle),
		reinterpret_cast<std::uintptr_t>(&WorldOverviewStaticWindow_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DrawOverlays)
	};
	static_assert(sizeof(gWorldOverviewStaticWindowVirtualTable) == 76);

	// Original function: FUN_004c9777; notify the manager before destroying children.
	static void* __fastcall WorldOverviewStaticWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		OverviewWindow_Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldOverviewStaticWindowVirtualTable + 0);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->MiscWindows_OverviewWindow_Closed();
		OverviewWindow_Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004c9708; CWorldOverviewStaticGuiWindow constructor.
	void* WorldOverviewStaticWindow_Construct(void* window)
	{
		// Include the top ornament in hit testing as well as the right rail.
		const std::int32_t y = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(NInGameGuiManager::CInGameGuiManager::mStaticVars.displayHeight) - 241u);
		const NXBasics::SRectangle rectangle(0, y, 257, 241);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		OverviewWindow_Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldOverviewStaticWindowVirtualTable + 0);
		std::memset(static_cast<std::byte*>(window) + 0x70, 0, 12);
		OverviewWindow_Write<std::uint32_t>(window, 0x70, 0);
		WorldOverviewStaticWindow_AddButtons(window);
		WorldOverviewStaticWindow_UpdateDisplayFlag(window);
		void* element = OverviewWindow_Read<void*>(window, 0x74);
		using TMessage = bool(__thiscall*)(void*, std::int32_t, std::int32_t, std::uint32_t, std::uint32_t, std::uint32_t);
		auto* methods = *static_cast<TMessage**>(element);
		methods[2](element, 0x800, 12000, 0, 0, 0);

		return window;
	}

	// Original function: FUN_004c9860; l_GuiButtons_Add.
	static void WorldOverviewStaticWindow_AddButtons(void* window)
	{
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(33132, 1);
		const NXBasics::SRectangle overviewRectangle(0, 17, 224, 224);
		void* element = allocation == nullptr ? nullptr : WorldOverviewElement_Construct(allocation, &overviewRectangle, 0);
		OverviewWindow_Write(window, 0x74, element);
		NXBaseGui::BaseWindow_Element_Add(window, element);

		struct SButtonParameters
		{
			std::int32_t y, width, height;
			std::uint32_t graphics, command, stringId;
			std::int32_t stringArray;
		};

		constexpr SButtonParameters buttons[] = {
			{ 0, 30, 16, 0x39, 10000, 0x21, 12 },
			{ 17, 30, 30, 0x3C, 10001, 0x22, 12 },
			{ 47, 30, 30, 0x89, 10005, 0x23, 12 },
			{ 92, 30, 30, 0x3A, 10010, 0x106, 4 },
			{ 69, 30, 30, 0x3B, 10011, 0x105, 4 }
		};

		for (std::uint32_t index = 0; index < 5; ++index)
		{
			const auto& button = buttons[index];
			allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
			void* control = nullptr;

			if (allocation != nullptr)
			{
				const char* text = NC2GuiToolsBase::CGuiBaseStringsManager::GetStringPtr(button.stringArray, button.stringId);
				const NXBasics::SRectangle rectangle(223, button.y == 0 ? 13 : button.y * 2 + 9, button.width, button.height);
				control = NC2InGameGuiBase::BaseToolGfxButton_ConstructWithToolTip(allocation, nullptr, &rectangle, button.graphics, text, button.command, window);
			}

			if (index == 1)
				OverviewWindow_Write(window, 0x78, control);
			// FUN_004b9407 stores the center-graphics flag; the Windows path
			// still dereferences the result after a failed allocation.
			OverviewWindow_Write<std::uint8_t>(control, 0x34, 0);
			NC2InGameGuiBase::ToolGraphics_SetPalettePtr(control, NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.iconsLeftPalette);
			NXBaseGui::BaseWindow_Element_Add(window, control);
		}

		WorldOverviewStaticWindow_UpdateButtonGraphics(window);
	}

	// Original function: FUN_004c9a80; l_GuiButtons_UpdateButtonGraphics.
	static void WorldOverviewStaticWindow_UpdateButtonGraphics(void* window)
	{
		const std::int32_t mode = OverviewWindow_Read<std::int32_t>(window, 0x70);
		std::uint32_t graphics = 0x3C;

		if (mode == 1)
			graphics = 0x3D;

		if (mode == 2)
			graphics = 0x3E;

		if (mode == 3)
			graphics = 0x3E;

		NC2InGameGuiBase::BaseToolGfxButton_SetGraphicsId(OverviewWindow_Read<void*>(window, 0x78), nullptr, graphics);
	}

	// Original function: FUN_004c9aa8; l_Misc_UpdateDisplayFlag.
	static void WorldOverviewStaticWindow_UpdateDisplayFlag(void* window)
	{
		const std::uint32_t mode = OverviewWindow_Read<std::uint32_t>(window, 0x70);

		if (mode > 3)
		{

			return;
		}

		void* element = OverviewWindow_Read<void*>(window, 0x74);
		OverviewWindow_Write<std::uint8_t>(element, 0x806C, mode == 1 ? 0 : 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x806D, 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x806E, 0);
		OverviewWindow_Write<std::uint8_t>(element, 0x806F, mode == 1 ? 0 : 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x8070, mode == 1 ? 0 : 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x8072, mode == 1 ? 0 : 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x8074, mode == 1 ? 0 : 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x8044, 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x8075, mode == 2 ? 1 : 0);
		OverviewWindow_Write<std::uint8_t>(element, 0x8044, 1);
		OverviewWindow_Write<std::uint8_t>(element, 0x8076, mode == 3 ? 1 : 0);
		OverviewWindow_Write<std::uint8_t>(element, 0x8044, 1);
	}

	// Complete Windows static region 00569478..00569487. Mac names:
	// CWorldOverviewLargeGuiWindow::sDisplayFlagsStructure/sDisplayFlagsValidFlag.
	struct SLargeOverviewDisplayFlags
	{
		std::uint8_t flags[15];
		std::uint8_t valid;
	};

	static SLargeOverviewDisplayFlags gLargeOverviewDisplayFlags{ };
	static_assert(sizeof(SLargeOverviewDisplayFlags) == 16);
	static constexpr std::size_t gLargeOverviewFlagOffsets[13] = { 0x806C, 0x806D, 0x806E, 0x806F, 0x8070, 0x8072, 0x8074, 0x8075, 0x8076, 0x8077, 0x8078, 0x8079, 0x807A };

	// Original function: FUN_004c5107; l_GuiButtons_UpdateMarkedStatus.
	static void WorldOverviewLargeWindow_UpdateMarkedStatus(void* window)
	{
		const void* overview = OverviewWindow_Read<void*>(window, 0x80);

		for (std::size_t index = 0; index < 13; ++index)
		{
			OverviewWindow_Write(OverviewWindow_Read<void*>(window, 0x84 + index * 4), 0x55, OverviewWindow_Read<std::uint8_t>(overview, gLargeOverviewFlagOffsets[index]));
		}
	}

	static void WorldOverviewLargeWindow_Destruct(void* window);

	// Original function: FUN_004c4e9e; scalar deleting destructor.
	static void* __fastcall WorldOverviewLargeWindow_Delete(void* window, void*, std::uint32_t flags)
	{
		WorldOverviewLargeWindow_Destruct(window);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(window);

		return window;
	}

	// Original function: FUN_004c4eff; XGui_BE_Message_Handle.
	static bool __fastcall WorldOverviewLargeWindow_MessageHandle(void* window, void*, std::int32_t message, std::int32_t command, std::uint32_t, std::uint32_t, std::uint32_t)
	{
		if (message == 0x1000)
			return command == 0x7E9;

		if (message != 0x800)
			return false;

		void* overview = OverviewWindow_Read<void*>(window, 0x80);

		if (command >= SwitchIds::OverviewWindowCommand::CycleViewMode && command <= 0x271D)
		{
			const auto offset = gLargeOverviewFlagOffsets[command - SwitchIds::OverviewWindowCommand::CycleViewMode];

			if (command == 0x2712)
				OverviewWindow_Write<std::uint8_t>(overview, 0x806C, 0);

			if (command == 0x2718 || command == 0x2719)
			{
				OverviewWindow_Write<std::uint8_t>(overview, command == 0x2718 ? 0x8076 : 0x8075, 0);
				OverviewWindow_Write<std::uint8_t>(overview, 0x8044, 1);
			}

			OverviewWindow_Write<std::uint8_t>(overview, offset, OverviewWindow_Read<std::uint8_t>(overview, offset) == 0 ? 1 : 0);

			if (command >= 0x2717 && command <= SwitchIds::OverviewWindowCommand::ZoomOut)
				OverviewWindow_Write<std::uint8_t>(overview, 0x8044, 1);

			WorldOverviewLargeWindow_UpdateMarkedStatus(window);
		}
		else if (command == 0x2774 || command == 0x2775)
		{
			WorldOverviewElement_ModifyZoom(overview, command == 0x2774 ? -1 : 1);
		}

		return true;
	}

	// Original function: FUN_004c50c5.
	static void __fastcall WorldOverviewLargeWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const NXBasics::SRectangle* rectangle)
	{
		NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
		NXBasics::SRectangle frame(0, 0, 0, 0);
		std::memcpy(&frame, static_cast<const std::byte*>(window) + 0x70, sizeof(frame));
		frame.Inflate(2, 2);
		NC2GuiToolsBase::DrawTool_DrawFrame(bitmap, frame, 2);
	}

	// Complete Windows table 004F7504..004F754F.
	static const std::uintptr_t gWorldOverviewLargeWindowVirtualTable[19] = {
		reinterpret_cast<std::uintptr_t>(&WorldOverviewLargeWindow_Delete),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&WorldOverviewLargeWindow_MessageHandle),
		reinterpret_cast<std::uintptr_t>(&WorldOverviewLargeWindow_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_Activate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonPressed),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseWindow_DrawOverlays)
	};
	static_assert(sizeof(gWorldOverviewLargeWindowVirtualTable) == 76);

	// Original function: FUN_004c4eba; save all fifteen flags before deleting the overview.
	static void WorldOverviewLargeWindow_Destruct(void* window)
	{
		OverviewWindow_Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldOverviewLargeWindowVirtualTable + 0);
		const auto* overview = static_cast<const std::byte*>(OverviewWindow_Read<void*>(window, 0x80));
		std::memcpy(gLargeOverviewDisplayFlags.flags, overview + 0x806C, sizeof(gLargeOverviewDisplayFlags.flags));
		gLargeOverviewDisplayFlags.valid = 1;
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Continue_LargeWindowClosed();
		OverviewWindow_Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
		NXBaseGui::BaseWindow_Destruct(window);
	}

	// Original function: FUN_004c4856; CWorldOverviewLargeGuiWindow constructor.
	void* WorldOverviewLargeWindow_Construct(void* window, bool topRight)
	{
		using namespace NC2InGameGuiBase;
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const auto left = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayWidth) - 500u);
		const auto top = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(state.displayHeight) - 420u);
		const NXBasics::SRectangle rectangle(topRight ? left : left / 2, topRight ? 0 : top / 2, 500, 420);
		BaseToolWindow_Construct(window, &rectangle);
		OverviewWindow_Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldOverviewLargeWindowVirtualTable + 0);
		std::memset(static_cast<std::byte*>(window) + 0x70, 0, 72);
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(248, 1);
		const NXBasics::SRectangle titleRectangle(2, 2, 478, 14);
		void* control = allocation == nullptr ? nullptr : WindowMoverButton_Construct(allocation, &titleRectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(0xFA));
		NXBaseGui::BaseWindow_Element_Add(window, control);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(84, 1);
		const NXBasics::SRectangle closeRectangle(482, 2, 16, 14);
		control = allocation == nullptr ? nullptr : WindowCloserButton_Construct(allocation, nullptr, &closeRectangle);
		NXBaseGui::BaseWindow_Element_Add(window, control);
		NXBasics::SRectangle overviewRectangle(138, 20, 358, 396);
		overviewRectangle.Inflate(-2, -2);
		OverviewWindow_Write(window, 0x70, overviewRectangle);
		allocation = NXBasics::Runtime_AllocateWithNewHandler(33132, 1);
		void* overview = allocation == nullptr ? nullptr : WorldOverviewElement_Construct(allocation, &overviewRectangle, 1);
		OverviewWindow_Write(window, 0x80, overview);
		NXBaseGui::BaseWindow_Element_Add(window, overview);

		if (topRight)
		{
			OverviewWindow_Write<std::uint8_t>(overview, 0x8071, 0);
			OverviewWindow_Write<std::uint8_t>(overview, 0x8044, 1);

			for (std::size_t offset = 0x8078; offset <= 0x807A; ++offset)
			{
				OverviewWindow_Write<std::uint8_t>(overview, offset, 1);
			}
		}

		if (gLargeOverviewDisplayFlags.valid != 0)
		{
			std::memcpy(static_cast<std::byte*>(overview) + 0x806C, gLargeOverviewDisplayFlags.flags, sizeof(gLargeOverviewDisplayFlags.flags));
			OverviewWindow_Write<std::uint8_t>(overview, 0x8044, 1);
		}

		struct SFilterButton
		{
			std::int32_t y;
			std::uint32_t command, stringId;
		};

		constexpr SFilterButton buttons[] = {
			{ 20, SwitchIds::OverviewWindowCommand::CycleViewMode, 0xFB }, { 40, 0x2712, 0xFC }, { 60, 0x2713, 0xFD }, { 80, 0x2714, 0xFE }, { 116, SwitchIds::OverviewWindowCommand::ToggleLargeOverview, NXBasics::IntegerRange::UInt8MaximumSigned }, { 136, 0x2716, 0x100 }, { 156, 0x2717, 0x101 }, { 176, SwitchIds::OverviewWindowCommand::ZoomOut, 0x102 }, { 212, 0x2718, 0x103 }, { 232, 0x2719, 0x104 }, { 268, SwitchIds::OverviewWindowCommand::ZoomIn, 0x107 }, { 288, 0x271C, 0x108 }, { 308, 0x271D, 0x109 }
		};

		for (const auto& button : buttons)
		{
			allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
			const NXBasics::SRectangle buttonRectangle(4, button.y, 130, 18);
			control = allocation == nullptr ? nullptr : BaseToolTextButton_Construct(allocation, nullptr, &buttonRectangle, NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(button.stringId), button.command, window);
			OverviewWindow_Write(window, 0x84 + (button.command - SwitchIds::OverviewWindowCommand::CycleViewMode) * 4, control);
			NXBaseGui::BaseWindow_Element_Add(window, control);
		}

		for (std::uint32_t index = 0; index < 2; ++index)
		{
			allocation = NXBasics::Runtime_AllocateWithNewHandler(96, 1);
			const NXBasics::SRectangle buttonRectangle(4 + static_cast<std::int32_t>(index) * 65, 344, 63, 18);
			control = allocation == nullptr ? nullptr : BaseToolTextButton_ConstructWithToolTip(allocation, nullptr, &buttonRectangle, index == 0 ? "-" : "+", NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(index == 0 ? 0x106 : 0x105), 0x2774 + index, window);
			NXBaseGui::BaseWindow_Element_Add(window, control);
		}

		WorldOverviewLargeWindow_UpdateMarkedStatus(window);
		using TMessage = bool(__thiscall*)(void*, std::int32_t, std::int32_t, std::uint32_t, std::uint32_t, std::uint32_t);
		OverviewWindow_Read<TMessage*>(overview, 0)[2](overview, 0x800, 12000, 0, 0, 0);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->Misc_GameTime_Stop_LargeWindowOpened();

		return window;
	}
}