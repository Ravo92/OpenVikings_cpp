#include "C2CallbackManager.h"
#include "C2ChecksumTools.h"
#include "C2DataIds.h"
#include "C2IoHelper.h"
#include "C2LifetimeAccess.h"
#include "C2LogicLifetimeGlobals.h"
#include "C2LogicObjectArrays.h"
#include "C2ObjectLayout.h"
#include "C2ObjectMemberConstructors.h"
#include "XCrtFileAccess.h"
#include "XMemoryChecksum.h"
#include "XNumericConstants.h"

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>

namespace NC2Logic
{
	// Source-level shared release loop; fixed-width record IDs and callback order are retained.
	void FreeAllocatedRecords(void* manager, std::uint32_t stride)
	{
		using namespace NC2Lifetime;
		auto* array = static_cast<CArrayHandler*>(manager);

		for (std::uint32_t index = static_cast<std::uint32_t>(array->mFirstScanIndex); index <= static_cast<std::uint32_t>(array->mLastScanIndex); ++index)
		{
			auto* entry = static_cast<std::byte*>(Read<void*>(manager, NC2Layout::OwnedArrayHandlerOffset::OwnedEntries)) + static_cast<std::size_t>(index) * stride;

			if (Read<std::uint8_t>(entry, NC2Layout::AllocatedRecordOffset::Allocated) != 0 && array->Element_Free(NXBasics::Runtime_TimeSigned32(index), true))
			{
				entry = static_cast<std::byte*>(Read<void*>(manager, NC2Layout::OwnedArrayHandlerOffset::OwnedEntries)) + static_cast<std::size_t>(index) * stride;
				Write<std::uint8_t>(entry, NC2Layout::AllocatedRecordOffset::Allocated, 0);
			}
		}
	}

	// Original function: FUN_00401be9; no allocation or bounds check.
	void* GetHousePtr(std::int32_t id)
	{

		return CHouseArray::mStaticVars + static_cast<std::uint32_t>(id) * CHouseArray::ElementSize;
	}

	// Original function: FUN_004020fb; no allocation or bounds check.
	void* GetVehiclePtr(std::int32_t id)
	{

		return CVehicleArray::mStaticVars + static_cast<std::uint32_t>(id) * CVehicleArray::ElementSize;
	}

	// Original function: FUN_0040210c; no allocation or bounds check.
	void* GetMapMoveablePtr(std::int32_t id)
	{

		return gMapMoveableArrayStorage.entries + static_cast<std::uint32_t>(id) * NC2Layout::RecordSize::MapMoveable;
	}

	// Original function: FUN_00430f4c.
	std::uint32_t CArrayHandler::Checksum_Calculate()
	{
		std::uint32_t checksum = 0x43211234u;
		Tool_Checksum_CalculateAndCombine(&checksum, &mCapacity, 0x18);
		auto index = mFirstScanIndex;

		while (index <= mLastScanIndex)
		{
			Tool_Checksum_Combine(&checksum, l_Element_CalculateChecksum(index));
			index = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) + 1u);
		}

		return checksum;
	}

	// Original function: FUN_00430f93.
	std::uint32_t CArrayHandler::l_Element_CalculateChecksum(std::int32_t index)
	{
		auto* entry = l_Element_GetPtr(static_cast<std::uint32_t>(index));

		if (!mIsAllocated(entry))
			return 0xF0F00000u;

		if (mElementChecksum != nullptr)
			return mElementChecksum(entry);

		return NXBasics::XB_GetMemoryChecksum(entry, mElementSize);
	}

	// Original function: FUN_00430e3d; allocation reserves a slot without setting its object flag.
	std::int32_t CArrayHandler::Element_Allocate()
	{
		if (mAllocatedCount == mCapacity)
			return -1;

		const auto index = mNextFreeIndex;
		mNextFreeIndex = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) + 1u);

		while (mNextFreeIndex != mCapacity && mIsAllocated(l_Element_GetPtr(static_cast<std::uint32_t>(mNextFreeIndex))))
			mNextFreeIndex = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(mNextFreeIndex) + 1u);

		mAllocatedCount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(mAllocatedCount) + 1u);

		if (index < mFirstScanIndex)
			mFirstScanIndex = index;

		if (index > mLastScanIndex)
			mLastScanIndex = index;

		return index;
	}

	// Original function: FUN_004417d2; a reference stores the ID and revision in DWORDs.
	bool HumanReference_IsAllocated(const void* reference)
	{
		const auto id = NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::HumanReferenceOffset::HumanId);

		if (id == NXBasics::IntegerRange::UInt32Maximum)
			return false;

		const auto* human = CHumanArray::mStaticVars + id * CHumanArray::ElementSize;

		return NC2Lifetime::Read<std::uint8_t>(human, NC2Layout::HumanOffset::AllocatedFlag) != 0 && NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::UniqueId) == NC2Lifetime::Read<std::uint32_t>(reference, NC2Layout::HumanReferenceOffset::UniqueId);
	}

	// Original function: FUN_0042d2ee.
	bool Human_HasEquippedShoes(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedShoes) != 0 && NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::ShoeCondition) != 0;
	}

	// Original function: FUN_0042d304.
	bool Human_HasEquippedTools(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::EquippedTools) != 0 && NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::ToolCondition) != 0;
	}

	// Original function: FUN_0040211a; IDs are deliberately not range checked.
	const void* GetHumanPtr(std::int32_t id)
	{

		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(CHumanArray::mStaticVars) + static_cast<std::uint32_t>(id) * CHumanArray::ElementSize);
	}

	// Original function: FUN_004077a7.
	const void* GetAnimalPtr(std::int32_t id)
	{

		return reinterpret_cast<const void*>(reinterpret_cast<std::uintptr_t>(CAnimalArray::mStaticVars) + static_cast<std::uint32_t>(id) * CAnimalArray::ElementSize);
	}

	// Original function: FUN_0042c62b.
	bool Human_IsJobSoldier(const void* human)
	{
		std::int32_t job;
		std::memcpy(&job, static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::JobType, sizeof(job));

		return job >= 0x1F && job <= 0x29;
	}

	// Original function: FUN_0042c63e.
	bool Human_IsJobHero(const void* human)
	{
		std::int32_t job;
		std::memcpy(&job, static_cast<const std::byte*>(human) + NC2Layout::HumanOffset::JobType, sizeof(job));

		return job >= 0x2A && job <= 0x2F;
	}

	// Original function: FUN_0040ad6c
	bool __cdecl ArrayTool_IsObjectAllocated(void* object)
	{

		return *static_cast<const std::uint8_t*>(object) != 0;
	}

	template <typename T>
	static void Array_ConstructMembers(std::byte* entries, std::uint32_t count)
	{
		if (entries == nullptr)
		{

			return;
		}

		for (std::uint32_t index = 0; index < count; ++index)
		{
			auto* entry = entries + static_cast<std::size_t>(index) * T::ElementSize;

			if constexpr (std::is_same_v<T, CHouseArray>)
			{
				House_ConstructMembers(entry, nullptr);
			}
			else if constexpr (std::is_same_v<T, CHumanArray>)
			{
				Human_ConstructMembers(entry, nullptr);
			}
			else if constexpr (std::is_same_v<T, CVehicleArray>)
			{
				Vehicle_ConstructMembers(entry, nullptr);
			}
			else if constexpr (std::is_same_v<T, CAnimalArray>)
			{
				Animal_ConstructStateMembers(entry + NC2Layout::AnimalOffset::ConstructedState, nullptr);
			}
		}
	}

	template <typename T>
	static void Array_Construct(T* array)
	{
		T::sTheObjectPtr = array;
		std::memset(static_cast<CArrayHandler*>(array), 0, sizeof(CArrayHandler));

		T::mStaticVars = nullptr;

		constexpr std::size_t allocationSize = static_cast<std::size_t>(T::InitialCapacity) * T::ElementSize;

		T::mStaticVars = static_cast<std::byte*>(NXBasics::Runtime_AllocateWithNewHandler(allocationSize, 1));

		Array_ConstructMembers<T>(T::mStaticVars, T::InitialCapacity);
		std::memset(T::mStaticVars, 0, allocationSize);

		array->System_InitArray(T::InitialCapacity, T::mStaticVars, T::ElementSize, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);

		CCallbackManager::sTheObjectPtr->ExecuteCallback(T::CallbackType, true, static_cast<std::uint32_t>(array->mCapacity), 0, 0);
	}

	template <typename T>
	static void Array_Destruct(T* array)
	{
		CCallbackManager::sTheObjectPtr->ExecuteCallback(T::CallbackType, false, 0, 0, 0);

		NXBasics::Runtime_Free(T::mStaticVars);

		T::mStaticVars = nullptr;
		T::sTheObjectPtr = nullptr;

		// CArrayHandler's destructor clears the initialized flag afterwards.
	}

	template <typename T>
	static void Array_PrepareLoad(T* array)
	{
		T::sTheObjectPtr = array;
		std::memset(static_cast<CArrayHandler*>(array), 0, sizeof(CArrayHandler));

		// The original does not reset mStaticVars before validating the input.
	}

	template <typename T>
	static void Array_LoadStorage(T* array, CIoHelper* io)
	{
		const std::uint32_t capacity = io->IO_File_ReadLong();
		const std::size_t allocationSize = static_cast<std::size_t>(capacity) * T::ElementSize;

		T::mStaticVars = static_cast<std::byte*>(NXBasics::Runtime_AllocateArrayWithNewHandler(capacity, T::ElementSize));

		Array_ConstructMembers<T>(T::mStaticVars, capacity);
		std::memset(T::mStaticVars, 0, allocationSize);

		array->IO_InitAndLoadArray(T::mStaticVars, io, ArrayTool_IsObjectAllocated, nullptr, nullptr, nullptr);

		CCallbackManager::sTheObjectPtr->ExecuteCallback(T::CallbackType, true, static_cast<std::uint32_t>(array->mCapacity), 0, 0);
	}

	template <typename T>
	static void Array_LoadOpenedChunk(T* array, CIoHelper* io)
	{
		Array_PrepareLoad(array);

		if (io->mReadHeader.version != T::SaveVersion)
		{
			io->IO_File_Chunk_Skip();

			return;
		}

		Array_LoadStorage(array, io);
	}

	template <typename T>
	static void Array_Save(T* array, CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(T::SaveChunkId, T::SaveVersion);
		io->IO_Memory_WriteLong(static_cast<std::uint32_t>(array->mCapacity));

		array->IO_SaveArray(io);

		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_00430c90
	CArrayHandler::CArrayHandler()
	{
		std::memset(this, 0, sizeof(*this));
	}

	// Original function: FUN_00430ca4
	CArrayHandler::~CArrayHandler()
	{
		mInitialized = 0;
	}

	// Original function: FUN_00430ca8
	void CArrayHandler::System_InitArray(std::int32_t capacity, std::byte* entries, std::uint32_t elementSize, TArrayIsAllocated isAllocated, TArrayClearElement clearElement, TArrayElementIO elementIO, TArrayElementChecksum elementChecksum)
	{
		std::memset(this, 0, sizeof(*this));

		mEntries = entries;
		mElementSize = elementSize;
		mIsAllocated = isAllocated;
		mClearElement = clearElement;
		mElementIO = elementIO;
		mElementChecksum = elementChecksum;
		mCapacity = capacity;

		for (std::int32_t index = 0; index < mCapacity; ++index)
		{
			l_Element_Clear(index);
		}

		mInitialized = 1;
	}

	// Original function: FUN_00430d02
	void CArrayHandler::IO_InitAndLoadArray(std::byte* entries, CIoHelper* io, TArrayIsAllocated isAllocated, TArrayClearElement clearElement, TArrayElementIO elementIO, TArrayElementChecksum elementChecksum)
	{
		std::memset(this, 0, sizeof(*this));

		mEntries = entries;
		mIsAllocated = isAllocated;
		mClearElement = clearElement;
		mElementIO = elementIO;
		mElementChecksum = elementChecksum;

		std::uint32_t version = 0;

		if (!io->IO_File_Chunk_CheckMarkerAndGetVersion(NC2DataIds::ArrayChunk::Header, &version, nullptr))
		{

			return;
		}

		// The original reads all six serialized state fields without validating the version.
		io->IO_File_ReadBinary(&mCapacity, NC2SaveFormat::ArrayHeaderBytes);

		for (std::int32_t index = 0; index < mCapacity; ++index)
		{
			l_Element_Clear(index);
		}

		for (std::int32_t ordinal = 0; ordinal < mAllocatedCount; ++ordinal)
		{
			const std::uint32_t index = io->IO_File_ReadLong();
			std::byte* element = l_Element_GetPtr(index);

			if (mElementIO != nullptr)
			{
				mElementIO(element, true, io);
			}
			else
			{
				io->IO_File_ReadBinary(element, mElementSize);
			}
		}

		mInitialized = 1;
	}

	// Original function: FUN_00430db7
	void CArrayHandler::IO_SaveArray(CIoHelper* io)
	{
		io->IO_Tool_Chunk_StartCreation(NC2DataIds::ArrayChunk::Header, 1u);
		io->IO_Memory_WriteBinary(&mCapacity, NC2SaveFormat::ArrayHeaderBytes);

		for (std::int32_t index = 0; index < mCapacity; ++index)
		{
			std::byte* element = l_Element_GetPtr(static_cast<std::uint32_t>(index));

			if (!mIsAllocated(element))
				continue;

			io->IO_Memory_WriteLong(static_cast<std::uint32_t>(index));

			if (mElementIO != nullptr)
			{
				mElementIO(element, false, io);
			}
			else
			{
				io->IO_Memory_WriteBinary(element, mElementSize);
			}
		}

		io->IO_Tool_Chunk_FinishAndWrite();
	}

	// Original function: FUN_00430e90. The second argument is unused in Windows.
	bool CArrayHandler::Element_Free(std::int32_t index, bool /*clearElement*/)
	{
		if (!mIsAllocated(l_Element_GetPtr(static_cast<std::uint32_t>(index))))
		{
			return false;
		}

		mAllocatedCount = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(mAllocatedCount) - 1u);

		if (mAllocatedCount == 0)
		{
			mLastScanIndex = 0;
			mFirstScanIndex = 0;
			mNextFreeIndex = 0;
			return true;
		}

		if (index < mNextFreeIndex)
		{
			mNextFreeIndex = index;
		}

		if (index == mFirstScanIndex)
		{
			mFirstScanIndex = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) + 1u);

			while (mFirstScanIndex <= mLastScanIndex && !mIsAllocated(l_Element_GetPtr(static_cast<std::uint32_t>(mFirstScanIndex))))
			{
				mFirstScanIndex = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(mFirstScanIndex) + 1u);
			}
		}

		if (index == mLastScanIndex)
		{
			mLastScanIndex = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(index) - 1u);

			while (mLastScanIndex >= mFirstScanIndex && !mIsAllocated(l_Element_GetPtr(static_cast<std::uint32_t>(mLastScanIndex))))
			{
				mLastScanIndex = NXBasics::Runtime_TimeSigned32(static_cast<std::uint32_t>(mLastScanIndex) - 1u);
			}
		}

		return true;
	}

	// Original function: FUN_00430f23
	void CArrayHandler::l_Element_Clear(std::int32_t index)
	{
		std::byte* element = l_Element_GetPtr(static_cast<std::uint32_t>(index));

		if (mClearElement != nullptr)
		{
			mClearElement(element);
		}
		else
		{
			std::memset(element, 0, mElementSize);
		}
	}

	std::byte* CArrayHandler::l_Element_GetPtr(std::uint32_t index)
	{
		const std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(mEntries);
		const std::uintptr_t address = baseAddress + static_cast<std::size_t>(index) * mElementSize;

		return reinterpret_cast<std::byte*>(static_cast<std::uintptr_t>(address));
	}

	// Original function: FUN_0040aa24
	CHouseArray::CHouseArray()
	{
		Array_Construct(this);
	}

	// Original function: FUN_0040aaec
	CHouseArray::~CHouseArray()
	{
		Array_Destruct(this);
	}

	// Original function: FUN_0040ab25
	void CHouseArray::IO_Savegame_DoSave(CIoHelper* io)
	{
		Array_Save(this, io);
	}

	// Original function: FUN_0040ab59
	CHouseArray::CHouseArray(CIoHelper* io)
	{
		Array_LoadOpenedChunk(this, io);
	}

	// Original function: FUN_0040b271
	CHumanArray::CHumanArray()
	{
		Array_Construct(this);
	}

	// Original function: FUN_0040b3c8
	CHumanArray::~CHumanArray()
	{
		Array_Destruct(this);
	}

	// Original function: FUN_0040b401
	void CHumanArray::IO_Savegame_DoSave(CIoHelper* io)
	{
		Array_Save(this, io);
	}

	// Original function: FUN_0040b435
	CHumanArray::CHumanArray(CIoHelper* io)
	{
		Array_LoadOpenedChunk(this, io);
	}

	// Original function: FUN_0040b5fc
	CVehicleArray::CVehicleArray()
	{
		Array_Construct(this);
	}

	// Original function: FUN_0040b740
	CVehicleArray::~CVehicleArray()
	{
		Array_Destruct(this);
	}

	// Original function: FUN_0040b779
	void CVehicleArray::IO_Savegame_DoSave(CIoHelper* io)
	{
		Array_Save(this, io);
	}

	// Original function: FUN_0040b7ad
	CVehicleArray::CVehicleArray(CIoHelper* io)
	{
		Array_LoadOpenedChunk(this, io);
	}

	// Original function: FUN_0041c076
	CAnimalArray::CAnimalArray()
	{
		Array_Construct(this);
	}

	// Original function: FUN_0041c16a
	CAnimalArray::~CAnimalArray()
	{
		Array_Destruct(this);
	}

	// Original function: FUN_0041c1a3
	void CAnimalArray::IO_Savegame_DoSave(CIoHelper* io)
	{
		Array_Save(this, io);
	}

	// Original function: FUN_0041c1d7
	CAnimalArray::CAnimalArray(CIoHelper* io)
	{
		Array_LoadOpenedChunk(this, io);
	}

	// Original function: FUN_004266a9
	CGuideArray::CGuideArray()
	{
		Array_Construct(this);
	}

	// Original function: FUN_00426747
	CGuideArray::~CGuideArray()
	{
		Array_Destruct(this);
	}

	// Original function: FUN_00426780
	void CGuideArray::IO_Savegame_DoSave(CIoHelper* io)
	{
		Array_Save(this, io);
	}

	// Original function: FUN_004267b4
	CGuideArray::CGuideArray(CIoHelper* io)
	{
		Array_PrepareLoad(this);

		std::uint32_t version = 0;

		if (!io->IO_File_Chunk_CheckMarkerAndGetVersion(SaveChunkId, &version, nullptr))
			return;

		if (version != SaveVersion)
		{
			io->IO_File_Chunk_Skip();

			return;
		}

		Array_LoadStorage(this, io);
	}

	// Original function: FUN_0042c597.
	bool Human_IsAgeAdult(const void* human)
	{
		const auto job = NC2Lifetime::Read<std::int32_t>(human, NC2Layout::HumanOffset::JobType);

		return job < 1 || job > 4;
	}

	// Original function: FUN_0042c522.
	bool Human_IsSexMale(const void* human)
	{
		const auto job = NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::JobType);

		return !(job == 1 || job == 3 || job == 5 || (job == 47 && NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::TribeType) == 3));
	}

	// Original function: FUN_0042c652.
	bool Human_IsAttachedToHomeHouse(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::HomeHouseId) != NXBasics::IntegerRange::UInt32Maximum;
	}

	// Original function: FUN_0042c65f.
	bool Human_IsAttachedToWorkHouse(const void* human)
	{

		return NC2Lifetime::Read<std::uint32_t>(human, NC2Layout::HumanOffset::WorkAttachmentHouseId) != NXBasics::IntegerRange::UInt32Maximum;
	}
}