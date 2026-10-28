#include "XBobManager.h"
#include "XCrtFileAccess.h"
#include "XFileAccess.h"
#include "XMemory.h"
#include "XStorableIO.h"

#include <cstring>
#include <memory>
#include <new>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace NXBasics
{
	// Original function: FUN_004398e9.
	void* CBobManager::Delete(std::uint32_t flags)
	{
		auto* result = this;
		this->~CBobManager();
		if ((flags & 1u) != 0) Runtime_Free(result);
		return result;
	}

	// Original function: FUN_00439905.
	CBobManager::~CBobManager()
	{
		if (mBobDataMemory != nullptr)
		{
			mBobDataMemory->Delete(1);
			mBobDataMemory = nullptr;
		}

		if (mPackedLineDataMemory != nullptr)
		{
			mPackedLineDataMemory->Delete(1);
			mPackedLineDataMemory = nullptr;
		}

		if (mLineControlMemory != nullptr)
		{
			mLineControlMemory->Delete(1);
			mLineControlMemory = nullptr;
		}

		l_System_InitObject();
	}

	// Original function: FUN_0043994d
	void CBobManager::l_System_InitObject()
	{
		static_assert(offsetof(CBobManager, mSerialized) == BobManagerOffset::FirstBobId);
		static_assert(offsetof(CBobManager, mBobData) == BobManagerOffset::BobData);
		static_assert(offsetof(CBobManager, mPackedLineData) == BobManagerOffset::PackedLineData);
		static_assert(offsetof(CBobManager, mLineControlData) == BobManagerOffset::LineControlData);
		static_assert(offsetof(CBobManager, mBobDataMemory) == BobManagerOffset::BobDataMemory);
		static_assert(offsetof(CBobManager, mPackedLineDataMemory) == BobManagerOffset::PackedLineDataMemory);
		static_assert(offsetof(CBobManager, mLineControlMemory) == BobManagerOffset::LineControlMemory);
		std::memset(&mSerialized, 0, sizeof(mSerialized));

		mUnknown20 = 0;
		mBobData = nullptr;
		mPackedLineData = nullptr;
		mLineControlData = nullptr;
		mBobDataMemory = nullptr;
		mPackedLineDataMemory = nullptr;
		mLineControlMemory = nullptr;

		mStaticVars = nullptr;
	}

	// Original function: FUN_0043997d
	CBobManager::CBobManager(void* file, std::uint32_t /*version*/)
	{
		l_System_InitObject();

		File_Read(file, &mSerialized, sizeof(mSerialized));

		if (mSerialized.bobCount != 0)
		{
			mBobDataMemory = static_cast<CMemory*>(XB_Storable_LoadObject(file));
			mPackedLineDataMemory = static_cast<CMemory*>(XB_Storable_LoadObject(file));
			mLineControlMemory = static_cast<CMemory*>(XB_Storable_LoadObject(file));
		}

		l_System_SetRawPointer();
	}

	// Remaster-only construction; allocation and destruction use the native CMemory owners.
	CBobManager::CBobManager(std::uint32_t firstBobId, std::uint32_t bobCount, const void* bobData, std::uint32_t bobDataSize, const void* packedLineData, std::uint32_t packedLineDataSize, const void* lineControlData, std::uint32_t lineControlDataSize)
	{
		l_System_InitObject();
		mSerialized.firstBobId = firstBobId;
		mSerialized.bobCount = bobCount;
		mSerialized.packedLineDataSize = packedLineDataSize;
		auto records = std::make_unique<CMemory>(bobDataSize);
		auto pixels = std::make_unique<CMemory>(packedLineDataSize);
		auto lines = std::make_unique<CMemory>(lineControlDataSize);
		if (bobDataSize != 0) std::memcpy(records->mData, bobData, bobDataSize);
		if (packedLineDataSize != 0) std::memcpy(pixels->mData, packedLineData, packedLineDataSize);
		if (lineControlDataSize != 0) std::memcpy(lines->mData, lineControlData, lineControlDataSize);
		mBobDataMemory = records.release(); mPackedLineDataMemory = pixels.release(); mLineControlMemory = lines.release();
		l_System_SetRawPointer();
	}

	// Remaster-only graphics copies; the native managers and their CMemory buffers stay unchanged.
	std::unique_ptr<CBobManager> CBobManager::CreateReducedCopy(std::uint32_t divisor, std::int32_t pivotX, std::int32_t pivotY) const
	{
		struct SFrame { std::uint32_t type; std::int32_t x, y, width, height; std::uint32_t firstLine; };
		struct SPixel { std::uint8_t index = 0, alpha = 0; bool visible = false; };
		static_assert(sizeof(SFrame) == 24);
		if (divisor < 2 || divisor > 64) throw std::runtime_error("Invalid bob reduction divisor.");
		if (mSerialized.bobCount != 0 && (mBobDataMemory == nullptr || mPackedLineDataMemory == nullptr || mLineControlMemory == nullptr || static_cast<std::uint64_t>(mSerialized.bobCount) * sizeof(SFrame) > mBobDataMemory->GetSize())) throw std::runtime_error("Invalid bob reduction source.");
		std::vector<SFrame> frames(mSerialized.bobCount);
		std::vector<std::uint8_t> packed;
		std::vector<std::uint32_t> lines;
		for (std::uint32_t i = 0; i < mSerialized.bobCount; ++i)
		{
			SFrame source; std::memcpy(&source, mBobData + static_cast<std::size_t>(i) * sizeof(SFrame), sizeof(source));
			if (source.type == 0) continue;
			if ((source.type != 1 && source.type != 2 && source.type != 4) || source.width <= 0 || source.width > 1048576 || source.height <= 0 || (static_cast<std::uint64_t>(source.firstLine) + static_cast<std::uint32_t>(source.height)) * 4 > mLineControlMemory->GetSize()) throw std::runtime_error("Unsupported or invalid bob reduction frame.");
			auto& target = frames[i]; target = source;
			const auto reducePosition = [divisor] (std::int64_t value) { return static_cast<std::int32_t>(value >= 0 ? value / divisor : -((-value + divisor - 1) / divisor)); };
			target.x = pivotX + reducePosition(static_cast<std::int64_t>(source.x) - pivotX); target.y = pivotY + reducePosition(static_cast<std::int64_t>(source.y) - pivotY);
			target.width = static_cast<std::int32_t>((static_cast<std::uint32_t>(source.width) + divisor - 1) / divisor);
			target.height = static_cast<std::int32_t>((static_cast<std::uint32_t>(source.height) + divisor - 1) / divisor);
			target.firstLine = static_cast<std::uint32_t>(lines.size());
			std::vector<SPixel> row(static_cast<std::size_t>(source.width)), reduced(static_cast<std::size_t>(target.width));
			for (std::int32_t y = 0; y < target.height; ++y)
			{
				std::fill(row.begin(), row.end(), SPixel{});
				const auto sampleY = (std::min)(static_cast<std::uint32_t>(source.height - 1), static_cast<std::uint32_t>(y) * divisor + divisor / 2);
				std::uint32_t descriptor; std::memcpy(&descriptor, mLineControlData + (static_cast<std::size_t>(source.firstLine) + sampleY) * 4, 4);
				if (descriptor != UINT32_MAX)
				{
					auto x = descriptor >> 22; std::size_t position = descriptor & 0x3FFFFFu;
					for (;;)
					{
						if (position >= mPackedLineDataMemory->GetSize()) throw std::runtime_error("Truncated bob reduction row.");
						const auto token = mPackedLineData[position++]; if (token == 0) break;
						const auto count = token & 127u;
						if (x > static_cast<std::uint32_t>(source.width) || count > static_cast<std::uint32_t>(source.width) - x) throw std::runtime_error("Bob reduction run exceeds its frame.");
						if ((token & 128u) != 0) { x += count; continue; }
						const auto bytesPerPixel = source.type == 2 ? 0u : source.type == 4 ? 2u : 1u;
						if (position + count * bytesPerPixel > mPackedLineDataMemory->GetSize()) throw std::runtime_error("Truncated bob reduction pixels.");
						for (std::uint32_t pixel = 0; pixel < count; ++pixel)
						{
							auto& value = row[x++]; value.index = bytesPerPixel != 0 ? mPackedLineData[position++] : 0;
							value.alpha = source.type == 4 ? mPackedLineData[position++] : 255; value.visible = value.alpha != 0;
						}
					}
				}
				for (std::int32_t x = 0; x < target.width; ++x) reduced[x] = row[(std::min)(static_cast<std::uint32_t>(source.width - 1), static_cast<std::uint32_t>(x) * divisor + divisor / 2)];
				std::uint32_t first = 0, last = static_cast<std::uint32_t>(target.width);
				while (first < last && !reduced[first].visible) ++first;
				while (last > first && !reduced[last - 1].visible) --last;
				if (first == last) { lines.push_back(UINT32_MAX); continue; }
				if (first >= 1024 || packed.size() >= 0x400000) throw std::runtime_error("Reduced bob exceeds native line addressing.");
				lines.push_back((first << 22) | static_cast<std::uint32_t>(packed.size()));
				for (auto x = first; x < last;)
				{
					const bool visible = reduced[x].visible; auto end = x + 1;
					while (end < last && end - x < 127 && reduced[end].visible == visible) ++end;
					packed.push_back(static_cast<std::uint8_t>((end - x) | (visible ? 0u : 128u)));
					if (visible && source.type != 2) for (auto pixel = x; pixel < end; ++pixel) { packed.push_back(reduced[pixel].index); if (source.type == 4) packed.push_back(reduced[pixel].alpha); }
					x = end;
				}
				packed.push_back(0);
			}
		}
		if (packed.size() > 0x400000 || lines.size() > UINT32_MAX / 4u) throw std::runtime_error("Reduced bob exceeds native buffer limits.");
		// The original constructor resets the desktop alias; a session copy must preserve it.
		struct SDesktopAlias { CBitmap* value; ~SDesktopAlias() { CBobManager::mStaticVars = value; } } restore{ mStaticVars };
		return std::make_unique<CBobManager>(mSerialized.firstBobId, mSerialized.bobCount, frames.data(), static_cast<std::uint32_t>(frames.size() * sizeof(SFrame)), packed.data(), static_cast<std::uint32_t>(packed.size()), lines.data(), static_cast<std::uint32_t>(lines.size() * 4));
	}

	// Original function: FUN_004399cf
	void CBobManager::Storable_SaveData(void* file)
	{
		l_Generate_ResizePackedLineDataBuffer(mSerialized.packedLineDataSize);

		File_Write(file, &mSerialized, sizeof(mSerialized));

		if (mSerialized.bobCount != 0)
		{
			mBobDataMemory->Storable_Save(file);
			mPackedLineDataMemory->Storable_Save(file);
			mLineControlMemory->Storable_Save(file);
		}
	}

	// Original function: FUN_00439a12
	void CBobManager::l_Generate_ResizePackedLineDataBuffer(std::uint32_t size)
	{
		if (mPackedLineDataMemory != nullptr)
		{
			mPackedLineDataMemory->ChangeBufferSize(size, true, 0);
		}
		else
		{
			mPackedLineDataMemory = new (std::nothrow) CMemory(size, static_cast<std::uint8_t>(0));
		}

		l_System_SetRawPointer();
	}

	// Original function: FUN_00439a56
	void CBobManager::l_System_SetRawPointer()
	{
		mBobData = mBobDataMemory != nullptr ? mBobDataMemory->mData : nullptr;
		mPackedLineData = mPackedLineDataMemory != nullptr ? mPackedLineDataMemory->mData : nullptr;
		mLineControlData = mLineControlMemory != nullptr ? mLineControlMemory->mData : nullptr;
	}
}
