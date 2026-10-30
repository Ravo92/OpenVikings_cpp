#include "C2E2HumanSoundAccess.h"
#include "C2EngineDrawingAccess.h"
#include "C2EngineObjectData.h"
#include "C2GameState.h"
#include "C2HumanJobAccess.h"
#include "C2LifetimeAccess.h"
#include "C2LifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2RuntimeConstants.h"
#include "C2StaticTribeTypeData.h"
#include "C2StringAccess.h"
#include "GuiBitmapToolElementAccess.h"
#include "GuiListBoxElementAccess.h"
#include "GuiTextToolElementAccess.h"
#include "GuiToolsBase.h"
#include "InGameGuiBaseAccess.h"
#include "InGameGuiHumanJobWindows.h"
#include "InGameGuiInputAccess.h"
#include "InGameGuiManager.h"
#include "NetworkShutdown.h"
#include "StringInputHandler.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XCrtFileAccess.h"
#include "XGuiDesktop.h"
#include "XGuiWindowAccess.h"
#include <array>
#include <cstring>
#include <new>

namespace NC2InGameGuiSelectedAction
{
	using namespace NC2Lifetime;
	using NXBasics::SRectangle;

	namespace
	{
		void* Input()
		{

			return NC2InGameGuiInputManager::gInputManager;
		}

		std::int32_t SelectedId()
		{

			return NC2InGameGuiInputManager::Selection_GetSelectedSingleHumanId(Input());
		}

		void* Human(std::int32_t id)
		{

			return NC2Logic::CHumanArray::mStaticVars + static_cast<std::intptr_t>(id) * NC2Logic::CHumanArray::RuntimeElementSize;
		}

		const char* Name(std::int32_t id)
		{

			return reinterpret_cast<const char*>(NC2E2::gDynamicDataHumans.entries) + static_cast<std::intptr_t>(id) * NC2Layout::RecordSize::DynamicHuman + NC2Layout::DynamicHumanOffset::Name;
		}

		const char* Misc(std::uint32_t id)
		{

			return NC2GuiToolsBase::StringTool_GetGameGuiMiscWindowStringPtr(id);
		}

		auto* TextInput(void* window)
		{

			return reinterpret_cast<NC2InGameGuiBaseTextInputElement::CStringInputHandler*>(static_cast<std::byte*>(window) + NC2Layout::SelectedHumanActionWindowOffset::NameInputHandler);
		}

		void Close(void* window)
		{
			NXBaseGui::CDesktop::sTheObjectPtr->Window_AddToCloseList(static_cast<NXBaseGui::CBaseWindow*>(window));
		}

		const NXBasics::SColorRGB* White()
		{

			return reinterpret_cast<const NXBasics::SColorRGB*>(reinterpret_cast<const std::byte*>(NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.fontWhitePalette) + NC2Layout::PaletteOffset::LastColor);
		}

		void Queue(std::int32_t id, std::uint8_t opcode, std::uint32_t value)
		{
			std::byte command[NC2Layout::RecordSize::UserNetworkCommandSigned]{ };
			Write(command, NC2Layout::UserNetworkCommandOffset::Opcode, opcode);
			Write(command, NC2Layout::UserNetworkCommandOffset::ObjectId, id);
			Write(command, NC2Layout::UserNetworkCommandOffset::Parameter1, value);
			NC2Logic::UserNetworkCommand_AddHumanCommand(NC2Logic::gUserNetworkCommandManager, command);
		}

		void Respond(std::int32_t id)
		{
			NC2E2::E2HumanSoundManager_PlayRespondingSound(NC2E2::gE2HumanSoundManager, 0, id);
		}

		void Frame(void* window, std::uint32_t title)
		{
			const auto width = Read<std::int32_t>(window, NC2Layout::GuiBaseElementOffset::Width);
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiWindowMover, 1);
			const SRectangle mover(2, 2, width - 22, 14);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : NC2InGameGuiBase::WindowMoverButton_Construct(memory, &mover, Misc(title)));
			memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiWindowCloser, 1);
			const SRectangle closer(width - 18, 2, 16, 14);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : NC2InGameGuiBase::WindowCloserButton_Construct(memory, nullptr, &closer));
		}

		void* List(const SRectangle& rectangle)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiListBox, 1);

			return memory == nullptr ? nullptr : ToolListBoxElement_Construct(memory, nullptr, &rectangle);
		}

		void AddList(void* window, void* list, const SRectangle& rectangle)
		{
			NXBaseGui::BaseWindow_Element_Add(window, list);
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiListBoxController, 1);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : ToolListBoxControllerElement_Construct(memory, nullptr, &rectangle, list, false, true));
		}

		void Row(void* window, void* list, SRectangle& rectangle, const char* caption, std::uint32_t job, std::uint32_t good, bool pressed)
		{
			void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
			void* element = memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_Construct(memory, nullptr, &rectangle, caption, SwitchIds::HumanJobWindowCommand::SelectRow, window);

			if (element != nullptr)
			{
				Write<std::uintptr_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter1, job);
				Write<std::uintptr_t>(element, NC2Layout::GuiTextButtonOffset::MessageParameter2, good);

				if (pressed)
					Write<std::uint8_t>(element, NC2Layout::GuiTextButtonOffset::SelectedFlag, 1);
			}

			BaseListBoxElement_Element_Add(list, nullptr, element);
			rectangle.y += 24;
		}

		// Original function: FUN_004b1a6d; shared Windows job-name comparator.
		std::int32_t __cdecl lsSortCompareFunction(const void* first, const void* second)
		{
			const char* secondName = NC2Logic::StringAccess_GetJobNamePtr(*static_cast<const std::uint32_t*>(second), 1);

			return NXBasics::Runtime_CompareStringsIgnoreCase(NC2Logic::StringAccess_GetJobNamePtr(*static_cast<const std::uint32_t*>(first), 1), secondName);
		}

		// Original function: FUN_004c8238; folded single-human validation also used by the name and production windows.
		void SelectedSingleHumanLearnJobWindow_l_System_Validate(void* window)
		{
			const auto saved = Read<std::uint32_t>(window, NC2Layout::SelectedHumanActionWindowOffset::SelectionChangeCount);
			const auto current = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionChangeCount);

			if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) != SwitchIds::SelectionType::SingleHuman || (saved != 0 && saved != current))
			{
				Write<std::uint32_t>(window, NC2Layout::SelectedHumanActionWindowOffset::SelectionChangeCount, 0);
				Write<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 0);
				Close(window);
			}
			else
			{
				Write(window, NC2Layout::SelectedHumanActionWindowOffset::SelectionChangeCount, current);
				Write<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 1);
			}
		}

		// Original function: FUN_004c7636.
		void SelectedSingleHumanChangeJobWindow_l_System_Validate(void* window)
		{
			const auto type = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);
			const auto saved = Read<std::uint32_t>(window, NC2Layout::SelectedHumanActionWindowOffset::SelectionChangeCount);
			const auto current = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionChangeCount);

			if ((type != SwitchIds::SelectionType::SingleHuman && type != SwitchIds::SelectionType::MultipleHumans) || (saved != 0 && saved != current))
			{
				Write<std::uint32_t>(window, NC2Layout::SelectedHumanActionWindowOffset::SelectionChangeCount, 0);
				Write<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 0);
				Close(window);
			}
			else
			{
				Write(window, NC2Layout::SelectedHumanActionWindowOffset::SelectionChangeCount, current);
				Write<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 1);
			}
		}

		// Original function: FUN_004c76bd.
		void SelectedSingleHumanChangeJobWindow_l_GuiButtons_Add(void* window)
		{
			Frame(window, 430);
			SelectedSingleHumanChangeJobWindow_l_System_Validate(window);

			if (Read<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection) == 0)
			{

				return;
			}

			void* list = List(SRectangle(4, 22, 268, 276));
			Write(window, NC2Layout::SelectedHumanActionWindowOffset::JobList, list);
			bool allowed[NC2Runtime::HumanJobWindowLimits::Jobs];
			std::memset(allowed, 1, sizeof(allowed));
			const auto selection = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);
			std::uint32_t currentJob = 0;

			if (selection == SwitchIds::SelectionType::SingleHuman)
			{
				const void* human = Human(SelectedId());

				for (std::uint32_t job = 0; job < NC2Runtime::HumanJobWindowLimits::Jobs; ++job)
				{
					if (!NC2Logic::Human_IsAbleToDoJob(human, job))
						allowed[job] = false;
				}

				currentJob = Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);
			}
			else if (selection == SwitchIds::SelectionType::MultipleHumans)
			{
				for (std::uint32_t index = 0; index < NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanNumber(Input()); ++index)
				{
					const void* human = Human(NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanId(Input(), index));

					for (std::uint32_t job = 0; job < NC2Runtime::HumanJobWindowLimits::Jobs; ++job)
					{
						if (!NC2Logic::Human_IsAbleToDoJob(human, job))
							allowed[job] = false;
					}
				}
			}
			else
			{
				std::memset(allowed, 0, sizeof(allowed));
			}

			std::uint32_t jobs[NC2Runtime::HumanJobWindowLimits::Jobs], count = 0;

			for (std::uint32_t job = 0; job < NC2Runtime::HumanJobWindowLimits::Jobs; ++job)
			{
				if (allowed[job])
					jobs[count++] = job;
			}

			if (count > 1)
				NXBasics::Runtime_QuickSort(jobs, count, NC2Layout::RecordSize::HumanJobId, lsSortCompareFunction);

			SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);
			// Complete native six-entry region 004F7680..004F76AF.
			constexpr std::uint32_t collectorGoods[NC2Runtime::HumanJobWindowLimits::CollectorGoodOptions][NC2Runtime::CollectorGoodOptionField::Fields]{ { 5, 0 }, { 2, 0 }, { 3, 0 }, { 14, 0 }, { 6, 1 }, { 7, 1 } };

			for (std::uint32_t index = 0; index < count; ++index)
			{
				const auto job = jobs[index];
				Row(window, list, row, NC2Logic::StringAccess_GetJobNamePtr(job, 1), job, 0, currentJob != 0 && job == currentJob);

				if (job != SwitchIds::HumanJob::Collector || selection != SwitchIds::SelectionType::SingleHuman)
					continue;

				const void* human = Human(SelectedId());
				const auto player = Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId);
				const auto tribe = player < NC2Runtime::GameControlLimits::Players ? Read<std::uint32_t>(NC2Logic::gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::TribeType) : 1u;

				for (const auto& entry : collectorGoods)
				{
					if (entry[NC2Runtime::CollectorGoodOptionField::RequiresCurrentAvailability] != 0 && !NC2Logic::Human_IsAbleToProduceGoodNow(human, entry[NC2Runtime::CollectorGoodOptionField::GoodId]))
						continue;

					if (Read<std::uint8_t>(NC2Logic::gGameControl, player * NC2Layout::RecordSize::GameControlPlayerSigned + NC2Layout::GameControlPlayerOffset::AllowedGoods + tribe * NC2Runtime::HumanJobWindowLimits::Goods + entry[NC2Runtime::CollectorGoodOptionField::GoodId]) == 0)
						continue;

					char caption[200];
					NXBasics::Runtime_PrintStringN(caption, sizeof(caption), "%s - %s", NC2Logic::StringAccess_GetJobNamePtr(SwitchIds::HumanJob::Collector, 1), NC2Logic::StringAccess_GetGoodNamePtr(entry[NC2Runtime::CollectorGoodOptionField::GoodId], 1));
					Row(window, list, row, caption, SwitchIds::HumanJob::Collector, entry[NC2Runtime::CollectorGoodOptionField::GoodId], false);
				}
			}

			AddList(window, list, SRectangle(276, 22, 20, 276));
			Write(window, NC2Layout::SelectedHumanActionWindowOffset::LastRefreshTime, static_cast<std::uint32_t>(timeGetTime()));
		}

		// Original function: FUN_004c767a.
		void SelectedSingleHumanChangeJobWindow_l_GuiButtons_Update(void* window)
		{
			const void* list = Read<void*>(window, NC2Layout::SelectedHumanActionWindowOffset::JobList);
			const auto scroll = list == nullptr ? 0 : Read<std::int32_t>(list, NC2Layout::ListBoxOffset::ScrollY);
			NXBaseGui::BaseWindow_Element_DeleteAll(window);
			Write<void*>(window, NC2Layout::SelectedHumanActionWindowOffset::JobList, nullptr);
			SelectedSingleHumanChangeJobWindow_l_GuiButtons_Add(window);

			if (void* replacement = Read<void*>(window, NC2Layout::SelectedHumanActionWindowOffset::JobList))
				NC2InGameGuiBaseListBox::BaseListBoxElement_ScrollOffset_Set(replacement, 0, scroll);
		}

		// Original function: FUN_004c7415.
		bool __fastcall SelectedSingleHumanChangeJobWindow_MessageHandle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t jobPayload, std::uintptr_t goodPayload)
		{
			const auto job = static_cast<std::uint32_t>(jobPayload);
			const auto good = static_cast<std::uint32_t>(goodPayload);

			if (message == SwitchIds::GuiWindowMessage::Query)
			{
				if (command == SwitchIds::GuiWindowQuery::HumanChangeJob)
				{
					SelectedSingleHumanChangeJobWindow_l_GuiButtons_Update(window);
					return true;
				}

				if (command == SwitchIds::HumanJobWindowCommand::ValidateSelection)
					SelectedSingleHumanChangeJobWindow_l_System_Validate(window);

				return false;
			}

			if (message != SwitchIds::GuiWindowMessage::Command)
				return false;

			if (command == SwitchIds::HumanJobWindowCommand::FrameUpdate)
			{
				if (static_cast<std::uint32_t>(timeGetTime()) > Read<std::uint32_t>(window, NC2Layout::SelectedHumanActionWindowOffset::LastRefreshTime) + NC2Runtime::HumanJobWindowTiming::RefreshMillisecondsUnsigned)
					SelectedSingleHumanChangeJobWindow_l_GuiButtons_Update(window);
			}
			else if (command == SwitchIds::HumanJobWindowCommand::SelectRow)
			{
				const auto selection = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);

				if (selection == SwitchIds::SelectionType::SingleHuman)
				{
					const auto id = SelectedId();
					Queue(id, SwitchIds::UserCommand::ChangeJob, job);

					if (good != 0)
						Queue(id, SwitchIds::UserCommand::ChangeProducedGood, good);

					Respond(id);

					if (Read<std::uint8_t>(NC2Logic::gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::UserShouldAttachWorkPlaceAfterJobChangeUnsigned) != 0)
					{
						NC2InGameGuiInputManager::InputMode_Set(Input(), SwitchIds::InputMode::AttachWorkHouse);
						NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->l_Selection_RemoveAllGuiElements();
					}
				}
				else if (selection == SwitchIds::SelectionType::MultipleHumans)
				{
					for (std::uint32_t index = 0; index < NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanNumber(Input()); ++index)
					{
						const auto id = NC2InGameGuiInputManager::Selection_GetSelectedMultipleHumanId(Input(), index);
						Queue(id, SwitchIds::UserCommand::ChangeJob, job);
						Respond(id);
					}
				}

				Close(window);
			}

			return true;
		}

		// Original function: FUN_004c75c1.
		void __fastcall SelectedSingleHumanChangeJobWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
		{
			NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
			const auto selection = Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType);

			if (selection == SwitchIds::SelectionType::SingleHuman && SelectedId() != -1)
			{
				bitmap->Text_Print(NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08, White(), 6, 300, Name(SelectedId()));
			}
			else if (selection == SwitchIds::SelectionType::MultipleHumans)
			{
				bitmap->Text_Print(NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08, White(), 6, 300, Misc(431));
			}
		}

		// Original function: FUN_004c73df.
		void* __fastcall SelectedSingleHumanChangeJobWindow_Delete(void* window, void*, std::uint32_t flags);
		// Original function: FUN_004c7b39.
		void* __fastcall SelectedSingleHumanChangeNameWindow_Delete(void* window, void*, std::uint32_t flags);

		// Original function: FUN_004c7cd0.
		void SelectedSingleHumanChangeNameWindow_l_GuiButtons_Add(void* window)
		{
			Frame(window, 230);
			SelectedSingleHumanLearnJobWindow_l_System_Validate(window);

			if (Read<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection) == 0)
			{

				return;
			}

			void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
			const SRectangle rectangle(4, 76, 140, 20);
			NXBaseGui::BaseWindow_Element_Add(window, memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_ConstructToggle(memory, nullptr, &rectangle, Misc(231), SwitchIds::HumanJobWindowCommand::SelectRow, window, false));
		}

		// Original function: FUN_004c7db4.
		void SelectedSingleHumanChangeNameWindow_l_Misc_DoChangeName(void* window)
		{
			SelectedSingleHumanLearnJobWindow_l_System_Validate(window);

			if (Read<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection) == 0 || Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) != SwitchIds::SelectionType::SingleHuman || SelectedId() == -1)
				return;

			const auto id = SelectedId();
			const char* text = TextInput(window)->GetString();

			if (std::strlen(text) == 0)
				return;

			NC2E2::DynamicHuman_SetName(static_cast<std::byte*>(NC2E2::gDynamicDataHumans.entries) + id * NC2Layout::RecordSize::DynamicHuman, text);

			if (NC2Logic::gGameStaticVars.networkGame == 0 || !NNetwork::Network_IsInSession())
			{

				return;
			}

			char formatted[100];
			NXBasics::Runtime_PrintStringN(formatted, sizeof(formatted), "%d:%d:%d:%s", Read<std::int32_t>(Human(id), NC2Layout::HumanOffset::PlayerId), id, Read<std::uint32_t>(Human(id), NC2Layout::HumanOffset::UniqueId), text);
			// The first UTF-16 code unit is the original rename record discriminator.
			wchar_t record[128]{ };
			record[0] = 2;
			NNetwork::CopyNetworkUtf8ToWide(record + 1, formatted, 127);
			NNetwork::SendDynamicNetworkChatMessage(&gDynamicNetworkData, 0, record);
		}

		// Original function: FUN_004c7b6b.
		bool __fastcall SelectedSingleHumanChangeNameWindow_MessageHandle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t keyPayload, std::uintptr_t characterPayload, std::uintptr_t)
		{
			const auto key = static_cast<std::uint32_t>(keyPayload);
			const auto character = static_cast<std::uint32_t>(characterPayload);

			if (message == SwitchIds::GuiWindowMessage::Query)
			{
				if (command == SwitchIds::GuiWindowQuery::HumanChangeName)
				{
					NXBaseGui::BaseWindow_Element_DeleteAll(window);
					SelectedSingleHumanChangeNameWindow_l_GuiButtons_Add(window);
					return true;
				}

				if (command == SwitchIds::HumanJobWindowCommand::ValidateSelection)
					SelectedSingleHumanLearnJobWindow_l_System_Validate(window);

				return false;
			}

			if (message != SwitchIds::GuiWindowMessage::Command)
				return false;

			if (command == SwitchIds::HumanJobWindowCommand::SelectRow)
			{
				SelectedSingleHumanChangeNameWindow_l_Misc_DoChangeName(window);
				Close(window);
			}
			else if (command == SwitchIds::HumanJobWindowCommand::NameInputKey)
			{
				if (Read<std::uint32_t>(window, NC2Layout::GuiBaseElementOffset::InteractionState) != 1)
					return false;

				if (key == SwitchIds::VirtualKey::Escape)
				{
					Close(window);
				}
				else if (TextInput(window)->WorkOnKey(key, character) == 3)
				{
					SelectedSingleHumanChangeNameWindow_l_Misc_DoChangeName(window);
					Close(window);
				}
			}

			return true;
		}

		// Original function: FUN_004c7bfa.
		void __fastcall SelectedSingleHumanChangeNameWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
		{
			NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);

			if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) != SwitchIds::SelectionType::SingleHuman || SelectedId() == -1)
			{

				return;
			}

			NC2E2::DrawAtom_MapMoveable_DoDraw(Read<std::uint32_t>(Human(SelectedId()), NC2Layout::HumanOffset::MapMoveableId), bitmap, 290, 80, nullptr, true, -1);
			auto* font = NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08;
			bitmap->Text_Print(font, White(), 10, 30, TextInput(window)->GetString());
			const std::uint8_t channel = ((timeGetTime() / NC2Runtime::HumanJobWindowTiming::CursorBlinkMillisecondsUnsigned) & 1u) != 0 ? 255 : 100;
			const NXBasics::SColorRGB color(channel, channel, channel);
			bitmap->Draw_VerticalLine(TextInput(window)->GetCursorPixelPosition(font) + 10, 32, 12, color);
		}

		// Original function: FUN_004c8129.
		bool __fastcall SelectedSingleHumanChangeProducedGoodWindow_MessageHandle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t, std::uintptr_t)
		{
			if (message == SwitchIds::GuiWindowMessage::Query)
			{
				if (command == SwitchIds::GuiWindowQuery::HumanChangeProducedGood)
					return true;

				if (command == SwitchIds::HumanJobWindowCommand::ValidateSelection)
					SelectedSingleHumanLearnJobWindow_l_System_Validate(window);

				return false;
			}

			if (message != SwitchIds::GuiWindowMessage::Command)
				return false;

			if (command >= SwitchIds::HumanJobWindowCommand::SelectRow && command < SwitchIds::HumanJobWindowCommand::ProducedGoodCommandEnd)
			{
				const auto id = SelectedId();

				if (id != -1)
				{
					Queue(id, SwitchIds::UserCommand::ChangeProducedGood, command - SwitchIds::HumanJobWindowCommand::SelectRow);
					Respond(id);
				}

				Close(window);
			}

			return true;
		}

		// Original function: FUN_004c81dd.
		void __fastcall SelectedSingleHumanChangeProducedGoodWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
		{
			NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);
			const auto id = SelectedId();

			if (id != -1)
				bitmap->Text_Print(NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08, White(), 4, 244, Name(id));
		}

		// Original function: FUN_004c8ecc.
		bool __fastcall SelectedSingleHumanLearnJobWindow_MessageHandle(void* window, void*, std::uint32_t message, std::uint32_t command, std::uintptr_t, std::uintptr_t jobPayload, std::uintptr_t goodPayload)
		{
			const auto job = static_cast<std::uint32_t>(jobPayload);
			const auto good = static_cast<std::uint32_t>(goodPayload);

			if (message == SwitchIds::GuiWindowMessage::Query)
			{
				if (command == SwitchIds::GuiWindowQuery::HumanLearnJob)
					return true;

				if (command == SwitchIds::HumanJobWindowCommand::ValidateSelection)
					SelectedSingleHumanLearnJobWindow_l_System_Validate(window);

				return false;
			}

			if (message != SwitchIds::GuiWindowMessage::Command)
				return false;

			if (command == SwitchIds::HumanJobWindowCommand::SelectRow)
			{
				const auto id = SelectedId();

				if (id != -1)
				{
					Queue(id, SwitchIds::UserCommand::LearnJob, job);

					if (good != 0)
						Queue(id, SwitchIds::UserCommand::LearnGood, good);

					Respond(id);
				}

				Close(window);
			}

			return true;
		}

		// Original function: FUN_004c8fa9.
		void __fastcall SelectedSingleHumanLearnJobWindow_Draw(void* window, void*, NXBasics::CBitmap* bitmap, const SRectangle* rectangle)
		{
			NC2InGameGuiBase::BaseToolWindow_Draw(window, nullptr, bitmap, rectangle);

			if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) == SwitchIds::SelectionType::SingleHuman && SelectedId() != -1)
				bitmap->Text_PrintV(NC2GuiToolsBase::CGuiBaseDataManager::mStaticVars.resources.font08, White(), 4, 284, "%s", Name(SelectedId()));
		}

		// Complete native nineteen-slot regions 004F76B0..004F76FB, 004F7710..004F775B and 004F7808..004F7853.
		static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries>& JobWindowVirtualTable()
		{
			static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries> methods = []
			{
				std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries> table{ };
				std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
				table[NC2Runtime::GuiVirtualSlot::DeletingDestructor] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeJobWindow_Delete>());
				table[NC2Runtime::GuiVirtualSlot::MessageHandle] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeJobWindow_MessageHandle>());
				table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeJobWindow_Draw>());

				return table;
			}();

			return methods;
		}

		static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries>& NameWindowVirtualTable()
		{
			static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries> methods = []
			{
				auto table = JobWindowVirtualTable();
				table[NC2Runtime::GuiVirtualSlot::DeletingDestructor] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeNameWindow_Delete>());
				table[NC2Runtime::GuiVirtualSlot::MessageHandle] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeNameWindow_MessageHandle>());
				table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeNameWindow_Draw>());

				return table;
			}();

			return methods;
		}

		static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries>& LearnWindowVirtualTable()
		{
			static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries> methods = []
			{
				std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries> table{ };
				std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
				table[NC2Runtime::GuiVirtualSlot::MessageHandle] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanLearnJobWindow_MessageHandle>());
				table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanLearnJobWindow_Draw>());

				return table;
			}();

			return methods;
		}

		// Complete native nineteen-slot region 004F775C..004F77A7.
		static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries>& ProducedGoodWindowVirtualTable()
		{
			static const std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries> methods = []
			{
				std::array<std::uintptr_t, NC2Runtime::GuiVirtualTable::BaseWindowEntries> table{ };
				std::memcpy(table.data(), NC2InGameGuiBase::gBaseToolWindowVirtualTable, sizeof(table));
				table[NC2Runtime::GuiVirtualSlot::MessageHandle] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeProducedGoodWindow_MessageHandle>());
				table[NC2Runtime::GuiVirtualSlot::Draw] = reinterpret_cast<std::uintptr_t>(NXBaseGui::NativeGuiVirtualEntry<&SelectedSingleHumanChangeProducedGoodWindow_Draw>());

				return table;
			}();

			return methods;
		}

		// Original function: FUN_004c73df.
		void* __fastcall SelectedSingleHumanChangeJobWindow_Delete(void* window, void*, std::uint32_t flags)
		{
			Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, JobWindowVirtualTable().data());
			NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_UnRegister(window);
			Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
			NXBaseGui::BaseWindow_Destruct(window);

			if ((flags & 1u) != 0)
				NXBasics::Runtime_Free(window);

			return window;
		}

		// Original function: FUN_004c7b39.
		void* __fastcall SelectedSingleHumanChangeNameWindow_Delete(void* window, void*, std::uint32_t flags)
		{
			Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NameWindowVirtualTable().data());
			TextInput(window)->Exit();
			Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NC2InGameGuiBase::gBaseToolWindowVirtualTable + 0);
			NXBaseGui::BaseWindow_Destruct(window);

			if ((flags & 1u) != 0)
				NXBasics::Runtime_Free(window);

			return window;
		}
	}

	// Original function: FUN_004c736e.
	void* SelectedSingleHumanChangeJobWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle rectangle((state.displayWidth - 300) / 2, (state.displayHeight - 320) / 2, 300, 320);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 0, NC2Layout::RecordSize::ChangeJobWindowState);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, JobWindowVirtualTable().data());
		SelectedSingleHumanChangeJobWindow_l_GuiButtons_Add(window);
		NInGameGuiManager::CInGameGuiManager::sTheObjectPtr->FrameCall_Register(window);

		return window;
	}

	// Original function: FUN_004c7a7d.
	void* SelectedSingleHumanChangeNameWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle rectangle((state.displayWidth - 320) / 2, (state.displayHeight - 100) / 2, 320, 100);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		static_assert(NC2Layout::SelectedHumanActionWindowOffset::NameInputHandler % alignof(NC2InGameGuiBaseTextInputElement::CStringInputHandler) == 0);
		static_assert(NC2Layout::ObjectSize::HumanChangeNameWindow == NC2Layout::SelectedHumanActionWindowOffset::NameInputHandler + sizeof(NC2InGameGuiBaseTextInputElement::CStringInputHandler));
		new (TextInput(window)) NC2InGameGuiBaseTextInputElement::CStringInputHandler;
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 0, NC2Layout::RecordSize::SelectedHumanActionWindowState);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, NameWindowVirtualTable().data());
		TextInput(window)->Init(12, true, true, 13, 0);

		if (Read<std::uint32_t>(Input(), NC2Layout::InputManagerOffset::SelectionType) != SwitchIds::SelectionType::SingleHuman || SelectedId() == -1)
		{
			Close(window);
		}
		else
		{
			TextInput(window)->SetString(Name(SelectedId()));
			SelectedSingleHumanChangeNameWindow_l_GuiButtons_Add(window);
		}

		return window;
	}

	// Original function: FUN_004c7ea7.
	void* SelectedSingleHumanChangeProducedGoodWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle rectangle((state.displayWidth - 200) / 2, (state.displayHeight - 260) / 2, 200, 260);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 0, NC2Layout::RecordSize::SelectedHumanActionWindowState);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, ProducedGoodWindowVirtualTable().data());
		Frame(window, 440);
		SelectedSingleHumanLearnJobWindow_l_System_Validate(window);

		if (Read<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection) == 0)
			return window;

		void* list = List(SRectangle(4, 22, 168, 216));
		const auto id = SelectedId();

		if (id != -1)
		{
			const void* human = Human(id);
			SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);

			for (std::uint32_t good = 1; good < NC2Runtime::HumanJobWindowLimits::Goods; ++good)
			{
				if (!NC2Logic::Human_IsCapableToProduceGood(human, good))
					continue;

				const bool available = NC2Logic::Human_IsAbleToProduceGoodNow(human, good);
				void* memory = NXBasics::Runtime_AllocateWithNewHandler(available ? NC2Layout::ObjectSize::GuiTextButton : NC2Layout::ObjectSize::GuiTextElement, 1);
				const char* caption = NC2Logic::StringAccess_GetGoodNamePtr(good, 1);
				void* element = nullptr;

				if (memory != nullptr)
					element = available ? NC2InGameGuiBase::BaseToolTextButton_ConstructToggle(memory, nullptr, &row, caption, SwitchIds::HumanJobWindowCommand::SelectRow + good, window, Read<std::uint32_t>(human, NC2Layout::HumanOffset::ProducedGoodType) == good) : NC2InGameGuiBase::BaseToolTextElement_Construct(memory, nullptr, &row, caption, false, false);

				BaseListBoxElement_Element_Add(list, nullptr, element);
				row.y += 24;
			}

			if (Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType) == 8)
			{
				void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::GuiTextButton, 1);
				NXBaseGui::CBaseElement* element = static_cast<NXBaseGui::CBaseElement*>(memory == nullptr ? nullptr : NC2InGameGuiBase::BaseToolTextButton_Construct(memory, nullptr, &row, NC2Logic::StringAccess_GetGoodNamePtr(65, 1), 10065, window));
				BaseListBoxElement_Element_Add(list, nullptr, element);
			}
		}

		AddList(window, list, SRectangle(176, 22, 20, 216));

		return window;
	}

	// Original function: FUN_004c8b04.
	void* SelectedSingleHumanLearnJobWindow_Construct(void* window)
	{
		const auto& state = NInGameGuiManager::CInGameGuiManager::mStaticVars;
		const SRectangle rectangle((state.displayWidth - 320) / 2, (state.displayHeight - 300) / 2, 320, 300);
		NC2InGameGuiBase::BaseToolWindow_Construct(window, &rectangle);
		std::memset(static_cast<std::byte*>(window) + NC2Layout::SelectedHumanActionWindowOffset::ValidSelection, 0, NC2Layout::RecordSize::SelectedHumanActionWindowState);
		Write(window, NC2Layout::GuiBaseElementOffset::VirtualTable, LearnWindowVirtualTable().data());
		Frame(window, 450);
		SelectedSingleHumanLearnJobWindow_l_System_Validate(window);

		if (Read<std::uint8_t>(window, NC2Layout::SelectedHumanActionWindowOffset::ValidSelection) == 0)
			return window;

		void* list = List(SRectangle(4, 22, 288, 256));
		const auto id = SelectedId();

		if (id != -1)
		{
			const void* human = Human(id);
			const auto player = Read<std::int32_t>(human, NC2Layout::HumanOffset::PlayerId);
			const auto tribe = Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType);
			std::uint32_t jobs[NC2Runtime::HumanJobWindowLimits::Jobs], count = 0;

			for (std::uint32_t job = SwitchIds::HumanJob::Builder; job <= SwitchIds::HumanJob::Druid; ++job)
			{
				const auto offset = player * NC2Layout::RecordSize::GameControlPlayerSigned + tribe * NC2Runtime::HumanJobWindowLimits::Jobs + job;

				if (Read<std::uint8_t>(NC2Logic::gStaticJobTypeData, job * NC2Layout::RecordSize::JobType + NC2Layout::StaticJobTypeOffset::CanBeTrainedUnsigned) != 0 && Read<std::uint8_t>(NC2Logic::gGameControl, offset + NC2Layout::GameControlPlayerOffset::AllowedJobs) != 0 && Read<std::uint8_t>(NC2Logic::gGameControl, offset + NC2Layout::GameControlPlayerOffset::AvailableJobs) != 0)
					jobs[count++] = job;
			}

			if (count > 1)
				NXBasics::Runtime_QuickSort(jobs, count, NC2Layout::RecordSize::HumanJobId, lsSortCompareFunction);

			SRectangle row(0, 0, Read<std::int32_t>(list, NC2Layout::GuiBaseElementOffset::Width), 22);

			for (std::uint32_t index = 0; index < count; ++index)
			{
				const auto job = jobs[index];
				const auto& enables = NC2Logic::gStaticTribeTypeStorage.records[tribe].jobEnables[job];

				if (enables.goodCount > 1)
				{
					for (std::uint32_t goodIndex = 0; goodIndex < enables.goodCount; ++goodIndex)
					{
						const auto good = enables.goods[goodIndex];

						if (job == SwitchIds::HumanJob::Collector && (good == 2 || good == 4 || good == 5 || good == 3))
							continue;

						const auto offset = player * NC2Layout::RecordSize::GameControlPlayerSigned + tribe * NC2Runtime::HumanJobWindowLimits::Goods + good;

						if (Read<std::uint8_t>(NC2Logic::gGameControl, offset + NC2Layout::GameControlPlayerOffset::AllowedGoods) == 0 || Read<std::uint8_t>(NC2Logic::gGameControl, offset + NC2Layout::GameControlPlayerOffset::AvailableGoods) == 0)
							continue;

						char caption[200];
						NXBasics::Runtime_PrintStringN(caption, sizeof(caption), "%s (%s)", NC2Logic::StringAccess_GetJobNamePtr(job, 1), NC2Logic::StringAccess_GetGoodNamePtr(good, 1));
						Row(window, list, row, caption, job, good, false);
					}
				}
				else
				{
					Row(window, list, row, NC2Logic::StringAccess_GetJobNamePtr(job, 1), job, static_cast<std::uint32_t>(enables.defaultGood), false);
				}
			}
		}

		AddList(window, list, SRectangle(296, 22, 20, 256));

		return window;
	}
}

namespace NInGameGuiManager
{
	// Original function: FUN_004a2683.
	void CInGameGuiManager::MiscWindows_HumanChangeJobWindow_Open()
	{
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::HumanChangeJob, 0, 0, 0) != nullptr)
		{

			return;
		}

		void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::HumanChangeJobWindow, 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(memory == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedSingleHumanChangeJobWindow_Construct(memory)), false);
	}

	// Original function: FUN_004a26c2.
	void CInGameGuiManager::MiscWindows_HumanChangeNameWindow_Open()
	{
		auto* desktop = NXBaseGui::CDesktop::sTheObjectPtr;

		if (desktop->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::HumanChangeName, 0, 0, 0) != nullptr)
		{

			return;
		}

		void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::HumanChangeNameWindow, 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(memory == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedSingleHumanChangeNameWindow_Construct(memory)), false);

		if (auto* window = desktop->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::HumanChangeName, 0, 0, 0))
			desktop->Window_BringToFront(reinterpret_cast<NXBaseGui::CBaseWindow*>(window));
	}

	// Original function: FUN_004a2723.
	bool CInGameGuiManager::MiscWindows_HumanChangeNameWindow_SendKey(std::uint32_t key, std::uint32_t character)
	{
		void* window = NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::HumanChangeName, 0, 0, 0);

		if (window == nullptr)
			return false;

		const auto table = NC2Lifetime::Read<const std::uintptr_t*>(window, NC2Layout::GuiBaseElementOffset::VirtualTable);

		return reinterpret_cast<bool(__thiscall*)(void*, std::uint32_t, std::uint32_t, std::uintptr_t, std::uintptr_t, std::uintptr_t)>(table[NC2Runtime::GuiVirtualSlot::MessageHandle])(window, SwitchIds::GuiWindowMessage::Command, SwitchIds::HumanJobWindowCommand::NameInputKey, key, character, 0);
	}

	// Original function: FUN_004a2761.
	void CInGameGuiManager::MiscWindows_HumanChangeProducedGoodWindow_Open()
	{
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::HumanChangeProducedGood, 0, 0, 0) != nullptr)
		{

			return;
		}

		void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::SelectedHumanActionWindow, 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(memory == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedSingleHumanChangeProducedGoodWindow_Construct(memory)), false);
	}

	// Original function: FUN_004a27ea.
	void CInGameGuiManager::MiscWindows_HumanLearnJobWindow_Open()
	{
		if (NXBaseGui::CDesktop::sTheObjectPtr->Message_SendToAllWindows(SwitchIds::GuiWindowQuery::HumanLearnJob, 0, 0, 0) != nullptr)
		{

			return;
		}

		void* memory = NXBasics::Runtime_AllocateWithNewHandler(NC2Layout::ObjectSize::SelectedHumanActionWindow, 1);
		NC2InGameGuiBase::BaseToolDesktop_AddWindow(static_cast<NXBaseGui::CBaseWindow*>(memory == nullptr ? nullptr : NC2InGameGuiSelectedAction::SelectedSingleHumanLearnJobWindow_Construct(memory)), false);
	}
}