#pragma once

#include <cstdint>

namespace NXBasics { struct SRectangle; }
namespace NC2MainMenuGuiManager { class CGuiManager; }

namespace NRemaster::GameSetup::Menu
{
	inline constexpr std::uint32_t AddAi = 0x7200, CloseSlot = 0x7201, OpenTribes = 0x7202, SelectTribe = 0x7203, FillAi = 0x7204, ChangePage = 0x7205, CloseDropdown = 0x7206, OpenControl = 0x7207, SelectControl = 0x7208;
	void Reset();
	void BuildSlots(NC2MainMenuGuiManager::CGuiManager& gui, const NXBasics::SRectangle& rectangle, void* owner);
	bool HandleCommand(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t command, std::uint32_t value);
	bool HandleDropdownKey(NC2MainMenuGuiManager::CGuiManager& gui, std::uint32_t key);

	// Native setup widgets are shared by the online slot table and offline skirmish screen.
	void* Button(NC2MainMenuGuiManager::CGuiManager& gui, const NXBasics::SRectangle& rectangle, const char* caption, const char* tooltip, std::uint32_t command, std::uint32_t value, void* owner, bool enabled, bool selected = false, bool dropdownField = false, bool registerElement = true);
	void* Label(NC2MainMenuGuiManager::CGuiManager& gui, const NXBasics::SRectangle& rectangle, const char* text, bool registerElement = true);
	void Panel(NC2MainMenuGuiManager::CGuiManager& gui, const NXBasics::SRectangle& rectangle);
	void* List(NC2MainMenuGuiManager::CGuiManager& gui, const NXBasics::SRectangle& rectangle);
	void Scrollbar(NC2MainMenuGuiManager::CGuiManager& gui, void* list, const NXBasics::SRectangle& rectangle);
	void WrappedLabel(NC2MainMenuGuiManager::CGuiManager& gui, NXBasics::SRectangle rectangle, const char* caption, std::uint32_t lines, void* list = nullptr);
}
