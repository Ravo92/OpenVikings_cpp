#pragma once

#include "C2LifetimeAccess.h"
#include "XBasicTypes.h"

namespace NXBasics
{
	class CBitmap;
	class CFont;
	class CPalette;
}

namespace NHyperLinkTextSystem
{
	using TUserBitmapCallback = NXBasics::CBitmap*(__cdecl*)(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t);

	// Complete Windows element suffix; pointers retain their constructor-established ownership.
	struct SHypertextGuiElementStorage
	{
		std::byte inheritedElement[44];
		void* resources;
		void* entities;
		void* display;
		std::uint8_t wheelScrollingEnabled, lastJumpSucceeded, fillBackground;
		std::byte padding3B;
		float autoScrollSpeed;
		std::uint32_t autoScrollStartTime, autoScrollDelay;
	};

	struct SDisplayManagerCallbackData
	{
		std::uint8_t called;
		std::byte padding[3];
		std::uint32_t values[3];
	};

	struct SFontListTag
	{
	};

	struct SBitmapListTag
	{
	};

	struct SPaletteListTag
	{
	};

	struct STextFileListTag
	{
	};

	struct SMicroEntityListTag
	{
	};

	struct SLineListTag
	{
	};

	struct SLineEntityListTag
	{
	};

}

namespace NC2Lifetime
{
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SLineListTag>::~CRuntimeOwnedList();
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SLineListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags);
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SLineEntityListTag>::~CRuntimeOwnedList();
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SLineEntityListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags);
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SMicroEntityListTag>::~CRuntimeOwnedList();
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SMicroEntityListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags);
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SFontListTag>::~CRuntimeOwnedList();
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SFontListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags);
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SBitmapListTag>::~CRuntimeOwnedList();
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SBitmapListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags);
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::SPaletteListTag>::~CRuntimeOwnedList();
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::SPaletteListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags);
	template <>
	CRuntimeOwnedList<NHyperLinkTextSystem::STextFileListTag>::~CRuntimeOwnedList();
	template <>
	void* CRuntimeOwnedList<NHyperLinkTextSystem::STextFileListTag>::Delete(CRuntimeOwnedList* list, std::uint32_t flags);
	template <>
	void CRuntimeOwnedList<NHyperLinkTextSystem::SLineListTag>::l_DeleteData(void* data);
	template <>
	void CRuntimeOwnedList<NHyperLinkTextSystem::SMicroEntityListTag>::l_DeleteData(void* data);
}

namespace NHyperLinkTextSystem
{
	class CDataContainer
	{
	  public:
		CDataContainer();
		~CDataContainer();
		NXBasics::CFont* GetFontPtr(const char* filename);
		NXBasics::CBitmap* GetBitmapPtr(const char* filename);
		NXBasics::CPalette* GetPalettePtr(const char* filename);
		std::uint32_t GetTextFileId(const char* filename);
		const char* GetTextFilePtr(std::uint32_t id);

		struct SVars
		{
			SVars();
			~SVars();
			NC2Lifetime::CRuntimeOwnedList<SFontListTag> fonts;
			NC2Lifetime::CRuntimeOwnedList<SBitmapListTag> bitmaps;
			NC2Lifetime::CRuntimeOwnedList<SPaletteListTag> palettes;
			NC2Lifetime::CRuntimeOwnedList<STextFileListTag> textFiles;
		} vars;
	};

	struct SMicroEntity
	{
		std::uint32_t type; // 1 controller, 2 text, 3 bitmap.

		union
		{
			std::uint32_t controller;
			char* text;
			NXBasics::CBitmap* bitmap;
		} content;

		union
		{
			std::uint32_t values[5];

			struct
			{
				std::uint8_t inlineBitmap;
				std::uint8_t userBitmap;
				std::byte padding[2];
				std::uint32_t arguments[4];
			} image;
		} data;

		SMicroEntity(const char* text, std::int32_t length);
	};

	void __stdcall MicroEntityList_DeleteData(void* data);
	void __stdcall LineHandlerList_DeleteData(void* data);

	class CEntityManager
	{
	  public:
		explicit CEntityManager(CDataContainer* resources);
		~CEntityManager();
		void Clear();
		bool AddString(char* text);
		bool AddTextFile(const char* filename, const char* block, bool insertLineBreaks);
		CDataContainer* resources;
		NC2Lifetime::CRuntimeOwnedList<SMicroEntityListTag>* entities;
	};

	struct SDisplayManagerLineEntity
	{
		std::uint8_t visible;
		std::byte padding01[3];
		NXBasics::SRectangle rectangle;
		SMicroEntity* entity;
		NXBasics::CFont* font;
		NXBasics::CPalette* palette;
		std::uint8_t jump;
		std::byte padding21[3];
		std::uint32_t textFile;
		std::uint32_t anchor;
	};

	struct SDisplayManagerLineHandler
	{
		explicit SDisplayManagerLineHandler(std::int32_t y);
		~SDisplayManagerLineHandler();
		std::int32_t y;
		std::int32_t height;
		NC2Lifetime::CRuntimeOwnedList<SLineEntityListTag>* entities;
	};

	// Eight-byte double alignment is explicit: +0x14 is unused Windows padding.
	class CDisplayManager
	{
	  public:
		CDisplayManager(CEntityManager* entities, CDataContainer* resources, std::int32_t width, std::int32_t height, SDisplayManagerCallbackData* callbacks, TUserBitmapCallback bitmapCallback);
		~CDisplayManager();
		void Draw(NXBasics::CBitmap* bitmap);
		void ScrollY(std::int32_t amount);
		void SetY(std::int32_t y);
		void HighlightEntityOnPoint(NXBasics::CBitmap* bitmap, const std::int32_t* point);
		bool ExecuteJumpFromPoint(const std::int32_t* point);
		void JumpToAnchor(std::uint32_t anchor);

		CEntityManager* entities;  // +0x00
		CDataContainer* resources; // +0x04
		std::int32_t width;        // +0x08
		std::int32_t height;       // +0x0C
		std::int32_t displayY;     // +0x10
		std::byte padding14[4];
		double preciseY;                                     // +0x18
		std::int32_t maximumY;                               // +0x20
		NC2Lifetime::CRuntimeOwnedList<SLineListTag>* lines; // +0x24
		SDisplayManagerCallbackData* callbacks;              // +0x28
		TUserBitmapCallback bitmapCallback;                  // +0x2C
		NXBasics::CFont* font;                               // +0x30
		NXBasics::CPalette* palette;                         // +0x34
		std::int32_t x;                                      // +0x38
		std::int32_t y;                                      // +0x3C
		SDisplayManagerLineHandler* currentLine;             // +0x40
		std::int32_t lineHeight;                             // +0x44
		std::uint8_t jump;                                   // +0x48
		std::byte padding49[3];
		std::uint32_t textFile;  // +0x4C
		std::uint32_t anchor;    // +0x50
		std::uint32_t alignment; // +0x54
		std::uint32_t xStart;    // +0x58
		std::uint32_t xEnd;      // +0x5C

	  private:
		void l_FillLinehandlerList();
		void l_DeleteAllLineHandlers();
		void l_Create_NewLine();
		void l_Create_NewLineHandler();
		void l_DrawEntity(NXBasics::CBitmap* bitmap, SDisplayManagerLineEntity* entity, bool highlight);
		void l_SetDisplayY(double y);
		SDisplayManagerLineEntity* l_GetDisplayManagerLineEntityOnPosition(const std::int32_t* point);
	};

	static_assert(sizeof(CDataContainer) == 80);
	static_assert(sizeof(SMicroEntity) == 28);
	static_assert(sizeof(CEntityManager) == 8);
	static_assert(sizeof(SDisplayManagerLineEntity) == 44);
	static_assert(sizeof(SDisplayManagerLineHandler) == 12);
	static_assert(sizeof(CDisplayManager) == 96);
	static_assert(offsetof(CDisplayManager, preciseY) == 0x18);
	static_assert(offsetof(CDisplayManager, xEnd) == 0x5C);
}

void* __fastcall HyperLinkTextGuiElementBase_Construct(void* element, void* unusedEdx, const NXBasics::SRectangle* rectangle, std::uint32_t flags, std::uint32_t messageMask, const char* directory, const char* filename, void* callbacks, void* bitmapCallback);
void __fastcall HyperLinkTextGuiElementBase_SetAutoScrollSpeed(void* element, void* unusedEdx, float speed, std::uint32_t delay);