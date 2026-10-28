#include "C2DataIds.h"
#include "C2ObjectLayout.h"
#include "GuiBitmapToolElementAccess.h"
#include "HyperLinkTextSystem.h"
#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XBTokenList.h"
#include "XFile.h"
#include "XFont.h"
#include "XFontMetrics.h"
#include "XUnicodeFont.h"
#include "XTextEncoding.h"
#include "XGuiElementAccess.h"
#include "XMMouseManager.h"
#include "XNumericConstants.h"
#include "XPalette.h"
#include "XPicture.h"
#include "XStorableIO.h"

#include <type_traits>

namespace NHyperLinkTextSystem
{
	using namespace NXBasics;
	using namespace NC2Lifetime;
	namespace
	{
		// Complete token array 004F7D08..004F7D87 and token-list state 005694A4..005694AB.
		const SToken sTokenArray[] = {
			{ "font", SwitchIds::HyperlinkToken::Font },{ "color", SwitchIds::HyperlinkToken::Color },{ "picture", SwitchIds::HyperlinkToken::Picture },
			{ "userpicture", NC2DataIds::HyperLinkTextSystemToken::UserPicture },{ "icon", SwitchIds::HyperlinkToken::Icon },{ "usericon", NC2DataIds::HyperLinkTextSystemToken::UserIcon },
			{ "block", NC2DataIds::HyperLinkTextSystemToken::Block },{ "textxstart", NC2DataIds::HyperLinkTextSystemToken::TextXStart },{ "textxend", NC2DataIds::HyperLinkTextSystemToken::TextXEnd },
			{ "anchor", SwitchIds::HyperlinkToken::Anchor },{ "localjump", NC2DataIds::HyperLinkTextSystemToken::LocalJump },{ "globaljump", NC2DataIds::HyperLinkTextSystemToken::GlobalJump },
			{ "include", SwitchIds::HyperlinkToken::Include },{ "rem", NC2DataIds::HyperLinkTextSystemToken::Rem },{ "onscreencallback", NC2DataIds::HyperLinkTextSystemToken::OnScreenCallback },{ nullptr, 0 }
		};
		struct STokenState
		{
			const SToken* tokens; std::uint32_t initialized;
		};
		STokenState sTokenList{ };
		static_assert(sizeof(sTokenArray) == 128);
		static_assert(sizeof(STokenState) == 8);

		// Source-level helpers retain the DWORD arithmetic of Windows layout stores.
		std::int32_t Add(std::int32_t first, std::int32_t second)
		{
			return Runtime_TimeSigned32(static_cast<std::uint32_t>(first) + static_cast<std::uint32_t>(second));
		}
		std::int32_t Subtract(std::int32_t first, std::int32_t second)
		{
			return Runtime_TimeSigned32(static_cast<std::uint32_t>(first) - static_cast<std::uint32_t>(second));
		}

		template<class T, class... Args> T* AllocateOriginal(Args&&... args)
		{
			void* allocation = Runtime_AllocateWithNewHandler(sizeof(T), 1);
			return allocation == nullptr ? nullptr : ::new(allocation) T(static_cast<Args&&>(args)...);
		}
		template<class T> void DestroyOriginal(T* object)
		{
			if (object != nullptr)
			{
				if constexpr (std::is_base_of_v<CListBase, T>) T::Delete(object, 1);
				else
				{
					object->~T(); Runtime_Free(object);
				}
			}
		}
		void* FindResource(CListBase& list, const char* filename)
		{
			CListBase::Iterator iterator(list);
			for (void* entry = iterator.First(); entry != nullptr; entry = iterator.Next())
				if (Runtime_CompareStringsIgnoreCase(static_cast<const char*>(entry), filename) == 0) return entry;
			return nullptr;
		}
		void* AllocateResource(std::uint32_t size, const char* filename)
		{
			void* entry = Runtime_AllocateWithNewHandler(size, 1);
			std::memset(entry, 0, size);
			std::strncpy(static_cast<char*>(entry), filename, 259);
			return entry;
		}
		SMicroEntity* Controller(std::uint32_t type, std::uint32_t a = 0, std::uint32_t b = 0, std::uint32_t c = 0)
		{
			auto* entity = static_cast<SMicroEntity*>(Runtime_AllocateWithNewHandler(28, 1));
			if (entity != nullptr)
			{
				std::memset(entity, 0, 28);
				entity->type = 1;
				entity->content.controller = type;
				entity->data.values[0] = a;
				entity->data.values[1] = b;
				entity->data.values[2] = c;
			}
			return entity;
		}
		SMicroEntity* Image(CBitmap* bitmap, bool inlineBitmap, bool userBitmap, std::uint32_t a = 0, std::uint32_t b = 0, std::uint32_t c = 0, std::uint32_t d = 0)
		{
			auto* entity = static_cast<SMicroEntity*>(Runtime_AllocateWithNewHandler(28, 1));
			if (entity != nullptr)
			{
				entity->type = 3;
				entity->content.bitmap = bitmap;
				entity->data.image.inlineBitmap = inlineBitmap;
				entity->data.image.userBitmap = userBitmap;
				entity->data.image.arguments[0] = a;
				entity->data.image.arguments[1] = b;
				entity->data.image.arguments[2] = c;
				entity->data.image.arguments[3] = d;
			}
			return entity;
		}
		// Original function: FUN_004d1aff; token-list exit callback is empty.
		int __cdecl DestroyEntityTokenList()
		{
			return 0;
		}
	}

	// Original function: FUN_004d1c96. Native list vtables retain their payload deleters;
	// the Windows zeroed list state is reproduced by their non-owning constructors.
	CDataContainer::CDataContainer()
	{}

	// Original function: FUN_004d1cfb; resources are released before the four list owners.
	CDataContainer::~CDataContainer()
	{
		for (auto* list : { static_cast<CListBase*>(&vars.fonts), static_cast<CListBase*>(&vars.bitmaps), static_cast<CListBase*>(&vars.palettes), static_cast<CListBase*>(&vars.textFiles) })
			while (void* entry = list->l_Base_GetStartElement())
			{
				list->l_Base_RemoveElement(entry);
				if (list != &vars.textFiles)
				{
					DeleteStorable(Read<void*>(entry, NC2Layout::HyperlinkResourceOffset::Resource));
					if (list == &vars.bitmaps) DeleteStorable(Read<void*>(entry, NC2Layout::HyperlinkResourceOffset::Palette));
				}
				Runtime_Free(entry);
			}
	}

	// Original function: FUN_004d1df4.
	CFont* CDataContainer::GetFontPtr(const char* filename)
	{
		if (void* entry = FindResource(vars.fonts, filename)) return Read<CFont*>(entry, NC2Layout::HyperlinkResourceOffset::Resource);
		if (!CFile::FileSystem_Tool_FileExists(filename, true)) return nullptr;
		void* entry = AllocateResource(0x108, filename);
		auto* font = static_cast<CFont*>(XB_Storable_LoadObject(filename));
		Write(entry, NC2Layout::HyperlinkResourceOffset::Resource, font);
		vars.fonts.InsertAtEnd(entry);
		return font;
	}

	// Original function: FUN_004d1e95.
	CBitmap* CDataContainer::GetBitmapPtr(const char* filename)
	{
		if (void* entry = FindResource(vars.bitmaps, filename)) return Read<CBitmap*>(entry, NC2Layout::HyperlinkResourceOffset::Resource);
		if (!CFile::FileSystem_Tool_FileExists(filename, true)) return nullptr;
		void* entry = AllocateResource(0x10C, filename);
		CPicture picture(filename);
		auto* bitmap = picture.ExtractBitmap();
		Write(entry, NC2Layout::HyperlinkResourceOffset::Resource, bitmap);
		auto* palette = picture.ExtractPalette();
		Write(entry, NC2Layout::HyperlinkResourceOffset::Palette, palette);
		bitmap->SetPalettePtr(palette);
		vars.bitmaps.InsertAtEnd(entry);
		return bitmap;
	}

	// Original function: FUN_004d1f69.
	CPalette* CDataContainer::GetPalettePtr(const char* filename)
	{
		if (void* entry = FindResource(vars.palettes, filename)) return Read<CPalette*>(entry, NC2Layout::HyperlinkResourceOffset::Resource);
		if (!CFile::FileSystem_Tool_FileExists(filename, true)) return nullptr;
		void* entry = AllocateResource(0x108, filename);
		CPicture picture(filename);
		auto* palette = picture.ExtractPalette();
		Write(entry, NC2Layout::HyperlinkResourceOffset::Resource, palette);
		vars.palettes.InsertAtEnd(entry);
		return palette;
	}

	// Original function: FUN_004d2020; a missing file returns zero, also the first valid ID.
	std::uint32_t CDataContainer::GetTextFileId(const char* filename)
	{
		CListBase::Iterator iterator(vars.textFiles);
		std::uint32_t index = 0;
		for (void* entry = iterator.First(); entry != nullptr; entry = iterator.Next(), ++index)
			if (Runtime_CompareStringsIgnoreCase(static_cast<const char*>(entry), filename) == 0) return index;
		if (!CFile::FileSystem_Tool_FileExists(filename, true)) return 0;
		vars.textFiles.InsertAtEnd(AllocateResource(0x104, filename));
		return vars.textFiles.GetCount() - 1;
	}

	// Original function: FUN_004d20b7.
	const char* CDataContainer::GetTextFilePtr(std::uint32_t id)
	{
		if (id >= vars.textFiles.GetCount()) return nullptr;
		CListBase::Iterator iterator(vars.textFiles);
		std::uint32_t index = 0;
		for (void* entry = iterator.First(); entry != nullptr; entry = iterator.Next(), ++index)
			if (index == id) return static_cast<const char*>(entry);
		return nullptr;
	}

	// Original function: FUN_004d1335; text uses the explicit runtime allocation thunk.
	SMicroEntity::SMicroEntity(const char* source, std::int32_t length)
	{
		type = 2;
		content.text = static_cast<char*>(Runtime_AllocateWithNewHandler(static_cast<std::uint32_t>(length) + 1, 1));
		std::strncpy(content.text, source, length);
		content.text[length] = '\0';
	}

	// Original function: FUN_004d136b.
	CEntityManager::CEntityManager(CDataContainer* data) : resources(data), entities(AllocateOriginal<CRuntimeOwnedList<SMicroEntityListTag>>(false))
	{}

	// Original function: FUN_004d13ac.
	CEntityManager::~CEntityManager()
	{
		Clear(); DestroyOriginal(entities);
	}

	// Original function: FUN_004d13c3.
	void CEntityManager::Clear()
	{
		while (auto* entity = static_cast<SMicroEntity*>(entities->l_Base_RemoveFromStart()))
		{
			MicroEntityList_DeleteData(entity);
		}
	}

	// Original function: FUN_004d13f3; tags temporarily modify the caller's buffer.
	bool CEntityManager::AddString(char* cursor)
	{
		// Keep non-western text together so shaping, bidi and CJK wrapping see
		// paragraphs rather than the native one-entity-per-space-delimited-word form.
		const auto wide = Text_ToWide(cursor);
		bool unicodeParagraph = false;
		for (wchar_t character : wide) if (character > 0xFF) { unicodeParagraph = true; break; }
		if ((sTokenList.initialized & 1) == 0)
		{
			sTokenList.initialized |= 1;
			sTokenList.tokens = sTokenArray;
			Runtime_AtExit(&DestroyEntityTokenList);
		}
		while (*cursor != '\0')
		{
			if (*cursor == ' ' || *cursor == '\t' || *cursor == '\n' || *cursor == '\r')
			{
				++cursor; continue;
			}
			if (*cursor == '<')
			{
				char* end = std::strchr(cursor, '>');
				if (end == nullptr) return false;
				char* colon = std::strchr(cursor + 1, ':');
				if (colon != nullptr && colon > end) colon = nullptr;
				char* delimiter = colon == nullptr ? end : colon;
				*delimiter = '\0';
				*end = '\0';
				char* argument = colon == nullptr ? nullptr : colon + 1;
				const auto token = XB_Token_FindTokenInArray(sTokenList.tokens, cursor + 1);
				SMicroEntity* entity = nullptr;
				bool append = false;
				switch (token)
				{
					case SwitchIds::HyperlinkToken::Font:
						if (auto* font = resources->GetFontPtr(argument))
						{
							entity = Controller(1, reinterpret_cast<std::uint32_t>(font)); append = true;
						}
						break;
					case SwitchIds::HyperlinkToken::Color:
						if (auto* palette = resources->GetPalettePtr(argument))
						{
							entity = Controller(2, reinterpret_cast<std::uint32_t>(palette)); append = true;
						}
						break;
					case SwitchIds::HyperlinkToken::Picture: case SwitchIds::HyperlinkToken::Icon:
						if (auto* bitmap = resources->GetBitmapPtr(argument))
						{
							entity = Image(bitmap, token == SwitchIds::HyperlinkToken::Icon, false); append = true;
						}
						break;
					case SwitchIds::HyperlinkToken::UserPicture: case SwitchIds::HyperlinkToken::UserIcon:
						{
							std::uint32_t values[4]{ };
							char* number = argument;
							const unsigned count = token == SwitchIds::HyperlinkToken::UserPicture ? 1 : 4;
							for (unsigned index = 0; index < count; ++index)
							{
								if (!Runtime_IsDigit(*number)) break;
								values[index] = Runtime_StringToInteger(number);
								while (Runtime_IsDigit(*number)) ++number;
								if (*number != ',') break;
								++number;
							}
							entity = Image(nullptr, token == SwitchIds::HyperlinkToken::UserIcon, true, values[0], values[1], values[2], values[3]);
							append = true;
							break;
						}
					case SwitchIds::HyperlinkToken::BlockAlignment: case SwitchIds::HyperlinkToken::XStart: case SwitchIds::HyperlinkToken::XEnd: case SwitchIds::HyperlinkToken::Anchor: case SwitchIds::HyperlinkToken::LocalJump:
						{
							const std::uint32_t controller = token == SwitchIds::HyperlinkToken::BlockAlignment ? 3 : token == SwitchIds::HyperlinkToken::XStart ? 4 : token == SwitchIds::HyperlinkToken::XEnd ? 5 : token == SwitchIds::HyperlinkToken::Anchor ? 6 : 7;
							entity = Controller(controller, Runtime_StringToInteger(argument));
							append = true;
							break;
						}
					case SwitchIds::HyperlinkToken::GlobalJump:
						if (char* comma = std::strchr(argument, ','))
						{
							*comma = '\0';
							const auto file = resources->GetTextFileId(argument);
							*comma = ',';
							// Remaster: the anchor starts after the filename separator.
							entity = Controller(8, Runtime_StringToInteger(comma + 1), file);
							append = true;
						}
						break;
					case SwitchIds::HyperlinkToken::Include:
						if (argument != nullptr)
							if (char* comma = std::strchr(argument, ','))
							{
								char filename[260], block[260];
								*comma = '\0'; std::strcpy(filename, argument); *comma = ',';
								char* second = std::strchr(comma + 1, ',');
								*second = '\0'; std::strcpy(block, comma + 1); *second = ',';
								AddTextFile(filename, block, Runtime_StringToInteger(second + 1) != 0);
							}
						break;
					case SwitchIds::HyperlinkToken::OnscreenCallback:
						{
							const auto a = Runtime_StringToInteger(argument);
							char* comma = std::strchr(argument, ',');
							const auto b = Runtime_StringToInteger(comma + 1);
							const auto c = Runtime_StringToInteger(std::strchr(comma + 1, ',') + 1);
							entity = Controller(9, a, b, c);
							append = true;
							break;
						}
					default: break;
				}
				if (append) entities->InsertAtEnd(entity);
				*delimiter = '>';
				cursor = end + 1;
				continue;
			}
			if (cursor[0] == '\\' && cursor[1] == 'n')
			{
				entities->InsertAtEnd(Controller(0));
				cursor += 2;
				continue;
			}
			char* start = cursor;
			while (*cursor != '\0' && (unicodeParagraph || (*cursor != ' ' && *cursor != '\t' && *cursor != '\n' && *cursor != '\r')) && *cursor != '<' && !(cursor[0] == '\\' && cursor[1] == 'n')) ++cursor;
			entities->InsertAtEnd(AllocateOriginal<SMicroEntity>(start, static_cast<std::int32_t>(cursor - start)));
		}
		return true;
	}

	// Original function: FUN_004d1b00; Windows allocates exactly file-size plus one.
	bool CEntityManager::AddTextFile(const char* filename, const char* block, bool insertLineBreaks)
	{
		if (!CFile::FileSystem_Tool_FileExists(filename, true)) return false;
		char* allocation;
		{
			CFile file(filename, true);
			const auto size = file.GetSize();
			if (size == NXBasics::IntegerRange::UInt32Maximum) return false;
			allocation = static_cast<char*>(Runtime_AllocateWithNewHandler(size + 1, 1));
			if (allocation == nullptr) return false;
			file.Read(allocation, size);
			if (file.GetPosition() != size) { Runtime_Free(allocation); return false; }
			allocation[size] = '\0';
		}
		char* text = allocation;
		if (block != nullptr && std::strlen(block) != 0)
		{
			char beginMarker[100], endMarker[100];
			if (std::strlen(block) > sizeof(beginMarker) - sizeof("[blockstart:]")) { Runtime_Free(allocation); return false; }
			Runtime_PrintString(beginMarker, "[blockstart:%s]", block);
			Runtime_PrintString(endMarker, "[blockend:%s]", block);
			text = std::strstr(allocation, beginMarker);
			if (text == nullptr) { Runtime_Free(allocation); return false; }
			text += std::strlen(beginMarker);
			if (text[0] == '\r' && text[1] == '\n') text += 2;
			char* end = std::strstr(text, endMarker);
			if (end == nullptr) { Runtime_Free(allocation); return false; }
			*end = '\0';
		}
		if (!insertLineBreaks) AddString(text);
		else
		{
			char* start = text;
			for (; *text != '\0'; ++text)
				if (*text == '\r')
				{
					*text = '\0';
					AddString(start);
					char lineBreak[] = "\\n";
					AddString(lineBreak);
					start = text + 1;
				}
			if (start != text) AddString(start);
		}
		Runtime_Free(allocation);
		return true;
	}

	// Original function: FUN_004cb1c4.
	SDisplayManagerLineHandler::SDisplayManagerLineHandler(std::int32_t positionY) : y(positionY), height(0), entities(AllocateOriginal<CRuntimeOwnedList<SLineEntityListTag>>(false))
	{}

	// Original function: FUN_004cb1fd.
	SDisplayManagerLineHandler::~SDisplayManagerLineHandler()
	{
		while (void* entity = entities->l_Base_RemoveFromStart()) Runtime_Free(entity);
		DestroyOriginal(entities);
	}

	// Original function: FUN_004cb224; only the first 0x30 bytes are initially cleared.
	CDisplayManager::CDisplayManager(CEntityManager* entityManager, CDataContainer* data, std::int32_t displayWidth, std::int32_t displayHeight, SDisplayManagerCallbackData* callbackData, TUserBitmapCallback callback)
	{
		std::memset(this, 0, 48);
		entities = entityManager; resources = data; width = displayWidth; height = displayHeight;
		callbacks = callbackData; bitmapCallback = callback;
		lines = AllocateOriginal<CRuntimeOwnedList<SLineListTag>>(false);
		l_FillLinehandlerList();
	}

	// Original function: FUN_004cb28d.
	CDisplayManager::~CDisplayManager()
	{
		l_DeleteAllLineHandlers(); DestroyOriginal(lines);
	}

	// Original function: FUN_004cb2a4.
	void CDisplayManager::l_Create_NewLine()
	{
		auto* list = currentLine->entities;
		if (list->GetCount() != 0)
		{
			auto* last = static_cast<SDisplayManagerLineEntity*>(list->l_Base_GetEndElement());
			if (last->entity->type == 2 && font != nullptr) x = Subtract(x, Font_GetCharacterWidth(font, 'i'));
			CListBase::Iterator iterator(*list);
			if (alignment == 2 || alignment == 3)
			{
				const auto distance = xEnd - static_cast<std::uint32_t>(x);
				const auto offset = alignment == 2 ? distance >> 1 : distance;
				for (auto* entity = static_cast<SDisplayManagerLineEntity*>(iterator.First()); entity != nullptr; entity = static_cast<SDisplayManagerLineEntity*>(iterator.Next())) entity->rectangle.x += offset;
			}
			else if (alignment == 1 && static_cast<std::int32_t>(list->GetCount()) > 1)
			{
				const auto spacing = Runtime_TimeSigned32((xEnd - static_cast<std::uint32_t>(x)) << 8) / static_cast<std::int32_t>(list->GetCount() - 1);
				if (spacing <= 0xC00)
				{
					std::int32_t offset = 0;
					for (auto* entity = static_cast<SDisplayManagerLineEntity*>(iterator.First()); entity != nullptr; entity = static_cast<SDisplayManagerLineEntity*>(iterator.Next()), offset = Add(offset, spacing)) entity->rectangle.x = Add(entity->rectangle.x, offset >> 8);
				}
			}
		}
		currentLine->height = lineHeight != 0 ? lineHeight : 20;
		CListBase::Iterator iterator(*currentLine->entities);
		for (auto* entity = static_cast<SDisplayManagerLineEntity*>(iterator.First()); entity != nullptr; entity = static_cast<SDisplayManagerLineEntity*>(iterator.Next())) entity->rectangle.y = Add(currentLine->y, Subtract(currentLine->height, entity->rectangle.height));
		y = Add(y, currentLine->height);
		x = xStart;
		l_Create_NewLineHandler();
	}

	// Original function: FUN_004cb40c.
	void CDisplayManager::l_Create_NewLineHandler()
	{
		currentLine = AllocateOriginal<SDisplayManagerLineHandler>(y);
		lines->InsertAtEnd(currentLine);
		lineHeight = 0;
	}

	// Original function: FUN_004cb43b.
	void CDisplayManager::Draw(CBitmap* bitmap)
	{
		CListBase::Iterator lineIterator(*lines);
		for (auto* line = static_cast<SDisplayManagerLineHandler*>(lineIterator.First()); line != nullptr; line = static_cast<SDisplayManagerLineHandler*>(lineIterator.Next()))
			if (Add(line->y, line->height) >= displayY && line->y <= Add(height, displayY))
			{
				CListBase::Iterator iterator(*line->entities);
				for (auto* entity = static_cast<SDisplayManagerLineEntity*>(iterator.First()); entity != nullptr; entity = static_cast<SDisplayManagerLineEntity*>(iterator.Next())) l_DrawEntity(bitmap, entity, false);
			}
	}

	// Original function: FUN_004cb4b8.
	void CDisplayManager::l_DrawEntity(CBitmap* bitmap, SDisplayManagerLineEntity* lineEntity, bool highlight)
	{
		if (lineEntity->visible == 0) return;
		auto* entity = lineEntity->entity;
		const auto drawX = lineEntity->rectangle.x;
		const auto drawY = Subtract(lineEntity->rectangle.y, displayY);
		if (entity->type == 3)
		{
			if (entity->data.image.userBitmap != 0)
			{
				if (bitmapCallback == nullptr) return;
				auto* source = bitmapCallback(entity->data.image.arguments[0], entity->data.image.arguments[1], entity->data.image.arguments[2], entity->data.image.arguments[3]);
				if (source != nullptr) source->CopyIntoBitmap(bitmap, drawX, drawY);
				return;
			}
			auto* source = entity->content.bitmap;
			if (source == nullptr) return;
			if (!highlight) source->CopyIntoBitmap_ColorKeyed(bitmap, drawX, drawY, 0);
			else
			{
				auto* original = Read<CPalette*>(source, NC2Layout::BitmapStorageOffset::Palette);
				CPalette palette(*original);
				palette.Modify(SColorModifier(false, 1.5f, 1.5f, 1.5f));
				source->SetPalettePtr(&palette);
				source->CopyIntoBitmap_ColorKeyed(bitmap, drawX, drawY, 0);
				source->SetPalettePtr(original);
			}
		}
		else if (entity->type == 2)
		{
			const auto drawText = [&](const SColorRGB* color)
			{
				std::string legacy;
				if (!Font_TryLegacyText(lineEntity->font, entity->content.text, legacy) && Font_GetPixelWidth(lineEntity->font, entity->content.text) > lineEntity->rectangle.width)
					UnicodeFont_Print(lineEntity->font, bitmap, color, drawX, drawY, entity->content.text, lineEntity->rectangle.width);
				else bitmap->Text_Print(lineEntity->font, color, drawX, drawY, entity->content.text);
			};
			if (highlight)
			{
				CPalette palette(*lineEntity->palette);
				palette.Modify(SColorModifier(false, 1.5f, 1.5f, 1.5f));
				drawText(reinterpret_cast<const SColorRGB*>(palette.GetColorData() + 255 * 4));
			}
			else drawText(reinterpret_cast<const SColorRGB*>(lineEntity->palette->GetColorData() + 255 * 4));
		}
		else if (entity->type == 1 && entity->content.controller == 9)
		{
			if (callbacks != nullptr)
			{
				callbacks->called = 1;
				std::memcpy(callbacks->values, entity->data.values, 12);
			}
			lineEntity->visible = 0;
		}
	}

	// Original function: FUN_004cb69a; scrolling starts from the integer position.
	void CDisplayManager::ScrollY(std::int32_t amount)
	{
		l_SetDisplayY(Runtime_TimeSigned32(static_cast<std::uint32_t>(displayY) + static_cast<std::uint32_t>(amount)));
	}
	// Original function: FUN_004cb6b7.
	void CDisplayManager::SetY(std::int32_t positionY)
	{
		l_SetDisplayY(positionY);
	}
	// Original function: FUN_004cb6c8.
	void CDisplayManager::l_SetDisplayY(double positionY)
	{
		if (!(positionY >= 0)) positionY = 0;
		else if (positionY > maximumY) positionY = maximumY;
		preciseY = positionY;
		displayY = static_cast<std::int32_t>(Runtime_TruncateDoubleToInt64(positionY));
	}

	// Original function: FUN_004cb70a.
	void CDisplayManager::HighlightEntityOnPoint(CBitmap* bitmap, const std::int32_t* point)
	{
		if (auto* entity = l_GetDisplayManagerLineEntityOnPosition(point))
			if (entity->jump != 0) l_DrawEntity(bitmap, entity, true);
	}
	// Original function: FUN_004cb732.
	bool CDisplayManager::ExecuteJumpFromPoint(const std::int32_t* point)
	{
		auto* entity = l_GetDisplayManagerLineEntityOnPosition(point);
		if (entity == nullptr || entity->jump == 0) return false;
		if (entity->textFile == NXBasics::IntegerRange::UInt32Maximum) JumpToAnchor(entity->anchor);
		else
		{
			const char* filename = resources->GetTextFilePtr(entity->textFile);
			const auto target = entity->anchor;
			l_DeleteAllLineHandlers();
			entities->Clear();
			entities->AddTextFile(filename, "", false);
			l_FillLinehandlerList();
			JumpToAnchor(target);
		}
		return true;
	}
	// Original function: FUN_004cb7a6.
	void CDisplayManager::JumpToAnchor(std::uint32_t target)
	{
		CListBase::Iterator lineIterator(*lines);
		for (auto* line = static_cast<SDisplayManagerLineHandler*>(lineIterator.First()); line != nullptr; line = static_cast<SDisplayManagerLineHandler*>(lineIterator.Next()))
		{
			CListBase::Iterator iterator(*line->entities);
			for (auto* entry = static_cast<SDisplayManagerLineEntity*>(iterator.First()); entry != nullptr; entry = static_cast<SDisplayManagerLineEntity*>(iterator.Next()))
				if (entry->entity->type == 1 && entry->entity->content.controller == 6 && entry->entity->data.values[0] == target)
				{
					l_SetDisplayY(line->y); return;
				}
		}
	}
	// Original function: FUN_004cb824.
	SDisplayManagerLineEntity* CDisplayManager::l_GetDisplayManagerLineEntityOnPosition(const std::int32_t* point)
	{
		const auto positionY = Add(point[1], displayY);
		CListBase::Iterator lineIterator(*lines);
		for (auto* line = static_cast<SDisplayManagerLineHandler*>(lineIterator.First()); line != nullptr; line = static_cast<SDisplayManagerLineHandler*>(lineIterator.Next()))
			if (positionY <= Add(line->y, line->height) && positionY >= line->y)
			{
				CListBase::Iterator iterator(*line->entities);
				for (auto* entry = static_cast<SDisplayManagerLineEntity*>(iterator.First()); entry != nullptr; entry = static_cast<SDisplayManagerLineEntity*>(iterator.Next()))
					if (entry->visible != 0 && point[0] >= entry->rectangle.x && point[0] <= Subtract(Add(entry->rectangle.x, entry->rectangle.width), 1) && positionY >= entry->rectangle.y && positionY <= Subtract(Add(entry->rectangle.y, entry->rectangle.height), 1)) return entry;
			}
		return nullptr;
	}

	// Original function: FUN_004cb8b6.
	void CDisplayManager::l_FillLinehandlerList()
	{
		font = nullptr; palette = nullptr; x = 0; y = 0; jump = 0; alignment = 0; xStart = 0; xEnd = width;
		l_Create_NewLineHandler();
		CListBase::Iterator iterator(*entities->entities);
		for (auto* entity = static_cast<SMicroEntity*>(iterator.First()); entity != nullptr; entity = static_cast<SMicroEntity*>(iterator.Next()))
		{
			if (entity->type == 1)
			{
				switch (entity->content.controller)
				{
					case SwitchIds::HyperlinkController::NewLine: l_Create_NewLine(); break;
					case SwitchIds::HyperlinkController::Font: font = reinterpret_cast<CFont*>(entity->data.values[0]); break;
					case SwitchIds::HyperlinkController::Palette: palette = reinterpret_cast<CPalette*>(entity->data.values[0]); break;
					case SwitchIds::HyperlinkController::Alignment: alignment = entity->data.values[0]; break;
					case SwitchIds::HyperlinkController::XStart: alignment = 0; xStart = entity->data.values[0] <= static_cast<std::uint32_t>(width) ? entity->data.values[0] : width; x = xStart; break;
					case SwitchIds::HyperlinkController::XEnd: xEnd = entity->data.values[0] <= static_cast<std::uint32_t>(width) ? entity->data.values[0] : width; break;
					case SwitchIds::HyperlinkController::Anchor: case SwitchIds::HyperlinkController::OnscreenCallback:
						{
							auto* entry = static_cast<SDisplayManagerLineEntity*>(Runtime_AllocateWithNewHandler(44, 1));
							currentLine->entities->InsertAtEnd(entry);
							entry->visible = entity->content.controller == 9;
							entry->entity = entity;
							break;
						}
					case SwitchIds::HyperlinkController::LocalJump: jump = 1; textFile = NXBasics::IntegerRange::UInt32Maximum; anchor = entity->data.values[0]; break;
					case SwitchIds::HyperlinkController::GlobalJump: jump = 1; textFile = entity->data.values[1]; anchor = entity->data.values[0]; break;
					default: break;
				}
				continue;
			}
			CBitmap* image = nullptr;
			std::int32_t entityWidth, entityHeight;
			if (entity->type == 3)
			{
				if (entity->data.image.userBitmap != 0)
				{
					if (bitmapCallback == nullptr) continue;
					image = bitmapCallback(entity->data.image.arguments[0], entity->data.image.arguments[1], entity->data.image.arguments[2], entity->data.image.arguments[3]);
				}
				else image = entity->content.bitmap;
				if (image == nullptr) continue;
				entityWidth = image->GetRectangle().width;
				entityHeight = image->GetRectangle().height;
				if (entity->data.image.inlineBitmap == 0)
				{
					if (currentLine->entities->GetCount() != 0) l_Create_NewLine();
					auto* entry = static_cast<SDisplayManagerLineEntity*>(Runtime_AllocateWithNewHandler(44, 1));
					currentLine->entities->InsertAtEnd(entry);
					entry->visible = 1;
					entry->rectangle.SetVariables(Subtract(width, image->GetRectangle().width) / 2, y, entityWidth, entityHeight);
					entry->entity = entity; entry->palette = nullptr; entry->jump = 0;
					x = width; lineHeight = Add(entityHeight, 2);
					l_Create_NewLine();
					continue;
				}
			}
			else if (entity->type == 2)
			{
				if (font == nullptr) continue;
				entityWidth = Font_GetPixelWidth(font, entity->content.text);
				entityHeight = Runtime_TimeSigned32(Read<std::uint32_t>(font, NC2Layout::FontOffset::Height) * 3u) / 2;
				std::string legacy;
				if (!Font_TryLegacyText(font, entity->content.text, legacy))
				{
					const auto available = Runtime_TimeSigned32(xEnd - xStart);
					if (available > 0 && entityWidth > available)
					{
						entityWidth = available;
						entityHeight = UnicodeFont_Measure(font, entity->content.text, true, available);
					}
					else entityHeight = UnicodeFont_Measure(font, entity->content.text, true);
				}
			}
			else continue;
			if (Runtime_TimeSigned32(xEnd) < Add(x, entityWidth)) l_Create_NewLine();
			auto* entry = static_cast<SDisplayManagerLineEntity*>(Runtime_AllocateWithNewHandler(44, 1));
			currentLine->entities->InsertAtEnd(entry);
			entry->visible = 1;
			entry->rectangle.SetVariables(x, y, entityWidth, entityHeight);
			entry->entity = entity;
			entry->font = entity->type == 2 ? font : nullptr;
			entry->palette = entity->type == 2 ? palette : nullptr;
			entry->jump = jump != 0;
			if (jump != 0)
			{
				entry->textFile = textFile; entry->anchor = anchor;
			}
			jump = 0;
			x = Add(x, entityWidth);
			x = Add(x, Font_GetCharacterWidth(font, 'i'));
			if (lineHeight < entityHeight) lineHeight = entityHeight;
		}
		l_Create_NewLine();
		auto* last = static_cast<SDisplayManagerLineHandler*>(lines->l_Base_GetEndElement());
		maximumY = Add(last->y, Subtract(last->height, height));
		if (maximumY < 0) maximumY = 0;
	}

	// Original function: FUN_004cbc24.
	void CDisplayManager::l_DeleteAllLineHandlers()
	{
		while (void* line = lines->l_Base_RemoveFromStart()) LineHandlerList_DeleteData(line);
	}
}

namespace
{
	using namespace NHyperLinkTextSystem;
	using namespace NC2Lifetime;
	using namespace NXBasics;
	extern const std::uintptr_t gHypertextVirtualTable[19];

	// Original function: FUN_004cbddb.
	void HyperLinkTextGuiElementBase_Destruct(void* element)
	{
		Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gHypertextVirtualTable);
		CFile::FileSystem_SystemPathName_Clear("$local$");
		DestroyOriginal(Read<CDisplayManager*>(element, NC2Layout::HypertextGuiElementOffset::Display));
		DestroyOriginal(Read<CEntityManager*>(element, NC2Layout::HypertextGuiElementOffset::Entities));
		DestroyOriginal(Read<CDataContainer*>(element, NC2Layout::HypertextGuiElementOffset::Resources));
		NXBaseGui::BaseElement_Destruct(element);
	}
	// Original function: FUN_004cbdbf.
	void* __fastcall HyperLinkTextGuiElementBase_Delete(void* element, void*, std::uint32_t flags)
	{
		HyperLinkTextGuiElementBase_Destruct(element);
		if ((flags & 1) != 0) Runtime_Free(element);
		return element;
	}
	// Original function: FUN_004cbe39.
	void __fastcall HyperLinkTextGuiElementBase_Draw_Background(void* element, void*, CBitmap* bitmap)
	{
		if (Read<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::FillBackground) != 0) bitmap->Fill(SColorRGB(0, 0, 0));
	}
	// Original function: FUN_004cbe5c.
	void __fastcall HyperLinkTextGuiElementBase_XGui_BE_Element_Draw(void* element, void*, CBitmap* bitmap, const SRectangle*)
	{
		auto* display = Read<CDisplayManager*>(element, NC2Layout::HypertextGuiElementOffset::Display);
		const float speed = Read<float>(element, NC2Layout::HypertextGuiElementOffset::AutoScrollSpeed);
		if (speed > 0)
		{
			const auto elapsed = Runtime_TimeSigned32(::timeGetTime() - Read<std::uint32_t>(element, NC2Layout::HypertextGuiElementOffset::AutoScrollDelay) - Read<std::uint32_t>(element, NC2Layout::HypertextGuiElementOffset::AutoScrollStartTime));
			const double distance = static_cast<double>(elapsed < 0 ? 0 : elapsed) * speed * static_cast<double>(0.001f);
			display->SetY(static_cast<std::int32_t>(Runtime_TruncateDoubleToInt64(distance)));
		}
		using TBackground = void(__thiscall*)(void*, CBitmap*);
		Read<TBackground*>(element, NC2Layout::GuiBaseElementOffset::VirtualTable)[18](element, bitmap);
		display->Draw(bitmap);
		if ((Read<std::uint32_t>(element, NC2Layout::GuiBaseElementOffset::Flags) & NXBasics::BitMask::Bit8Signed) != 0)
		{
			std::int32_t point[2];
			std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, sizeof(point));
			NC2InGameGuiBase::BaseElement_GlobalToLocalCoordinates(element, point);
			display->HighlightEntityOnPoint(bitmap, point);
		}
	}
	// Original function: FUN_004cbef5.
	bool __fastcall HyperLinkTextGuiElementBase_XGui_BE_Message_Handle(void* element, void*, std::uint32_t message, std::uint32_t param1, std::uint32_t param2, std::uint32_t param3, std::uint32_t param4)
	{
		if (Read<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::WheelScrollingEnabled) != 0 && (message & NXBasics::BitMask::Bit8Signed) != 0)
		{
			Write<float>(element, NC2Layout::HypertextGuiElementOffset::AutoScrollSpeed, 0);
			Read<CDisplayManager*>(element, NC2Layout::HypertextGuiElementOffset::Display)->ScrollY(Runtime_TimeSigned32(param3) / -4);
			return true;
		}
		return NXBaseGui::BaseElement_Message_Handle(element, nullptr, message, param1, param2, param3, param4);
	}
	// Original function: FUN_004cbf39.
	void __fastcall HyperLinkTextGuiElementBase_XGui_BE_Element_DoPressedAction(void* element)
	{
		std::int32_t point[2];
		std::memcpy(point, NXSysMouseManager::CMouseManager::sTheObjectPtr, sizeof(point));
		NC2InGameGuiBase::BaseElement_GlobalToLocalCoordinates(element, point);
		Write<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::LastJumpSucceeded, Read<CDisplayManager*>(element, NC2Layout::HypertextGuiElementOffset::Display)->ExecuteJumpFromPoint(point));
	}
	// Complete nineteen-slot table 004F795C..004F79A7.
	const std::uintptr_t gHypertextVirtualTable[19] = {
		reinterpret_cast<std::uintptr_t>(&HyperLinkTextGuiElementBase_Delete), reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_HasSubElements),
		reinterpret_cast<std::uintptr_t>(&HyperLinkTextGuiElementBase_XGui_BE_Message_Handle), reinterpret_cast<std::uintptr_t>(&HyperLinkTextGuiElementBase_XGui_BE_Element_Draw),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Activate), reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_DeActivate),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonPressed), reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ButtonReleased),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_IsHit), reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_GetRealActiveElementPtr),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_FindElementOnPosition), reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ParentToLocalPosition),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Notify), reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_Available),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_ToolTip_AtOnce), reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_Hide_CanBeHidden),
		reinterpret_cast<std::uintptr_t>(&NXBaseGui::BaseElement_UnresolvedVirtual40), reinterpret_cast<std::uintptr_t>(&HyperLinkTextGuiElementBase_XGui_BE_Element_DoPressedAction),
		reinterpret_cast<std::uintptr_t>(&HyperLinkTextGuiElementBase_Draw_Background)
	};
}

// Original function: FUN_004cbcd6.
void* __fastcall HyperLinkTextGuiElementBase_Construct(void* element, void*, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t messageMask, const char* directory, const char* filename, void* callbacks, void* bitmapCallback)
{
	NXBaseGui::BaseElement_Construct(element, rectangle, flags, messageMask);
	NC2Lifetime::Write(element, NC2Layout::GuiBaseElementOffset::VirtualTable, gHypertextVirtualTable);
	std::memset(static_cast<std::byte*>(element) + NC2Layout::HypertextGuiElementOffset::Resources, 0, NC2Layout::RecordSize::HypertextGuiElementVars);
	NXBasics::CFile::FileSystem_SystemPathName_Set("$local$", directory);
	auto* resources = NHyperLinkTextSystem::AllocateOriginal<NHyperLinkTextSystem::CDataContainer>();
	NC2Lifetime::Write(element, NC2Layout::HypertextGuiElementOffset::Resources, resources);
	auto* entities = NHyperLinkTextSystem::AllocateOriginal<NHyperLinkTextSystem::CEntityManager>(resources);
	NC2Lifetime::Write(element, NC2Layout::HypertextGuiElementOffset::Entities, entities);
	char path[260];
	std::strcpy(path, "$local$\\");
	std::strcat(path, filename);
	entities->AddTextFile(path, "", false);
	auto* display = NHyperLinkTextSystem::AllocateOriginal<NHyperLinkTextSystem::CDisplayManager>(entities, resources, rectangle->width, rectangle->height, static_cast<NHyperLinkTextSystem::SDisplayManagerCallbackData*>(callbacks), reinterpret_cast<NHyperLinkTextSystem::TUserBitmapCallback>(bitmapCallback));
	NC2Lifetime::Write(element, NC2Layout::HypertextGuiElementOffset::Display, display);
	NC2Lifetime::Write<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::WheelScrollingEnabled, 1);
	NC2Lifetime::Write<std::uint8_t>(element, NC2Layout::HypertextGuiElementOffset::FillBackground, 1);
	return element;
}
// Original function: FUN_004cbf6c; unordered x87 comparison takes the zero branch.
void __fastcall HyperLinkTextGuiElementBase_SetAutoScrollSpeed(void* element, void*, float speed, std::uint32_t delay)
{
	NC2Lifetime::Write(element, NC2Layout::HypertextGuiElementOffset::AutoScrollSpeed, speed);
	if (speed < 0 || speed > 0)
	{
		NC2Lifetime::Write(element, NC2Layout::HypertextGuiElementOffset::AutoScrollStartTime, ::timeGetTime());
		NC2Lifetime::Write(element, NC2Layout::HypertextGuiElementOffset::AutoScrollDelay, delay);
	}
}
