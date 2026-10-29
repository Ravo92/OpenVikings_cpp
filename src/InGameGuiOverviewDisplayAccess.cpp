#include "C2EngineDisplayAccess.h"
#include "XPalette.h"
#include "C2EngineMapTerrainAccess.h"
#include "C2EngineObjectData.h"
#include "C2EngineTerrainData.h"
#include "C2GameMapAccess.h"
#include "C2LogicObjectArrays.h"
#include "C2MapGeometry.h"
#include "C2ObjectLayout.h"
#include "C2OverviewObjectIterators.h"
#include "C2StaticTrianglePatternTypeData.h"
#include "GuiBitmapToolElementAccess.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "InGameGuiOverviewDisplayAccess.h"
#include "InGameGuiOverviewHandler.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiElementAccess.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"

#include <cstring>
#include <new>
#include <Windows.h>

namespace NC2InGameGuiOverviewDisplay
{
	template <typename T>
	static T OverviewDisplay_Read(const void* object, std::size_t offset)
	{
		T value;
		std::memcpy(&value, static_cast<const std::byte*>(object) + offset, sizeof(value));

		return value;
	}

	template <typename T>
	static void OverviewDisplay_Write(void* object, std::size_t offset, T value)
	{
		std::memcpy(static_cast<std::byte*>(object) + offset, &value, sizeof(value));
	}

	static NXBasics::SColorRGB OverviewDisplay_ReadColor(const void* object, std::size_t offset)
	{
		NXBasics::SColorRGB color(0, 0, 0);
		std::memcpy(&color, static_cast<const std::byte*>(object) + offset, sizeof(color));

		return color;
	}

	static std::int32_t OverviewDisplay_Signed(std::uint32_t bits)
	{

		return NXBasics::Runtime_TimeSigned32(bits);
	}

	static std::int16_t OverviewDisplay_Word(std::uint32_t bits)
	{
		const std::uint16_t word = static_cast<std::uint16_t>(bits);
		std::int16_t value;
		std::memcpy(&value, &word, sizeof(value));

		return value;
	}

	static std::int32_t OverviewDisplay_Add(std::int32_t first, std::int32_t second)
	{

		return OverviewDisplay_Signed(static_cast<std::uint32_t>(first) + static_cast<std::uint32_t>(second));
	}

	static std::int32_t OverviewDisplay_Subtract(std::int32_t first, std::int32_t second)
	{

		return OverviewDisplay_Signed(static_cast<std::uint32_t>(first) - static_cast<std::uint32_t>(second));
	}

	static std::int32_t OverviewDisplay_Divide(std::int32_t numerator, std::int32_t denominator)
	{
		// Runtime IDIV retains Windows division faults, including INT_MIN / -1.
		const volatile std::int32_t dividend = numerator;
		const volatile std::int32_t divisor = denominator;

		return dividend / divisor;
	}

	static std::uint32_t OverviewDisplay_DivideUnsigned(std::uint32_t numerator, std::uint32_t denominator)
	{
		const volatile std::uint32_t dividend = numerator;
		const volatile std::uint32_t divisor = denominator;

		return dividend / divisor;
	}

	// Complete 00569488..005694A3 region; the next color belongs to another GUI block.
	struct SOverviewColors
	{
		NXBasics::SColorRGB cBrownColor{ 0, 0, 0 };       // 00569488.
		std::uint32_t terrainInitializationFlags = 0;     // 0056948C, low byte used.
		NXBasics::SColorRGB cBlackColor{ 0, 0, 0 };       // 00569490.
		NXBasics::SColorRGB animalType10Color{ 0, 0, 0 }; // 00569494.
		NXBasics::SColorRGB animalType19Color{ 0, 0, 0 }; // 00569498.
		std::uint32_t animalInitializationFlags = 0;      // 0056949C, low byte used.
		NXBasics::SColorRGB cGreyColor{ 0, 0, 0 };        // 005694A0.
	};

	static SOverviewColors gOverviewColors;
	static_assert(sizeof(SOverviewColors) == 28);
	static_assert(offsetof(SOverviewColors, cBlackColor) == 8);
	static_assert(offsetof(SOverviewColors, cGreyColor) == 0x18);

	// Original no-op destruction callbacks: FUN_004cf4e8, FUN_004cf4e7, FUN_004cf4e6.
	// The runtime ignores callback return values; its interface uses int callbacks.
	// Original function: FUN_004cf4e8; the native RET callback has no side effects.
	static int __cdecl OverviewBlackColor_Destruct()
	{

		return 0;
	}

	// Original function: FUN_004cf4e7; the native RET callback has no side effects.
	static int __cdecl OverviewBrownColor_Destruct()
	{

		return 0;
	}

	// Original function: FUN_004cf4e6; the native RET callback has no side effects.
	static int __cdecl OverviewGreyColor_Destruct()
	{

		return 0;
	}

	static void OverviewDisplay_InitializeTerrainColors()
	{
		if ((gOverviewColors.terrainInitializationFlags & 1u) == 0)
		{
			gOverviewColors.terrainInitializationFlags |= 1u;
			gOverviewColors.cBlackColor = NXBasics::SColorRGB(0, 0, 0);
			NXBasics::Runtime_AtExit(&OverviewBlackColor_Destruct);
		}

		if ((gOverviewColors.terrainInitializationFlags & 2u) == 0)
		{
			gOverviewColors.terrainInitializationFlags |= 2u;
			gOverviewColors.cBrownColor = NXBasics::SColorRGB(128, 64, 0);
			NXBasics::Runtime_AtExit(&OverviewBrownColor_Destruct);
		}

		if ((gOverviewColors.terrainInitializationFlags & 4u) == 0)
		{
			gOverviewColors.terrainInitializationFlags |= 4u;
			gOverviewColors.cGreyColor = NXBasics::SColorRGB(180, 180, 180);
			NXBasics::Runtime_AtExit(&OverviewGreyColor_Destruct);
		}
	}

	static void OverviewDisplay_SetColor(NXBasics::SColorRGB* color, std::uint32_t red, std::uint32_t green, std::uint32_t blue)
	{
		// FUN_0040f7f3 changes RGB only, retaining the fourth stored byte.
		color->red = static_cast<std::uint8_t>(red);
		color->green = static_cast<std::uint8_t>(green);
		color->blue = static_cast<std::uint8_t>(blue);
	}

	static std::uint32_t OverviewTexture_ByteOffset(std::uint32_t textureId, std::int32_t x, std::int32_t y)
	{

		return (textureId * 16u + static_cast<std::uint32_t>(y / 64) * 4u + static_cast<std::uint32_t>(x / 64)) * 8u;
	}

	// Original function: FUN_004cf6db; state receiver is element +0x30.
	static void OverviewTexture_GetColor(const void* state, std::uint32_t textureId, std::int32_t x, std::int32_t y, NXBasics::SColorRGB* color)
	{
		*color = OverviewDisplay_ReadColor(state, OverviewTexture_ByteOffset(textureId, x, y));
	}

	// Original function: FUN_004cf709.
	static void OverviewTexture_SetColor(void* state, std::uint32_t textureId, std::int32_t x, std::int32_t y, const NXBasics::SColorRGB* color)
	{
		const std::uint32_t offset = OverviewTexture_ByteOffset(textureId, x, y);
		OverviewDisplay_Write(state, offset, *color);
		OverviewDisplay_Write<std::uint8_t>(state, offset + 4u, 1);
	}

	// Original function: FUN_004cf741.
	static std::uint8_t OverviewTexture_GetFlag(const void* state, std::uint32_t textureId, std::int32_t x, std::int32_t y)
	{

		return OverviewDisplay_Read<std::uint8_t>(state, OverviewTexture_ByteOffset(textureId, x, y) + 4u);
	}

	// Original function: FUN_004cf4bb; Windows macro-point arguments narrow to WORD.
	static std::uint32_t OverviewDisplay_GetMapBrightness(std::int32_t x, std::int32_t y)
	{
		const std::int16_t macroPoint[2] = { OverviewDisplay_Word(static_cast<std::uint32_t>(x)), OverviewDisplay_Word(static_cast<std::uint32_t>(y)) };
		const void* data = NC2E2::EngineMap_GetConstMapMacroPointPtr(macroPoint);

		return data == nullptr ? 0 : OverviewDisplay_Read<std::uint8_t>(data, 4);
	}

	static bool OverviewDisplay_IsExplored(const void* element, const void* microPoint)
	{
		const std::uint32_t player = OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 4);

		return OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 45) == 0 || player >= 16u || (OverviewDisplay_Read<std::uint16_t>(microPoint, 8) & (1u << player)) != 0;
	}

	static const void* OverviewDisplay_Entry(const void* data, std::int32_t id, std::uint32_t stride)
	{

		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(data) + static_cast<std::uint32_t>(id) * stride);
	}

	static bool OverviewDisplay_IsMapPointExplored(const void* element, const void* point)
	{
		if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 45) == 0 || OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 4) >= 16u)
			return true;

		return OverviewDisplay_IsExplored(element, NC2Logic::GameMap_GetConstMapMicroPointPtr(point));
	}

	static bool OverviewDisplay_IsPointInsideBitmap(const std::int32_t* point, const NXBasics::CBitmap* bitmap)
	{
		const NXBasics::SPoint position{ point[0], point[1] };

		return position.IsInside(bitmap->GetRectangle());
	}

	static NXBasics::SColorRGB OverviewDisplay_GetPulseColor()
	{
		const std::uint32_t phase = timeGetTime() & NXBasics::BitMask::Low9Bits;
		const auto intensity = static_cast<std::uint8_t>((phase & NXBasics::BitMask::Bit8) != 0 ? 0x1FEu - phase : phase & NXBasics::IntegerRange::UInt8Maximum);
		// At phase 511, the original subtraction narrows -1 to 255.

		return NXBasics::SColorRGB(intensity, intensity, intensity);
	}

	// Original no-op callbacks: FUN_004cef60 (type 19), FUN_004cef5f (type 10).
	// Original function: FUN_004cef60; the native RET callback has no side effects.
	static int __cdecl OverviewAnimalType19Color_Destruct()
	{

		return 0;
	}

	// Original function: FUN_004cef5f; the native RET callback has no side effects.
	static int __cdecl OverviewAnimalType10Color_Destruct()
	{

		return 0;
	}

	// Original virtual entry: FUN_004ce533.
	static void* __fastcall WorldOverviewElement_DeletingDestructor(void* element, void* unusedEdx, std::uint32_t flags)
	{
		(void)unusedEdx;
		WorldOverviewElement_Destruct(element);

		if ((flags & 1u) != 0)
			NXBasics::Runtime_Free(element);

		return element;
	}

	// ABI adapter for FUN_004ce60e.
	static void __fastcall WorldOverviewElement_VirtualDraw(void* element, void* unusedEdx, NXBasics::CBitmap* bitmap, NXBasics::SRectangle* rectangle)
	{
		(void)unusedEdx;
		WorldOverviewElement_Draw(element, bitmap, rectangle);
	}

	// ABI adapter for FUN_004cf81a; preserves ECX and ret 20.
	static bool __fastcall WorldOverviewElement_VirtualMessage(void* element, void* unusedEdx, std::uint32_t messageBits, std::uint32_t param1Bits, std::uintptr_t param2Payload, std::uintptr_t param3Payload, std::uintptr_t param4Payload)
	{
		(void)unusedEdx;

		return WorldOverviewElement_MessageHandle(element, messageBits, param1Bits, param2Payload, param3Payload, param4Payload);
	}

	// Complete 15-slot Windows table at 004F7AF8..004F7B33.
	static const std::uintptr_t gWorldOverviewElementVirtualTable[] = {
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&WorldOverviewElement_DeletingDestructor>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&WorldOverviewElement_VirtualMessage>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&WorldOverviewElement_VirtualDraw>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_Activate>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ButtonPressed>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_IsHit>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_FindElementOnPosition>()),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ParentToLocalPosition>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify),
		reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&NXBaseGui::BaseElement_ToolTip_Available>()),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce)
	};
	static_assert(sizeof(gWorldOverviewElementVirtualTable) == 15 * sizeof(std::uintptr_t));

	// Original function: FUN_004ce5cc; STextureElement constructor.
	static void WorldOverviewTextureElement_Construct(void* texture)
	{
		OverviewDisplay_Write<std::uint32_t>(texture, 0, 0); // SColorRGB, FUN_0040f7b4.
	}

	// Original function: FUN_004ce5b4; initializes only marker point members.
	static void WorldOverviewMarkers_Construct(void* markers)
	{
		for (std::uint32_t index = 0; index < 20; ++index)
		{
			OverviewDisplay_Write<std::uint16_t>(markers, index * 12u + 10u, 0);
			OverviewDisplay_Write<std::uint16_t>(markers, index * 12u + 8u, 0);
		}
	}

	// Original function: FUN_004ce54f; state at element +0x30.
	static void WorldOverviewElement_ConstructState(void* state)
	{
		for (std::uint32_t texture = 0; texture < 256; ++texture)
		{
			for (std::uint32_t sample = 0; sample < 16; ++sample)
			{
				WorldOverviewTextureElement_Construct(static_cast<std::byte*>(state) + texture * 0x80u + sample * 8u);
			}
		}

		OverviewDisplay_Write<std::uint16_t>(state, offsetof(SOverviewElementState, flagsAndMarkers) + 12, 0);
		OverviewDisplay_Write<std::uint16_t>(state, offsetof(SOverviewElementState, flagsAndMarkers) + 10, 0);
		OverviewDisplay_Write<std::uint16_t>(state, offsetof(SOverviewElementState, flagsAndMarkers) + 16, 0);
		OverviewDisplay_Write<std::uint16_t>(state, offsetof(SOverviewElementState, flagsAndMarkers) + 14, 0);
		OverviewDisplay_Write<std::uint16_t>(state, offsetof(SOverviewElementState, flagsAndMarkers) + 20, 0);
		OverviewDisplay_Write<std::uint16_t>(state, offsetof(SOverviewElementState, flagsAndMarkers) + 18, 0);
		WorldOverviewMarkers_Construct(static_cast<std::byte*>(state) + offsetof(SOverviewElementState, flagsAndMarkers) + 56);
	}

	// Original function: FUN_004ce418; CWorldOverviewBitmapGuiElement constructor.
	void* WorldOverviewElement_Construct(void* element, const NXBasics::SRectangle* rectangle, std::uint32_t viewIndex)
	{
		auto* desktopBitmap = NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();
		NC2InGameGuiBase::BaseBitmapToolElement_Construct(element, rectangle, 1, 0x1A1, OverviewDisplay_Read<std::int32_t>(desktopBitmap, 4));
		void* state = static_cast<std::byte*>(element) + offsetof(SOverviewElementStorage, state);
		WorldOverviewElement_ConstructState(state);
		OverviewDisplay_Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldOverviewElementVirtualTable + 0);
		std::memset(state, 0, sizeof(SOverviewElementState));
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		bitmap->Fill(static_cast<std::uint8_t>(0));
		void* allocation = NXBasics::Runtime_AllocateWithNewHandler(sizeof(NXBasics::CBitmap), 1);
		NXBasics::CBitmap* background = nullptr;

		if (allocation != nullptr)
		{
			desktopBitmap = NXBaseGui::CDesktop::sTheObjectPtr->GetDisplayBitmap();
			background = ::new (allocation) NXBasics::CBitmap(bitmap->GetRectangle(), OverviewDisplay_Read<std::int32_t>(desktopBitmap, 4));
		}

		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 41, 0);
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.background), background);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 40, 1);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 42, 1);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 43, 1);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 44, 1);
		auto* manager = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr;
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 45, static_cast<std::uint8_t>(manager->Exploration_GetFlag()));
		constexpr std::size_t enabledOffsets[] = { offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 46, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 47, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 48, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 51, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 55 };

		for (std::size_t offset : enabledOffsets)
		{
			OverviewDisplay_Write<std::uint8_t>(element, offset, 1);
		}

		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 36, viewIndex);
		const std::int32_t savedZoom = CWorldOverviewElementHandler::mStaticVars.zoomByView[viewIndex];
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32, savedZoom);

		if (savedZoom == 0)
			OverviewDisplay_Write<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32, 1);

		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 4, manager->GetLocalPlayerId());
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0, 1);
		CWorldOverviewElementHandler::sTheObjectPtr->System_RegisterElement(static_cast<CWorldOverviewBitmapGuiElement*>(element));

		return element;
	}

	// Original function: FUN_004ce5d8.
	void WorldOverviewElement_Destruct(void* element)
	{
		OverviewDisplay_Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gWorldOverviewElementVirtualTable + 0);
		CWorldOverviewElementHandler::sTheObjectPtr->System_UnRegisterElement(static_cast<CWorldOverviewBitmapGuiElement*>(element));
		auto* background = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, state.background));

		if (background != nullptr)
		{
			delete background;
			OverviewDisplay_Write<void*>(element, offsetof(SOverviewElementStorage, state.background), nullptr);
		}

		NC2InGameGuiBase::BaseBitmapToolElement_Destruct(element);
	}

	// Original function: FUN_004ce60e; XGui_BE_Element_Draw.
	void WorldOverviewElement_Draw(void* element, NXBasics::CBitmap* bitmap, NXBasics::SRectangle* rectangle)
	{
		if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0) != 0)
		{
			WorldOverviewElement_RedrawAll(element);
		}
		else if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 8) != 0 && OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 1) != 0)
		{
			std::int32_t minimum[2], maximum[2];
			WorldOverviewElement_TransformToOverViewDisplayCoord(element, maximum, reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(element) + offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 14));
			WorldOverviewElement_TransformToOverViewDisplayCoord(element, minimum, reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(element) + offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 10));
			WorldOverviewElement_UpdateArea(element, minimum, maximum);
		}

		WorldOverviewElement_CheckForScrolling(element);
		auto* background = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, state.background));
		background->CopyIntoBitmap(OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap)), 0, 0);

		if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 44) != 0)
			WorldOverviewElement_DrawHouses(element);

		if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 46) != 0)
			WorldOverviewElement_DrawGuides(element);

		WorldOverviewElement_DrawMapMoveables(element);

		if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 54) != 0)
			WorldOverviewElement_DrawChestMarker(element);

		WorldOverviewElement_DrawSightRectangle(element);

		if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 47) != 0)
			WorldOverviewElement_DrawMarker(element);

		NC2InGameGuiBase::BaseBitmapToolElement_Draw(element, bitmap, rectangle);
	}

	// Original function: FUN_004ce6da; l_MapOverview_DrawGuides.
	void WorldOverviewElement_DrawGuides(void* element)
	{
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		NC2Logic::CGuideIterator iterator;
		const NXBasics::SColorRGB white(255, 255, 255);

		for (std::int32_t id = iterator.GetFirstGuideId(OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 4)); id != -1; id = iterator.GetNextGuideId())
		{
			const void* guide = OverviewDisplay_Entry(NC2Logic::CGuideArray::mStaticVars, id, NC2Logic::CGuideArray::ElementSize);
			std::int32_t point[2];
			WorldOverviewElement_TransformToOverViewDisplayCoord(element, point, reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(guide) + 0x14));

			if (!OverviewDisplay_IsPointInsideBitmap(point, bitmap))
				continue;

			const NXBasics::SColorRGB* color = NC2E2::Tool_PlayerColors_GetColorRef(OverviewDisplay_Read<std::int32_t>(guide, 4));
			bitmap->Draw_Circle(point[0], point[1], 2, *color);

			for (std::int32_t connection = 0; connection < OverviewDisplay_Read<std::int32_t>(guide, 0x78); connection = OverviewDisplay_Add(connection, 1))
			{
				const std::int32_t connectedId = OverviewDisplay_Read<std::int32_t>(guide, 0x3Cu + static_cast<std::uint32_t>(connection) * 8u);
				const void* connected = OverviewDisplay_Entry(NC2Logic::CGuideArray::mStaticVars, connectedId, NC2Logic::CGuideArray::ElementSize);
				std::int32_t connectedPoint[2];
				WorldOverviewElement_TransformToOverViewDisplayCoord(element, connectedPoint, reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(connected) + 0x14));
				bitmap->Draw_Line(point[0], point[1], connectedPoint[0], connectedPoint[1], white);
			}
		}
	}

	// Original function: FUN_004ce7ca; l_MapOverview_DrawHouses.
	void WorldOverviewElement_DrawHouses(void* element)
	{
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		const NXBasics::SColorRGB pulse = OverviewDisplay_GetPulseColor();
		NC2Logic::CHouseIterator iterator;

		for (std::int32_t id = iterator.GetFirstHouseId(); id != -1; id = iterator.GetNextHouseId())
		{
			const void* house = OverviewDisplay_Entry(NC2Logic::CHouseArray::mStaticVars, id, NC2Logic::CHouseArray::ElementSize);

			if (OverviewDisplay_Read<std::uint8_t>(house, 0x28) == 0)
				continue;

			const auto* position = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(house) + 0x1C);

			if (!OverviewDisplay_IsMapPointExplored(element, position))
				continue;

			std::int32_t point[2];
			WorldOverviewElement_TransformToOverViewDisplayCoord(element, point, position);

			if (!OverviewDisplay_IsPointInsideBitmap(point, bitmap))
				continue;

			const NXBasics::SColorRGB* color = NC2E2::Tool_PlayerColors_GetColorRef(OverviewDisplay_Read<std::int32_t>(house, 0x0C));
			const void* displayData = OverviewDisplay_Entry(NC2E2::gDynamicDataHouses.entries, OverviewDisplay_Read<std::int32_t>(house, 4), 0xD0);
			bitmap->Draw_HorizontalLine(OverviewDisplay_Subtract(point[0], 1), point[1], 3, *color);
			bitmap->Draw_HorizontalLine(OverviewDisplay_Subtract(point[0], 1), OverviewDisplay_Add(point[1], 1), 3, *color);
			bitmap->Draw_SetPixel(point[0], OverviewDisplay_Subtract(point[1], 1), *color);

			if (OverviewDisplay_Read<std::uint8_t>(displayData, 0xCC) != 0)
				bitmap->Draw_Circle(point[0], point[1], 3, pulse);
		}
	}

	// Original function: FUN_004cead7; l_MapOverview_DrawSightRectangle.
	void WorldOverviewElement_DrawSightRectangle(void* element)
	{
		void* desktop = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetDesktop();
		const void* display = OverviewDisplay_Read<void*>(desktop, NC2Layout::WorldGuiElementOffset::Display);
		std::int32_t x = OverviewDisplay_Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelX);
		std::int32_t height = OverviewDisplay_Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::BufferHeight);
		std::int32_t y = OverviewDisplay_Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::WorldPixelY);
		const std::int32_t zoom = OverviewDisplay_Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::ZoomCounter);
		std::int32_t width = OverviewDisplay_Read<std::int32_t>(display, NC2Layout::EngineDisplayOffset::BufferWidth);

		if (zoom > 0)
		{
			const std::int32_t scaledWidth = width >> (static_cast<std::uint32_t>(zoom) & 31u);
			const std::int32_t scaledHeight = height >> (static_cast<std::uint32_t>(zoom) & 31u);
			x = OverviewDisplay_Add(x, OverviewDisplay_Subtract(width, scaledWidth) / 2);
			y = OverviewDisplay_Add(y, OverviewDisplay_Subtract(height, scaledHeight) / 2);
			width = scaledWidth;
			height = scaledHeight;
		}

		std::int16_t mapRectangle[4] = { 0, 0, 0, 0 };
		NC2E2::DisplayDescription_GetMapRectangle(display, static_cast<std::uint32_t>(x), static_cast<std::uint32_t>(y), static_cast<std::uint32_t>(width), static_cast<std::uint32_t>(height), mapRectangle);
		const std::int16_t minimum[2] = { mapRectangle[0], mapRectangle[1] };
		const std::int16_t maximum[2] = { OverviewDisplay_Word(static_cast<std::uint32_t>(mapRectangle[0]) + static_cast<std::uint32_t>(mapRectangle[2])), OverviewDisplay_Word(static_cast<std::uint32_t>(mapRectangle[1]) + static_cast<std::uint32_t>(mapRectangle[3])) };
		std::int32_t nearPoint[2], farPoint[2];
		WorldOverviewElement_TransformToOverViewDisplayCoord(element, nearPoint, minimum);
		WorldOverviewElement_TransformToOverViewDisplayCoord(element, farPoint, maximum);
		const NXBasics::SRectangle rectangle(nearPoint[0], nearPoint[1], OverviewDisplay_Subtract(farPoint[0], nearPoint[0]), OverviewDisplay_Subtract(farPoint[1], nearPoint[1]));
		const NXBasics::SColorRGB yellow(255, 255, 0);
		OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap))->Draw_Rectangle(rectangle, yellow);
	}

	// Original function: FUN_004cebf5; l_MapOverview_DrawMapMoveables.
	void WorldOverviewElement_DrawMapMoveables(void* element)
	{
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		const NXBasics::SColorRGB pulse = OverviewDisplay_GetPulseColor();
		NC2Logic::CMapMoveableIterator iterator;

		for (std::int32_t id = iterator.GetFirstMapMoveableId(); id != -1; id = iterator.GetNextMapMoveableId())
		{
			const void* moveable = OverviewDisplay_Entry(NC2Logic::gMapMoveableArrayStorage.entries, id, 0x58);

			if (OverviewDisplay_Read<std::uint8_t>(moveable, 0x10) == 0)
				continue;

			const auto* position = reinterpret_cast<const std::int16_t*>(static_cast<const std::byte*>(moveable) + 0x12);

			if (!OverviewDisplay_IsMapPointExplored(element, position))
				continue;

			std::int32_t point[2];
			WorldOverviewElement_TransformToOverViewDisplayCoord(element, point, position);

			if (!OverviewDisplay_IsPointInsideBitmap(point, bitmap))
				continue;

			const std::uint32_t type = OverviewDisplay_Read<std::uint32_t>(moveable, 8);

			if (type == 3)
			{
				if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 43) == 0)
					continue;

				const void* vehicle = OverviewDisplay_Entry(NC2Logic::CVehicleArray::mStaticVars, OverviewDisplay_Read<std::int32_t>(moveable, 0x54), NC2Logic::CVehicleArray::ElementSize);
				const NXBasics::SColorRGB* color = NC2E2::Tool_PlayerColors_GetColorRef(OverviewDisplay_Read<std::int32_t>(vehicle, 0x1E0));
				const void* displayData = OverviewDisplay_Entry(NC2E2::gDynamicDataVehicles.entries, OverviewDisplay_Read<std::int32_t>(moveable, 0x54), NC2Layout::RecordSize::DynamicAnimalVehicle);
				bitmap->Draw_HorizontalLine(OverviewDisplay_Subtract(point[0], 1), OverviewDisplay_Subtract(point[1], 1), 3, *color);
				bitmap->Draw_HorizontalLine(OverviewDisplay_Subtract(point[0], 1), OverviewDisplay_Add(point[1], 1), 3, *color);
				bitmap->Draw_SetPixel(OverviewDisplay_Subtract(point[0], 1), point[1], *color);
				bitmap->Draw_SetPixel(OverviewDisplay_Add(point[0], 1), point[1], *color);

				if (OverviewDisplay_Read<std::uint8_t>(displayData, 0x40) != 0)
					bitmap->Draw_Circle(point[0], point[1], 3, pulse);
			}
			else if (type == 2)
			{
				if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 42) != 0)
				{
					const void* animal = NC2Logic::GetAnimalPtr(OverviewDisplay_Read<std::int32_t>(moveable, 0x54));
					const std::int32_t player = OverviewDisplay_Read<std::int32_t>(animal, 0x10);
					const NXBasics::SColorRGB neutral(200, 200, 200);
					const NXBasics::SColorRGB color = player == 20 ? neutral : *NC2E2::Tool_PlayerColors_GetColorRef(player);
					bitmap->Draw_HorizontalLine(point[0], OverviewDisplay_Add(point[1], 1), 2, color);
					bitmap->Draw_HorizontalLine(point[0], point[1], 2, color);
				}

				if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 52) != 0 && OverviewDisplay_Read<std::int32_t>(NC2Logic::GetAnimalPtr(OverviewDisplay_Read<std::int32_t>(moveable, 0x54)), 0x14) == 0x13)
				{
					if ((gOverviewColors.animalInitializationFlags & 1u) == 0)
					{
						gOverviewColors.animalInitializationFlags |= 1u;
						gOverviewColors.animalType19Color = NXBasics::SColorRGB(140, 255, 120);
						NXBasics::Runtime_AtExit(&OverviewAnimalType19Color_Destruct);
					}

					bitmap->Draw_Circle(point[0], point[1], 2, gOverviewColors.animalType19Color);
				}

				if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 53) != 0 && OverviewDisplay_Read<std::int32_t>(NC2Logic::GetAnimalPtr(OverviewDisplay_Read<std::int32_t>(moveable, 0x54)), 0x14) == 10)
				{
					if ((gOverviewColors.animalInitializationFlags & 2u) == 0)
					{
						gOverviewColors.animalInitializationFlags |= 2u;
						gOverviewColors.animalType10Color = NXBasics::SColorRGB(255, 207, 144);
						NXBasics::Runtime_AtExit(&OverviewAnimalType10Color_Destruct);
					}

					bitmap->Draw_Circle(point[0], point[1], 2, gOverviewColors.animalType10Color);
				}
			}
			else if (type == 1)
			{
				if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 40) == 0)
				{
					if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 41) == 0)
						continue;

					const void* human = NC2Logic::GetHumanPtr(OverviewDisplay_Read<std::int32_t>(moveable, 0x54));

					if (!NC2Logic::Human_IsJobSoldier(human) && !NC2Logic::Human_IsJobHero(NC2Logic::GetHumanPtr(OverviewDisplay_Read<std::int32_t>(moveable, 0x54))))
						continue;
				}

				const void* human = NC2Logic::GetHumanPtr(OverviewDisplay_Read<std::int32_t>(moveable, 0x54));
				const NXBasics::SColorRGB* color = NC2E2::Tool_PlayerColors_GetColorRef(OverviewDisplay_Read<std::int32_t>(human, 0x10));
				const void* displayData = OverviewDisplay_Entry(NC2E2::gDynamicDataHumans.entries, OverviewDisplay_Read<std::int32_t>(moveable, 0x54), NC2Layout::RecordSize::DynamicHuman);
				bitmap->Draw_SetPixel(point[0], OverviewDisplay_Subtract(point[1], 1), *color);
				bitmap->Draw_SetPixel(OverviewDisplay_Subtract(point[0], 1), point[1], *color);
				bitmap->Draw_SetPixel(OverviewDisplay_Add(point[0], 1), point[1], *color);
				bitmap->Draw_SetPixel(point[0], OverviewDisplay_Add(point[1], 1), *color);

				if (OverviewDisplay_Read<std::uint8_t>(displayData, NC2Layout::DynamicHumanOffset::SelectedFlag) != 0)
					bitmap->Draw_Circle(point[0], point[1], 3, pulse);
			}
		}
	}

	// Original function: FUN_004cfdb7; l_MapOverview_DrawChestMarker.
	void WorldOverviewElement_DrawChestMarker(void* element)
	{
		const std::int32_t origin[2] = { 0, 0 };
		std::int16_t minimum[2], maximum[2];
		WorldOverviewElement_TransformToMapCoord(element, origin, minimum);
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		const std::int32_t extent[2] = { bitmap->GetRectangle().width, bitmap->GetRectangle().height };
		WorldOverviewElement_TransformToMapCoord(element, extent, maximum);

		for (std::int16_t y = minimum[1]; y < maximum[1]; y = OverviewDisplay_Word(static_cast<std::uint32_t>(y) + 1u))
		{
			for (std::int16_t x = minimum[0]; x < maximum[0]; x = OverviewDisplay_Word(static_cast<std::uint32_t>(x) + 1u))
			{
				const std::int16_t mapPoint[2] = { x, y };

				if (!NC2Logic::MapMigPoint_IsInsideMap(mapPoint) || !OverviewDisplay_IsMapPointExplored(element, mapPoint))
					continue;

				const void* data = NC2Logic::GameMap_GetConstMapMicroPointPtr(mapPoint);

				if (OverviewDisplay_Read<std::uint8_t>(data, 0) != 1)
					continue;

				const std::uint8_t type = OverviewDisplay_Read<std::uint8_t>(data, 2);

				if (type != 0x55 && type != 0x56)
					continue;

				std::int32_t point[2];
				WorldOverviewElement_TransformToOverViewDisplayCoord(element, point, mapPoint);
				bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
				const NXBasics::SColorRGB white(255, 255, 255);
				const NXBasics::SRectangle rectangle(OverviewDisplay_Subtract(point[0], 2), OverviewDisplay_Subtract(point[1], 2), 5, 5);
				bitmap->Draw_Rectangle(rectangle, white);
			}
		}
	}

	// Original function: FUN_004ce918; l_MapOverview_RedrawAll.
	void WorldOverviewElement_RedrawAll(void* element)
	{
		const std::int32_t mapWidth = NC2Logic::GameMap_GetWidth();
		auto* background = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, state.background));
		const std::int32_t xScale = OverviewDisplay_Divide(OverviewDisplay_Signed(static_cast<std::uint32_t>(mapWidth) << 8), background->GetRectangle().width);
		const std::int32_t mapHeight = NC2Logic::GameMap_GetHeight();
		const std::int32_t yScale = OverviewDisplay_Divide(OverviewDisplay_Signed(static_cast<std::uint32_t>(mapHeight) << 8), background->GetRectangle().height);
		const std::int32_t scale = xScale > yScale ? xScale : yScale;

		if (OverviewDisplay_Divide(scale, OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32)) < 100)
		{
			std::int32_t zoom = OverviewDisplay_Divide(scale, 100);
			OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32, zoom);

			if (zoom == 0)
				zoom = 1;

			OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32, zoom);
			CWorldOverviewElementHandler::mStaticVars.zoomByView[OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 36)] = zoom;

			if (OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.scaleX)) == OverviewDisplay_Divide(scale, zoom))
			{
				OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0, 0);

				return;
			}
		}

		background->Fill(static_cast<std::uint8_t>(0));
		OverviewDisplay_Write<std::int32_t>(element, offsetof(SOverviewElementStorage, state.originX), 0);
		OverviewDisplay_Write<std::int32_t>(element, offsetof(SOverviewElementStorage, state.originY), 0);
		OverviewDisplay_Write<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 24, 0);
		OverviewDisplay_Write<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 28, 0);
		const std::int32_t resultingScale = OverviewDisplay_Divide(scale, OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32));
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.scaleX), resultingScale);
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.scaleY), resultingScale);
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		const std::int32_t extent[2] = { bitmap->GetRectangle().width, bitmap->GetRectangle().height };
		std::int16_t visibleMapExtent[2];
		WorldOverviewElement_TransformToMapCoord(element, extent, visibleMapExtent);
		std::memcpy(static_cast<std::byte*>(element) + offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 18, visibleMapExtent, 4);
		const std::int16_t centerX = OverviewDisplay_Word(static_cast<std::uint32_t>(OverviewDisplay_Subtract(visibleMapExtent[0], mapWidth) / 2));
		const std::int16_t centerY = OverviewDisplay_Word(static_cast<std::uint32_t>(OverviewDisplay_Subtract(visibleMapExtent[1], mapHeight) / 2));
		const std::int32_t originX = centerX < 0 ? 0 : centerX;
		const std::int32_t originY = centerY < 0 ? 0 : centerY;
		const std::int32_t offsetX = OverviewDisplay_Signed(0u - static_cast<std::uint32_t>(OverviewDisplay_Divide(originX * 256, resultingScale)));
		const std::int32_t offsetY = OverviewDisplay_Signed(0u - static_cast<std::uint32_t>(OverviewDisplay_Divide(originY * 256, resultingScale)));
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 24, offsetX);
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 28, offsetY);
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.originX), offsetX);
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.originY), offsetY);
		const std::int16_t farMapPoint[2] = { 3000, 3000 };
		const std::int16_t nearMapPoint[2] = { 0, 0 };
		std::int32_t minimum[2], maximum[2];
		WorldOverviewElement_TransformToOverViewDisplayCoord(element, maximum, farMapPoint);
		WorldOverviewElement_TransformToOverViewDisplayCoord(element, minimum, nearMapPoint);
		WorldOverviewElement_UpdateArea(element, minimum, maximum);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0, 0);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 1, 0);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 8, 0);
	}

	// Original function: FUN_004cef61; l_MapOverview_UpdateArea(SPoint const&, SPoint const&).
	void WorldOverviewElement_UpdateArea(void* element, const std::int32_t* minimum, const std::int32_t* maximum)
	{
		if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 9) != 0)
		{
			WorldOverviewElement_ScanTextures(element);
			OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 9, 0);
		}

		NXBasics::SRectangle area(minimum[0], minimum[1], OverviewDisplay_Subtract(maximum[0], minimum[0]), OverviewDisplay_Subtract(maximum[1], minimum[1]));
		auto* background = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, state.background));

		if (!area.IsTouching(background->GetRectangle()))
		{

			return;
		}

		area.CutInside(background->GetRectangle());
		std::int16_t previousPoint[2] = { -1, -1 };
		NXBasics::SColorRGB color(0, 0, 0);
		OverviewDisplay_InitializeTerrainColors();
		std::uint32_t searchDirection = 0;

		for (std::int32_t y = area.y; y < OverviewDisplay_Add(area.y, area.height); y = OverviewDisplay_Add(y, 1))
		{
			for (std::int32_t x = area.x; x < OverviewDisplay_Add(area.x, area.width); x = OverviewDisplay_Add(x, 1))
			{
				const std::int32_t pixel[2] = { x, y };
				std::int16_t mapPoint[2];
				WorldOverviewElement_TransformToMapCoord(element, pixel, mapPoint);

				if (!NC2Logic::MapMigPoint_IsInsideMap(mapPoint))
					continue;

				const void* microPoint = NC2Logic::GameMap_GetConstMapMicroPointPtr(mapPoint);
				const NXBasics::SColorRGB* directColor = nullptr;

				if (!OverviewDisplay_IsExplored(element, microPoint))
				{
					directColor = &gOverviewColors.cBlackColor;
				}
				else if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 51) != 0 && (OverviewDisplay_Read<std::uint8_t>(microPoint, 0x0D) & 2u) != 0)
				{
					directColor = &gOverviewColors.cBrownColor;
				}
				else if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 48) != 0 && OverviewDisplay_Read<std::uint8_t>(microPoint, 0) == 1 && OverviewDisplay_Read<std::uint8_t>(microPoint, 2) == 0x52)
				{
					const std::int32_t player = OverviewDisplay_Read<std::int8_t>(microPoint, 4);
					directColor = player == -1 ? &gOverviewColors.cGreyColor : NC2E2::Tool_PlayerColors_GetColorRef(player);
					// This branch draws even when the original color accessor returns null.
					background->Draw_SetPixelUnclipped(x, y, *directColor);
					continue;
				}

				if (directColor != nullptr)
				{
					background->Draw_SetPixelUnclipped(x, y, *directColor);
					continue;
				}

				if (mapPoint[0] == previousPoint[0] && mapPoint[1] == previousPoint[1])
				{
					background->Draw_SetPixelUnclipped(x, y, color);
					continue;
				}

				std::memcpy(previousPoint, mapPoint, sizeof(previousPoint));
				std::int16_t macroPoint[2];
				NC2Logic::MapMagPoint_ConstructFromMig(macroPoint, mapPoint, false);
				const std::uint32_t brightness = OverviewDisplay_GetMapBrightness(macroPoint[0], macroPoint[1]);
				std::uint32_t landscapeType = OverviewDisplay_Read<std::uint8_t>(microPoint, 0) == 1 ? OverviewDisplay_Read<std::uint8_t>(microPoint, 2) : 0;

				while (landscapeType == 0 && searchDirection < 6)
				{
					std::uint32_t neighbor = OverviewDisplay_Read<std::uint32_t>(mapPoint, 0);
					NC2Logic::MapMigPoint_MoveCursorUnwrapped(neighbor, searchDirection);

					if (NC2Logic::MapMigPoint_IsInsideMap(&neighbor))
					{
						const void* neighborData = NC2Logic::GameMap_GetConstMapMicroPointPtr(&neighbor);

						if (OverviewDisplay_Read<std::uint8_t>(neighborData, 0) == 1)
							landscapeType = OverviewDisplay_Read<std::uint8_t>(neighborData, 2);
					}

					++searchDirection;
				}

				const std::int32_t adjustedBrightness = OverviewDisplay_Add(OverviewDisplay_Subtract(OverviewDisplay_Signed(brightness), 128) / 2, 128);

				if (landscapeType == 4 || landscapeType == 0x11 || landscapeType == 0x1B)
				{
					std::uint32_t intensity = static_cast<std::uint32_t>(adjustedBrightness);

					if (landscapeType != 0x1B)
					{
						intensity = intensity * (landscapeType == 4 ? 120u : 60u) >> 7;

						if (intensity > 255u)
							intensity = 255u;
					}

					if (landscapeType == 4)
					{
						OverviewDisplay_SetColor(&color, 0, intensity, 0);
					}
					else
					{
						OverviewDisplay_SetColor(&color, intensity, intensity, landscapeType == 0x1B ? 0u : intensity);
					}
				}
				else if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 49) != 0)
				{
					const void* patterns[6];
					NC2E2::Engine2d_EntryPoint_Tool_GetPatternDescriptionsAroundPoint(mapPoint, patterns);

					if (patterns[0] == nullptr)
					{
						OverviewDisplay_SetColor(&color, 0, 0, 0);
					}
					else
					{
						color = NC2Logic::gStaticTrianglePatternTypes[OverviewDisplay_Read<std::uint32_t>(patterns[0], 0x38)].debugColor;
					}
				}
				else if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 50) != 0)
				{
					OverviewDisplay_SetColor(&color, 40, 40, 40);
				}
				else
				{
					const void* patterns[6];
					NC2E2::Engine2d_EntryPoint_Tool_GetPatternDescriptionsAroundPoint(mapPoint, patterns);
					std::uint32_t red = 0, green = 0, blue = 0;
					std::int32_t brightnessSum = OverviewDisplay_Subtract(OverviewDisplay_Signed(OverviewDisplay_GetMapBrightness(macroPoint[0], macroPoint[1])), 128);

					for (std::uint32_t direction = 0; direction < 6; ++direction)
					{
						WorldOverviewElement_GetSampleColor(element, patterns[direction], &color);
						NC2Logic::MapMagPoint_ConstructFromMig(macroPoint, mapPoint, false);
						const void* enginePoint = NC2E2::EngineMap_GetConstMapMacroPointPtr(macroPoint);
						const std::uint32_t vertexColor = enginePoint == nullptr ? 0u : OverviewDisplay_Read<std::uint8_t>(enginePoint, 5);

						if (vertexColor != 0 && NC2E2::gVertexColorsPalette != nullptr)
						{
							const NXBasics::SColorRGB tint = OverviewDisplay_ReadColor(NC2E2::gVertexColorsPalette, vertexColor * 4u + NXBasics::PaletteOffset::Colors);
							const auto modulate = [](std::uint8_t channel, std::uint8_t factor) -> std::uint32_t
							{
								const std::uint32_t value = static_cast<std::uint32_t>(channel) * factor >> 7;

								return value > 255u ? 255u : value;
							};
							const std::uint32_t tintedRed = modulate(color.red, tint.red);
							const std::uint32_t tintedGreen = modulate(color.green, tint.green);
							const std::uint32_t tintedBlue = modulate(color.blue, tint.blue);
							color.red = static_cast<std::uint8_t>(tintedRed);
							color.green = static_cast<std::uint8_t>(tintedGreen);
							color.blue = static_cast<std::uint8_t>(tintedBlue);
						}

						red += color.red;
						green += color.green;
						blue += color.blue;
						std::uint32_t neighbor = OverviewDisplay_Read<std::uint32_t>(mapPoint, 0);
						NC2Logic::MapMigPoint_MoveCursorUnwrapped(neighbor, direction);

						if (!NC2Logic::MapMigPoint_IsInsideMap(&neighbor))
							continue;

						if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 45) != 0 && OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 4) < 16u && !OverviewDisplay_IsExplored(element, NC2Logic::GameMap_GetConstMapMicroPointPtr(&neighbor)))
						{
							brightnessSum = OverviewDisplay_Subtract(brightnessSum, 128);
						}
						else
						{
							NC2Logic::MapMagPoint_ConstructFromMig(macroPoint, &neighbor, false);
							brightnessSum = OverviewDisplay_Add(brightnessSum, OverviewDisplay_Subtract(OverviewDisplay_Signed(OverviewDisplay_GetMapBrightness(macroPoint[0], macroPoint[1])), 128));
						}
					}

					OverviewDisplay_SetColor(&color, red / 6u, green / 6u, blue / 6u);
					WorldOverviewElement_BrightColor(&color, static_cast<std::uint32_t>(OverviewDisplay_Add(brightnessSum / 7, 128)));
				}

				background->Draw_SetPixelUnclipped(x, y, color);
				searchDirection = 0;
			}
		}

		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 8, 0);
		OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 1, 0);
	}

	// Original function: FUN_004cf4e9; l_ScanTextures.
	void WorldOverviewElement_ScanTextures(void* element)
	{
		const std::uint32_t height = NC2Logic::GameMap_GetMacroHeight();
		const std::uint32_t width = NC2Logic::GameMap_GetMacroWidth();
		std::int16_t macroPoint[2] = { 0, 0 };
		NXBasics::SColorRGB color(0, 0, 0);

		if (height == 0)
		{

			return;
		}

		do
		{
			macroPoint[0] = 0;

			if (width != 0)
			{
				do
				{
					const void* pattern = NC2E2::Engine2d_EntryPoint_Tool_GetPatternDescriptionPtr(macroPoint, true);
					WorldOverviewElement_GetSampleColor(element, pattern, &color);
					pattern = NC2E2::Engine2d_EntryPoint_Tool_GetPatternDescriptionPtr(macroPoint, false);
					WorldOverviewElement_GetSampleColor(element, pattern, &color);
					macroPoint[0] = OverviewDisplay_Word(static_cast<std::uint32_t>(macroPoint[0]) + 1u);
				} while (macroPoint[0] < static_cast<std::int32_t>(width));
			}

			macroPoint[1] = OverviewDisplay_Word(static_cast<std::uint32_t>(macroPoint[1]) + 1u);
		} while (macroPoint[1] < static_cast<std::int32_t>(height));
	}

	// Original function: FUN_004cf57c; l_BrightColor.
	void WorldOverviewElement_BrightColor(NXBasics::SColorRGB* color, std::uint32_t brightness)
	{
		const std::uint32_t red = (static_cast<std::uint32_t>(color->red) * brightness) >> 7;
		const std::uint32_t blue = (static_cast<std::uint32_t>(color->blue) * brightness) >> 7;
		const std::uint32_t green = (static_cast<std::uint32_t>(color->green) * brightness) >> 7;
		OverviewDisplay_SetColor(color, red > 255u ? 255u : red, green > 255u ? 255u : green, blue > 255u ? 255u : blue);
	}

	// Original function: FUN_004cf5c9; l_GetSampleColor.
	void WorldOverviewElement_GetSampleColor(void* element, const void* pattern, NXBasics::SColorRGB* color)
	{
		if (pattern == nullptr)
		{
			OverviewDisplay_SetColor(color, 0, 0, 0);
			return;
		}

		void* state = static_cast<std::byte*>(element) + offsetof(SOverviewElementStorage, state);
		const std::uint32_t texture = OverviewDisplay_Read<std::uint32_t>(pattern, 0x3C);
		std::int32_t textureX = OverviewDisplay_Read<std::int32_t>(pattern, 0x40);
		std::int32_t textureY = OverviewDisplay_Read<std::int32_t>(pattern, 0x44);

		if (OverviewTexture_GetFlag(state, texture, textureX, textureY) == 0)
		{
			NXBasics::CBitmap* bitmap = NC2E2::gStaticDataTextures.bitmaps[texture];
			NXBasics::CPalette* palette = NC2E2::gStaticDataTextures.palettes[texture];
			std::uint32_t red = 0, green = 0, blue = 0, count = 0;

			for (std::int32_t y = textureY; y < OverviewDisplay_Read<std::int32_t>(pattern, 0x4C); y = OverviewDisplay_Add(y, 1))
			{
				for (std::int32_t x = OverviewDisplay_Read<std::int32_t>(pattern, 0x40); x < OverviewDisplay_Read<std::int32_t>(pattern, 0x48); x = OverviewDisplay_Add(x, 1))
				{
					const std::uint8_t index = bitmap->Draw_GetPixel(x, y);
					const NXBasics::SColorRGB sample = OverviewDisplay_ReadColor(palette, static_cast<std::uint32_t>(index) * 4u + NXBasics::PaletteOffset::Colors);
					red += sample.red;
					green += sample.green;
					blue += sample.blue;
					++count;
				}
			}
			// Remaster: an empty sample has the same black fallback as a missing pattern.

			if (count == 0)
			{
				OverviewDisplay_SetColor(color, 0, 0, 0);

				return;
			}

			const std::uint32_t averageRed = OverviewDisplay_DivideUnsigned(red, count);
			const std::uint32_t averageGreen = OverviewDisplay_DivideUnsigned(green, count);
			const std::uint32_t averageBlue = OverviewDisplay_DivideUnsigned(blue, count);
			const NXBasics::SColorRGB average(static_cast<std::uint8_t>(averageRed), static_cast<std::uint8_t>(averageGreen), static_cast<std::uint8_t>(averageBlue));
			textureX = OverviewDisplay_Read<std::int32_t>(pattern, 0x40);
			textureY = OverviewDisplay_Read<std::int32_t>(pattern, 0x44);
			OverviewTexture_SetColor(state, OverviewDisplay_Read<std::uint32_t>(pattern, 0x3C), textureX, textureY, &average);
		}

		OverviewTexture_GetColor(state, OverviewDisplay_Read<std::uint32_t>(pattern, 0x3C), textureX, textureY, color);
	}

	// Original function: FUN_004cf76a; l_TransformToMapCoord.
	void WorldOverviewElement_TransformToMapCoord(const void* element, const std::int32_t* point, std::int16_t* mapPoint)
	{
		const std::int32_t xInput = point[0];
		const std::int32_t yInput = point[1];
		mapPoint[0] = OverviewDisplay_Word(static_cast<std::uint32_t>(xInput));
		// Windows reads the low word at SPoint +4, without sign extension.
		mapPoint[1] = OverviewDisplay_Word(static_cast<std::uint32_t>(yInput));
		mapPoint[0] = OverviewDisplay_Word(static_cast<std::uint32_t>(mapPoint[0]) + OverviewDisplay_Read<std::uint16_t>(element, offsetof(SOverviewElementStorage, state.originX)));
		mapPoint[1] = OverviewDisplay_Word(static_cast<std::uint32_t>(mapPoint[1]) + OverviewDisplay_Read<std::uint16_t>(element, offsetof(SOverviewElementStorage, state.originY)));
		const std::uint32_t xBits = OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.scaleX)) * static_cast<std::uint32_t>(mapPoint[0]);
		const std::uint32_t yBits = OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.scaleY)) * static_cast<std::uint32_t>(mapPoint[1]);
		// MSVC Win32 arithmetic right shift matches SAR before narrowing to WORD.
		mapPoint[0] = OverviewDisplay_Word(static_cast<std::uint32_t>(OverviewDisplay_Signed(xBits) >> 8));
		mapPoint[1] = OverviewDisplay_Word(static_cast<std::uint32_t>(OverviewDisplay_Signed(yBits) >> 8));
	}

	// Original function: FUN_004cf7ca; TransformToOverViewDisplayCoord.
	std::int32_t* WorldOverviewElement_TransformToOverViewDisplayCoord(const void* element, std::int32_t* point, const std::int16_t* mapPoint)
	{
		const std::int32_t x = OverviewDisplay_Divide(mapPoint[0] * 256, OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.scaleX)));
		const std::int32_t y = OverviewDisplay_Divide(mapPoint[1] * 256, OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.scaleY)));
		const std::int32_t xResult = OverviewDisplay_Subtract(x, OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.originX)));
		const std::int32_t yResult = OverviewDisplay_Subtract(y, OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.originY)));
		point[0] = xResult;
		point[1] = yResult;

		return point;
	}

	// Original function: FUN_004cf81a; XGui_BE_Message_Handle.
	bool WorldOverviewElement_MessageHandle(void* element, std::uint32_t messageBits, std::uint32_t param1Bits, std::uintptr_t param2Payload, std::uintptr_t param3Payload, std::uintptr_t param4Payload)
	{
		const auto message = static_cast<std::int32_t>(messageBits);
		const auto param1 = static_cast<std::int32_t>(param1Bits);
		const auto param2 = static_cast<std::uint32_t>(param2Payload);
		const auto param3 = static_cast<std::uint32_t>(param3Payload);
		const auto param4 = static_cast<std::uint32_t>(param4Payload);
		(void)param4;

		if (message == 0x800)
		{
			if (param1 == 12000)
			{
				auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
				const std::int32_t maximum[2] = { bitmap->GetRectangle().width, bitmap->GetRectangle().height };
				const std::int32_t minimum[2] = { 0, 0 };
				WorldOverviewElement_UpdateArea(element, minimum, maximum);
			}

			return true;
		}

		if (message == 0x100)
		{
			if (OverviewDisplay_Read<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 55) != 0)
			{
				const std::int32_t wheel = OverviewDisplay_Signed(param3);
				WorldOverviewElement_ModifyZoom(element, wheel < 0 ? -1 : wheel > 0 ? 1 : 0);
			}

			return true;
		}

		if (message != 1 && message != 0x20 && message != 0x80)
			return false;

		if (message == 0x80 && !NXSysMouseManager::CMouseManager::sTheObjectPtr->IsLeftButtonDown())
			return true;

		std::int32_t point[2] = { param1, OverviewDisplay_Signed(param2) };
		NC2InGameGuiBase::BaseElement_GlobalToLocalCoordinates(element, point);
		std::int16_t mapPoint[2] = { 0, 0 };
		WorldOverviewElement_TransformToMapCoord(element, point, mapPoint);
		NC2Logic::MapMigPoint_ValidatePosition(mapPoint);

		if (message == 0x20)
		{
			void* input = NC2InGameGuiInputManager::gInputManager;

			if (OverviewDisplay_Read<std::int32_t>(input, 0) != 0)
				return true;

			const std::int32_t selection = OverviewDisplay_Read<std::int32_t>(input, 4);

			if (selection <= 0)
				return true;

			if (selection <= 2)
			{
				NC2InGameGuiInputManager::Misc_Tool_SendSelectedHuman(input, mapPoint);
				return true;
			}

			if (selection == 4)
			{
				std::byte command[20]{ };
				command[0] = std::byte{ 0x65 };
				const std::int32_t id = NC2InGameGuiInputManager::Selection_GetSelectedSingleVehicleId(NC2InGameGuiInputManager::gInputManager);
				const std::uint32_t position = NC2Logic::MapMigPoint_GetLong(mapPoint);
				std::memcpy(command + 4, &id, sizeof(id));
				std::memcpy(command + 0x0C, &position, sizeof(position));
				NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, command);
			}
			else if (selection == 5)
			{
				std::uint32_t index = 0;

				if (NC2InGameGuiInputManager::Selection_GetSelectedMultipleVehiclesNumber(input) == 0)
					return true;

				do
				{
					std::byte command[20]{ };
					command[0] = std::byte{ 0x65 };
					const std::int32_t id = NC2InGameGuiInputManager::Selection_GetSelectedMultipleVehicleId(NC2InGameGuiInputManager::gInputManager, index);
					const std::uint32_t position = NC2Logic::MapMigPoint_GetLong(mapPoint);
					std::memcpy(command + 4, &id, sizeof(id));
					std::memcpy(command + 0x0C, &position, sizeof(position));
					NC2Logic::UserNetworkCommand_AddVehicleCommand(NC2Logic::gUserNetworkCommandManager, command);
					++index;
				} while (index < NC2InGameGuiInputManager::Selection_GetSelectedMultipleVehiclesNumber(NC2InGameGuiInputManager::gInputManager));
			}

			return true;
		}

		void* desktop = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetDesktop();
		using MessageFn = NXBaseGui::TGuiMessage;
		const auto* table = OverviewDisplay_Read<MessageFn*>(desktop, 0);
		const std::uint32_t position = NC2Logic::MapMigPoint_GetLong(mapPoint);
		table[2](desktop, 0x1000, 2004, position, 0, 0);

		return true;
	}

	// Original function: FUN_004cfa85; l_CheckForScrolling.
	void WorldOverviewElement_CheckForScrolling(void* element)
	{
		void* desktop = NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->GetDesktop();
		void* display = OverviewDisplay_Read<void*>(desktop, NC2Layout::WorldGuiElementOffset::Display);
		const std::int32_t worldY = NC2E2::DE_GetDisplayedCenterWorldYPosition(display);
		const std::int32_t worldX = NC2E2::DE_GetDisplayedCenterWorldXPosition(display);
		const std::int32_t worldPoint[2] = { worldX, worldY };
		std::uint32_t packedMapPoint = 0;
		NC2E2::DisplayDescription_WorldPixelToMapMigCoordinates(display, worldPoint, &packedMapPoint);
		std::int16_t mapPoint[2];
		std::memcpy(mapPoint, &packedMapPoint, sizeof(mapPoint));
		const std::int16_t visibleX = OverviewDisplay_Read<std::int16_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 18);
		const std::int16_t visibleY = OverviewDisplay_Read<std::int16_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 20);
		mapPoint[0] = OverviewDisplay_Word(static_cast<std::uint32_t>(OverviewDisplay_Subtract(mapPoint[0], visibleX / 2)));
		mapPoint[1] = OverviewDisplay_Word(static_cast<std::uint32_t>(OverviewDisplay_Subtract(mapPoint[1], visibleY / 2)));

		if (static_cast<std::int32_t>(mapPoint[0]) + visibleX > NC2Logic::GameMap_GetWidth())
			mapPoint[0] = OverviewDisplay_Word(static_cast<std::uint32_t>(NC2Logic::GameMap_GetWidth()) - static_cast<std::uint32_t>(visibleX));

		if (static_cast<std::int32_t>(mapPoint[1]) + visibleY > NC2Logic::GameMap_GetHeight())
			mapPoint[1] = OverviewDisplay_Word(static_cast<std::uint32_t>(NC2Logic::GameMap_GetHeight()) - static_cast<std::uint32_t>(visibleY));

		if (mapPoint[0] < 0)
			mapPoint[0] = 0;

		if (mapPoint[1] < 0)
			mapPoint[1] = 0;

		std::int32_t movement[2];
		WorldOverviewElement_TransformToOverViewDisplayCoord(element, movement, mapPoint);
		movement[0] = OverviewDisplay_Add(movement[0], OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 24));
		movement[1] = OverviewDisplay_Add(movement[1], OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 28));

		if (movement[0] == 0 && movement[1] == 0)
		{

			return;
		}

		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		auto* background = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, state.background));
		bitmap->Fill(NXBasics::SColorRGB(0, 0, 0));
		background->CopyIntoBitmap(bitmap, OverviewDisplay_Subtract(0, movement[0]), OverviewDisplay_Subtract(0, movement[1]));
		bitmap->CopyIntoBitmap(background, 0, 0);
		std::int32_t xStart, xEnd, stripStart, stripEnd;
		const std::int32_t width = background->GetRectangle().width;
		const std::int32_t height = background->GetRectangle().height;

		if (movement[0] < 0)
		{
			xStart = 0;
			stripStart = OverviewDisplay_Subtract(0, movement[0]);
			xEnd = width;
			stripEnd = width;
		}
		else
		{
			stripStart = 0;
			stripEnd = OverviewDisplay_Subtract(width, movement[0]);
			xStart = OverviewDisplay_Subtract(width, movement[0]);
			xEnd = width;
		}

		std::int32_t yStart = 0, yEnd;

		if (movement[1] < 0)
		{
			yEnd = OverviewDisplay_Subtract(0, movement[1]);
		}
		else
		{
			yStart = OverviewDisplay_Subtract(height, movement[1]);
			yEnd = height;
		}

		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.originX), OverviewDisplay_Add(OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.originX)), movement[0]));
		OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.originY), OverviewDisplay_Add(OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.originY)), movement[1]));
		const std::int32_t verticalMinimum[2] = { xStart, 0 };
		const std::int32_t verticalMaximum[2] = { xEnd, height };
		WorldOverviewElement_UpdateArea(element, verticalMinimum, verticalMaximum);
		const std::int32_t horizontalMinimum[2] = { stripStart, yStart };
		const std::int32_t horizontalMaximum[2] = { stripEnd, yEnd };
		WorldOverviewElement_UpdateArea(element, horizontalMinimum, horizontalMaximum);
	}

	// Original function: FUN_004cfcf2; l_MapOverview_DrawMarker.
	void WorldOverviewElement_DrawMarker(void* element)
	{
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		const NXBasics::SColorRGB white(255, 255, 255), red(255, 0, 0);

		for (std::uint32_t index = 0; index < 20; ++index)
		{
			const std::uint32_t offset = offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 56 + index * 12u;
			const std::uint32_t timestamp = OverviewDisplay_Read<std::uint32_t>(element, offset);

			if (timestamp == 0)
				continue;

			const std::uint32_t age = timeGetTime() - timestamp;

			if (age > 1000u)
			{
				OverviewDisplay_Write<std::uint32_t>(element, offset, 0);
				continue;
			}

			std::int16_t mapPoint[2];
			std::memcpy(mapPoint, static_cast<const std::byte*>(element) + offset + 8, 4);
			std::int32_t point[2];
			WorldOverviewElement_TransformToOverViewDisplayCoord(element, point, mapPoint);
			const auto& bounds = bitmap->GetRectangle();

			if (point[0] < bounds.x || point[0] > OverviewDisplay_Add(bounds.x, OverviewDisplay_Subtract(bounds.width, 1)))
				continue;

			if (point[1] < bounds.y || point[1] > OverviewDisplay_Add(bounds.y, OverviewDisplay_Subtract(bounds.height, 1)))
				continue;

			const auto& color = OverviewDisplay_Read<std::uint32_t>(element, offset + 4) == 1 ? red : white;
			bitmap->Draw_Circle(point[0], point[1], (1000u - age) / 100u + 3u, color);
		}
	}

	// Original function: FUN_004cff03; ModifyZoom. The original transforms the
	// bitmap center even when the resulting zoom does not change.
	void WorldOverviewElement_ModifyZoom(void* element, std::int32_t delta)
	{
		auto* bitmap = OverviewDisplay_Read<NXBasics::CBitmap*>(element, offsetof(SOverviewElementStorage, bitmap));
		const std::int32_t center[2] = { bitmap->GetRectangle().width / 2, bitmap->GetRectangle().height / 2 };
		std::int16_t mapPoint[2];
		WorldOverviewElement_TransformToMapCoord(element, center, mapPoint);
		std::int32_t zoom = OverviewDisplay_Add(OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32), delta);

		if (zoom < 1)
			zoom = 1;

		if (zoom != OverviewDisplay_Read<std::int32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32))
		{
			OverviewDisplay_Write(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 32, zoom);
			CWorldOverviewElementHandler::mStaticVars.zoomByView[OverviewDisplay_Read<std::uint32_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 36)] = zoom;
			OverviewDisplay_Write<std::uint8_t>(element, offsetof(SOverviewElementStorage, state.flagsAndMarkers) + 0, 1);
		}
	}
}