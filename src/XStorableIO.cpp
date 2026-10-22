#include "SwitchIdentifiers.h"
#include "XBitmap.h"
#include "XBobManager.h"
#include "XCrtFileAccess.h"
#include "XDiagnostics.h"
#include "XFile.h"
#include "XFileAccess.h"
#include "XFont.h"
#include "XMemory.h"
#include "XPalette.h"
#include "XRemapTable.h"
#include "XStorableIO.h"
#include "XStringArray.h"

#include <cstdint>
#include <new>

namespace NXBasics
{
	// Original function: FUN_0041067e; only the fixed-width header is serialized.
	void CStorable::Storable_Save(void* file)
	{
		std::uint32_t id = Storable_GetId();
		std::uint32_t version = Storable_GetVersion();
		File_Write(file, &id, sizeof(id));
		File_Write(file, &version, sizeof(version));
		Storable_SaveData(file);
	}

	// Original allocation/free family used by the CStorable factory and deleting wrappers.
	// Windows factory 00410715 calls 004E5DDD in each of its seven allocation branches.
	void* CStorable::operator new(std::size_t size)
	{
		return Runtime_AllocateWithNewHandler(size);
	}
	void* CStorable::operator new(std::size_t size, const std::nothrow_t&) noexcept
	{
		return Runtime_AllocateWithNewHandler(size);
	}
	void CStorable::operator delete(void* allocation) noexcept
	{
		Runtime_Free(allocation);
	}
	void CStorable::operator delete(void* allocation, const std::nothrow_t&) noexcept
	{
		Runtime_Free(allocation);
	}

	// Original function: FUN_00410715
	void* XB_Storable_LoadObject(void* file)
	{
		std::uint32_t objectId;
		std::uint32_t version;

		File_Read(file, &objectId, 4);
		File_Read(file, &version, 4);

		switch (objectId)
		{
			case SwitchIds::StorableObjectId::Memory:
				return new (std::nothrow) CMemory(file, version);

			case SwitchIds::StorableObjectId::Bitmap:
				return new (std::nothrow) CBitmap(file, version);

			case SwitchIds::StorableObjectId::BobManager:
				return new (std::nothrow) CBobManager(file, version);

			case SwitchIds::StorableObjectId::Font:
				return new (std::nothrow) CFont(file, version);

			case SwitchIds::StorableObjectId::Palette:
				return new (std::nothrow) CPalette(file, version);

			case SwitchIds::StorableObjectId::RemapTable:
				return new (std::nothrow) CRemapTable(file, version);

			case SwitchIds::StorableObjectId::StringArray:
				return new (std::nothrow) CStringArray(file, version);

			default:
				NXBasics::ReportFatalError("NXBasics::XB_Storable_LoadObject  cannot load unknown object - class id %d", objectId);
				return nullptr;
		}
	}

	// Original function: FUN_0041083d
	void* XB_Storable_LoadObject(const char* fileName)
	{
		CFile file(fileName, true);

		return XB_Storable_LoadObject(static_cast<void*>(&file));
	}

	// Original function: FUN_00410876
	void XB_Storable_GetObjectIdAndVersion(void* file, std::uint32_t* objectId, std::uint32_t* version)
	{
		if (File_GetSize(file) < 8)
		{
			*objectId = 0;
			*version = 0;
			return;
		}

		const std::uint32_t originalPosition = File_GetPosition(file);

		File_Read(file, objectId, 4);
		File_Read(file, version, 4);

		File_SeekToPosition(file, originalPosition);
	}

	// Original function: FUN_004108c5
	void XB_Storable_GetObjectIdAndVersion(const char* fileName, std::uint32_t* objectId, std::uint32_t* version)
	{
		CFile file(fileName, true);

		XB_Storable_GetObjectIdAndVersion(static_cast<void*>(&file), objectId, version);
	}
}
