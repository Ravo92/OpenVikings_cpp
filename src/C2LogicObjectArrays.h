#pragma once

#include <cstddef>
#include <cstdint>

namespace NC2Logic
{
	void FreeAllocatedRecords(void* manager, std::uint32_t stride);
	class CIoHelper;

	using TArrayIsAllocated = bool(__cdecl*)(void*);
	using TArrayClearElement = void(__cdecl*)(void*);
	using TArrayElementIO = void(__cdecl*)(void*, bool, CIoHelper*);
	using TArrayElementChecksum = std::uint32_t(__cdecl*)(void*);

	class CArrayHandler
	{
	  public:
		// Original function: FUN_00430c90
		CArrayHandler();
		CArrayHandler(const CArrayHandler&) = delete;
		CArrayHandler& operator=(const CArrayHandler&) = delete;

		// Original function: FUN_00430ca8
		void System_InitArray(std::int32_t capacity, std::byte* entries, std::uint32_t elementSize, TArrayIsAllocated isAllocated, TArrayClearElement clearElement, TArrayElementIO elementIO, TArrayElementChecksum elementChecksum);

		// Original function: FUN_00430d02
		void IO_InitAndLoadArray(std::byte* entries, CIoHelper* io, TArrayIsAllocated isAllocated, TArrayClearElement clearElement, TArrayElementIO elementIO, TArrayElementChecksum elementChecksum);

		// Original function: FUN_00430db7
		void IO_SaveArray(CIoHelper* io);

		std::int32_t Element_Allocate();
		bool Element_Free(std::int32_t index, bool clearElement);
		std::uint32_t Checksum_Calculate();

		// Original function: FUN_00430f23
		void l_Element_Clear(std::int32_t index);

		std::uint8_t mInitialized;              // +0x00: array initialization state
		std::byte padding01[3];                 // +0x01: alignment bytes
		std::int32_t mCapacity;                 // +0x04: number of available slots
		std::uint32_t mElementSize;             // +0x08: element stride in bytes
		std::int32_t mNextFreeIndex;            // +0x0C: next allocation search position
		std::int32_t mAllocatedCount;           // +0x10: number of allocated elements
		std::int32_t mFirstScanIndex;           // +0x14: lower occupied-range scan bound
		std::int32_t mLastScanIndex;            // +0x18: upper occupied-range scan bound
		std::byte* mEntries;                    // +0x1C: element storage
		TArrayIsAllocated mIsAllocated;         // +0x20: allocation-state callback
		TArrayClearElement mClearElement;       // +0x24: optional element-clear callback
		TArrayElementIO mElementIO;             // +0x28: optional element load/save callback
		TArrayElementChecksum mElementChecksum; // +0x2C: optional element-checksum callback

	  protected:
		// Original function: FUN_00430ca4
		~CArrayHandler();

	  private:
		std::byte* l_Element_GetPtr(std::uint32_t index);
		std::uint32_t l_Element_CalculateChecksum(std::int32_t index);
	};

	static_assert(sizeof(CArrayHandler) == (sizeof(void*) == 8 ? 72 : 48));
	static_assert(offsetof(CArrayHandler, mLastScanIndex) + sizeof(std::int32_t) - offsetof(CArrayHandler, mCapacity) == 24);
	static_assert(offsetof(CArrayHandler, mClearElement) == (sizeof(void*) == 8 ? 48 : 36));
	static_assert(offsetof(CArrayHandler, mElementIO) == (sizeof(void*) == 8 ? 56 : 40));
	static_assert(offsetof(CArrayHandler, mCapacity) == 0x04);
	static_assert(offsetof(CArrayHandler, mAllocatedCount) == 0x10);
	static_assert(offsetof(CArrayHandler, mEntries) == (sizeof(void*) == 8 ? 32 : 28));
	static_assert(offsetof(CArrayHandler, mIsAllocated) == (sizeof(void*) == 8 ? 40 : 32));
	static_assert(offsetof(CArrayHandler, mElementChecksum) == (sizeof(void*) == 8 ? 64 : 44));

	// Complete CMapMoveableArray static region at 00510A18..00510A1F.
	// Its original constructors/destructor remain external ownership boundaries.
	struct SMapMoveableArrayStorage
	{
		std::byte* entries;
		CArrayHandler* manager;
	};

	extern SMapMoveableArrayStorage gMapMoveableArrayStorage;
	static_assert(sizeof(SMapMoveableArrayStorage) == 2 * sizeof(void*));
	static_assert(offsetof(SMapMoveableArrayStorage, manager) == sizeof(void*));

	class CHouseArray : public CArrayHandler
	{
	  public:
		inline static std::byte* mStaticVars = nullptr;     // Original global: DAT_005109f8
		inline static CHouseArray* sTheObjectPtr = nullptr; // Original global: DAT_005109fc

		static constexpr std::int32_t InitialCapacity = 400;
		static constexpr std::uint32_t ElementSize = 0x7E0;
		// Address arithmetic is native-width; the serialized element-size word remains a DWORD.
		static constexpr std::size_t RuntimeElementSize = ElementSize;
		static constexpr std::uint32_t CallbackType = 9;
		static constexpr std::uint32_t SaveChunkId = 0x78646861;
		static constexpr std::uint32_t SaveVersion = 2;

		CHouseArray(); // Original function: FUN_0040aa24
		std::int32_t AllocateHouse(std::uint32_t player, std::uint32_t engineType, std::uint32_t stage, const void* point, bool finished, std::uint32_t flags);
		void FreeHouse(std::int32_t id);
		~CHouseArray();                         // Original function: FUN_0040aaec
		void IO_Savegame_DoSave(CIoHelper* io); // Original function: FUN_0040ab25
		explicit CHouseArray(CIoHelper* io);    // Original function: FUN_0040ab59
	};

	class CHumanArray : public CArrayHandler
	{
	  public:
		inline static std::byte* mStaticVars = nullptr;     // Original global: DAT_00510a00
		inline static CHumanArray* sTheObjectPtr = nullptr; // Original global: DAT_00510a04

		static constexpr std::int32_t InitialCapacity = 1000;
		static constexpr std::uint32_t ElementSize = 0xC58;
		// Address arithmetic is native-width; the serialized element-size word remains a DWORD.
		static constexpr std::size_t RuntimeElementSize = ElementSize;
		static constexpr std::uint32_t CallbackType = 3;
		static constexpr std::uint32_t SaveChunkId = 0x78646875;
		static constexpr std::uint32_t SaveVersion = 1;

		CHumanArray();                          // Original function: FUN_0040b271
		~CHumanArray();                         // Original function: FUN_0040b3c8
		void IO_Savegame_DoSave(CIoHelper* io); // Original function: FUN_0040b401
		std::int32_t AllocateHuman(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags);
		void FreeHuman(std::int32_t id);
		explicit CHumanArray(CIoHelper* io); // Original function: FUN_0040b435
	};

	class CVehicleArray : public CArrayHandler
	{
	  public:
		inline static std::byte* mStaticVars = nullptr;       // Original global: DAT_00510a08
		inline static CVehicleArray* sTheObjectPtr = nullptr; // Original global: DAT_00510a0c

		static constexpr std::int32_t InitialCapacity = 200;
		static constexpr std::uint32_t ElementSize = 0x54C;
		// Address arithmetic is native-width; the serialized element-size word remains a DWORD.
		static constexpr std::size_t RuntimeElementSize = ElementSize;
		static constexpr std::uint32_t CallbackType = 7;
		static constexpr std::uint32_t SaveChunkId = 0x78647661;
		static constexpr std::uint32_t SaveVersion = 1;

		CVehicleArray(); // Original function: FUN_0040b5fc
		std::int32_t AllocateVehicle(std::uint32_t player, std::uint32_t tribe, std::uint32_t type, const void* point, std::uint32_t missionCheck);
		void FreeVehicle(std::int32_t id);
		~CVehicleArray();                       // Original function: FUN_0040b740
		void IO_Savegame_DoSave(CIoHelper* io); // Original function: FUN_0040b779
		explicit CVehicleArray(CIoHelper* io);  // Original function: FUN_0040b7ad
	};

	class CAnimalArray : public CArrayHandler
	{
	  public:
		inline static std::byte* mStaticVars = nullptr;      // Original global: DAT_00511554
		inline static CAnimalArray* sTheObjectPtr = nullptr; // Original global: DAT_00511558

		static constexpr std::int32_t InitialCapacity = 500;
		static constexpr std::uint32_t ElementSize = 0x98;
		// Address arithmetic is native-width; the serialized element-size word remains a DWORD.
		static constexpr std::size_t RuntimeElementSize = ElementSize;
		static constexpr std::uint32_t CallbackType = 5;
		static constexpr std::uint32_t SaveChunkId = 0x78646161;
		static constexpr std::uint32_t SaveVersion = 1;

		CAnimalArray(); // Original function: FUN_0041c076
		std::int32_t AllocateAnimal(std::uint32_t player, std::uint32_t tribe, std::uint32_t job, const void* point, std::uint32_t missionCheck, std::uint32_t flags);
		void FreeAnimal(std::int32_t id);
		~CAnimalArray();                        // Original function: FUN_0041c16a
		void IO_Savegame_DoSave(CIoHelper* io); // Original function: FUN_0041c1a3
		explicit CAnimalArray(CIoHelper* io);   // Original function: FUN_0041c1d7
	};

	class CGuideArray : public CArrayHandler
	{
	  public:
		inline static std::byte* mStaticVars = nullptr;     // Original global: DAT_00512588
		inline static CGuideArray* sTheObjectPtr = nullptr; // Original global: DAT_0051258c

		static constexpr std::int32_t InitialCapacity = 400;
		static constexpr std::uint32_t ElementSize = 0x184;
		// Address arithmetic is native-width; the serialized element-size word remains a DWORD.
		static constexpr std::size_t RuntimeElementSize = ElementSize;
		static constexpr std::uint32_t CallbackType = 11;
		static constexpr std::uint32_t SaveChunkId = 0x78646761;
		static constexpr std::uint32_t SaveVersion = 1;

		CGuideArray();                          // Original function: FUN_004266a9
		~CGuideArray();                         // Original function: FUN_00426747
		void IO_Savegame_DoSave(CIoHelper* io); // Original function: FUN_00426780
		explicit CGuideArray(CIoHelper* io);    // Original function: FUN_004267b4
		std::int32_t AllocateGuide(std::uint32_t player, const void* point);
		void FreeGuide(std::int32_t id);
	};

	static_assert(sizeof(CHouseArray) == sizeof(CArrayHandler));
	static_assert(sizeof(CHumanArray) == sizeof(CArrayHandler));
	static_assert(sizeof(CVehicleArray) == sizeof(CArrayHandler));
	static_assert(sizeof(CAnimalArray) == sizeof(CArrayHandler));
	static_assert(sizeof(CGuideArray) == sizeof(CArrayHandler));

	const void* GetHumanPtr(std::int32_t id);
	void* GetHousePtr(std::int32_t id);
	void* GetVehiclePtr(std::int32_t id);
	void* GetMapMoveablePtr(std::int32_t id);
	bool HumanReference_IsAllocated(const void* reference);
	bool __cdecl ArrayTool_IsObjectAllocated(void* object);
	const void* GetAnimalPtr(std::int32_t id);
	bool Human_IsJobSoldier(const void* human);
	bool Human_IsJobHero(const void* human);
	bool Human_IsAgeAdult(const void* human);
	bool Human_IsMarried(const void* human);
	bool Human_IsSexMale(const void* human);
	bool Human_HasEquippedShoes(const void* human);
	bool Human_HasEquippedTools(const void* human);
	bool Human_IsAttachedToHomeHouse(const void* human);
	bool Human_IsAttachedToWorkHouse(const void* human);
}