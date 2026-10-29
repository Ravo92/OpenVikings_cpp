#pragma once

#include <cstdint>
#include <cstring>

namespace NXBasics
{
	// Pixel/count arithmetic remains fixed-width; row/table addresses use native uintptr_t.
	static_assert(sizeof(std::uint8_t) == 1 && sizeof(std::uint16_t) == 2 && sizeof(std::uint32_t) == 4);
	static_assert(sizeof(std::uintptr_t) == sizeof(void*));

	// Original function: FUN_00410900
	inline void __cdecl XB_Tool_Byte_FillBlock(std::uint8_t* pixels, std::uint32_t width, std::int32_t height, std::uint32_t rowPixelCount, std::uint8_t color)
	{
		if (height <= 0)
		{

			return;
		}

		std::uintptr_t rowAddress = reinterpret_cast<std::uintptr_t>(pixels);

		for (std::int32_t rowsRemaining = height; rowsRemaining > 0; --rowsRemaining)
		{
			std::memset(reinterpret_cast<void*>(rowAddress), color, width);
			rowAddress += rowPixelCount;
		}
	}

	// Original function: FUN_0041092c
	inline void __cdecl XB_Tool_Byte_CopyBlock(const std::uint8_t* source, std::uint8_t* destination, std::uint32_t width, std::int32_t height, std::uint32_t sourceRowPixelCount, std::uint32_t destinationRowPixelCount)
	{
		if (height <= 0)
		{

			return;
		}

		std::uintptr_t sourceAddress = reinterpret_cast<std::uintptr_t>(source);
		std::uintptr_t destinationAddress = reinterpret_cast<std::uintptr_t>(destination);

		do
		{
			// Original function: FUN_004e6170; overlap-safe copying within this row.
			std::memmove(reinterpret_cast<void*>(destinationAddress), reinterpret_cast<const void*>(sourceAddress), width);

			sourceAddress += sourceRowPixelCount;
			destinationAddress += destinationRowPixelCount;
			--height;
		} while (height != 0);
	}

	// Original function: FUN_0041095a
	inline void __cdecl XB_Tool_Word_FillBlock(std::uint16_t* pixels, std::int32_t width, std::int32_t height, std::uint32_t rowPixelCount, std::uint16_t color)
	{
		if (height <= 0)
		{
			return;
		}

		const std::uint32_t rowByteCount = rowPixelCount * 2u;
		std::uintptr_t rowAddress = reinterpret_cast<std::uintptr_t>(pixels);

		if (color == 0)
		{
			const std::uint32_t fillByteCount = static_cast<std::uint32_t>(width) * 2u;

			for (std::int32_t rowsRemaining = height; rowsRemaining > 0; --rowsRemaining)
			{
				std::memset(reinterpret_cast<void*>(rowAddress), 0, fillByteCount);
				rowAddress += rowByteCount;
			}

			return;
		}

		const std::uint32_t pairedColor = (static_cast<std::uint32_t>(color) << 16) | color;

		for (std::int32_t rowsRemaining = height; rowsRemaining > 0; --rowsRemaining)
		{
			if (width > 0)
			{
				std::uintptr_t writeAddress = rowAddress;
				const std::uint32_t pairCount = static_cast<std::uint32_t>(width) >> 1;

				for (std::uint32_t pair = 0; pair < pairCount; ++pair)
				{
					std::memcpy(reinterpret_cast<void*>(writeAddress), &pairedColor, sizeof(pairedColor));
					writeAddress += 4u;
				}

				if ((static_cast<std::uint32_t>(width) & 1u) != 0)
				{
					std::memcpy(reinterpret_cast<void*>(writeAddress), &color, sizeof(color));
				}
			}

			rowAddress += rowByteCount;
		}
	}

	// Original function: FUN_004109c9
	inline void __cdecl XB_Tool_Word_CopyBlock(const std::uint16_t* source, std::uint16_t* destination, std::uint32_t width, std::int32_t height, std::uint32_t sourceRowPixelCount, std::uint32_t destinationRowPixelCount)
	{
		if (height <= 0)
		{

			return;
		}

		// Preserve 32-bit wraparound when converting pixel counts to bytes.
		const std::uint32_t rowByteCount = width * 2u;
		const std::uint32_t sourceRowByteCount = sourceRowPixelCount * 2u;
		const std::uint32_t destinationRowByteCount = destinationRowPixelCount * 2u;

		std::uintptr_t sourceAddress = reinterpret_cast<std::uintptr_t>(source);
		std::uintptr_t destinationAddress = reinterpret_cast<std::uintptr_t>(destination);

		do
		{
			// Original function: FUN_004e6170; overlap-safe copying within this row.
			std::memmove(reinterpret_cast<void*>(destinationAddress), reinterpret_cast<const void*>(sourceAddress), rowByteCount);

			sourceAddress += sourceRowByteCount;
			destinationAddress += destinationRowByteCount;
			--height;
		} while (height != 0);
	}

	// Original function: FUN_00410a0a
	inline void __cdecl XB_Tool_Long_FillBlock(std::uint32_t* pixels, std::int32_t width, std::int32_t height, std::uint32_t rowPixelCount, std::uint32_t color)
	{
		if (height <= 0)
		{
			return;
		}

		const std::uint32_t rowByteCount = rowPixelCount * 4u;
		std::uintptr_t rowAddress = reinterpret_cast<std::uintptr_t>(pixels);

		if (color == 0)
		{
			const std::uint32_t fillByteCount = static_cast<std::uint32_t>(width) * 4u;

			for (std::int32_t rowsRemaining = height; rowsRemaining > 0; --rowsRemaining)
			{
				std::memset(reinterpret_cast<void*>(rowAddress), 0, fillByteCount);
				rowAddress += rowByteCount;
			}

			return;
		}

		for (std::int32_t rowsRemaining = height; rowsRemaining > 0; --rowsRemaining)
		{
			if (width > 0)
			{
				std::uintptr_t writeAddress = rowAddress;

				for (std::int32_t column = 0; column < width; ++column)
				{
					std::memcpy(reinterpret_cast<void*>(writeAddress), &color, sizeof(color));
					writeAddress += 4u;
				}
			}

			rowAddress += rowByteCount;
		}
	}

	// Original function: FUN_00410a6a
	inline void __cdecl XB_Tool_Long_CopyBlock(const std::uint32_t* source, std::uint32_t* destination, std::uint32_t width, std::int32_t height, std::uint32_t sourceRowPixelCount, std::uint32_t destinationRowPixelCount)
	{
		if (height <= 0)
		{

			return;
		}

		const std::uint32_t rowByteCount = width * 4u;
		const std::uint32_t sourceRowByteCount = sourceRowPixelCount * 4u;
		const std::uint32_t destinationRowByteCount = destinationRowPixelCount * 4u;

		std::uintptr_t sourceAddress = reinterpret_cast<std::uintptr_t>(source);
		std::uintptr_t destinationAddress = reinterpret_cast<std::uintptr_t>(destination);

		do
		{
			// Original function: FUN_004e6170; overlap-safe copying within this row.
			std::memmove(reinterpret_cast<void*>(destinationAddress), reinterpret_cast<const void*>(sourceAddress), rowByteCount);

			sourceAddress += sourceRowByteCount;
			destinationAddress += destinationRowByteCount;
			--height;
		} while (height != 0);
	}

	// Original function: FUN_00410ab1
	inline void __cdecl XB_Tool_ByteToWord_CopyBlock(const std::uint8_t* source, std::uint16_t* destination, std::int32_t width, std::int32_t height, std::uint32_t sourceRowPixelCount, std::uint32_t destinationRowPixelCount, const std::uint16_t* colorTable)
	{
		if (height <= 0)
		{

			return;
		}

		std::uintptr_t sourceAddress = reinterpret_cast<std::uintptr_t>(source);
		std::uintptr_t destinationAddress = reinterpret_cast<std::uintptr_t>(destination);

		// Preserve the original row-tail arithmetic, including negative widths.
		const std::uint32_t sourceRowTail = sourceRowPixelCount - static_cast<std::uint32_t>(width);
		const std::uint32_t destinationRowTail = (destinationRowPixelCount - static_cast<std::uint32_t>(width)) * 2u;

		do
		{
			std::int32_t remaining = width;

			while (remaining > 0)
			{
				const std::uint8_t index = *reinterpret_cast<const std::uint8_t*>(sourceAddress);
				const std::uintptr_t tableAddress = reinterpret_cast<std::uintptr_t>(colorTable) + static_cast<std::uint32_t>(index) * 2u;

				std::uint16_t color;
				std::memcpy(&color, reinterpret_cast<const void*>(tableAddress), sizeof(color));
				std::memcpy(reinterpret_cast<void*>(destinationAddress), &color, sizeof(color));

				destinationAddress += 2u;
				++sourceAddress;
				--remaining;
			}

			sourceAddress += sourceRowTail;
			destinationAddress += destinationRowTail;
			--height;
		} while (height != 0);
	}

	// Original function: FUN_00410afc
	inline void __cdecl XB_Tool_ByteToLong_CopyBlock(const std::uint8_t* source, std::uint32_t* destination, std::int32_t width, std::int32_t height, std::uint32_t sourceRowPixelCount, std::uint32_t destinationRowPixelCount, const std::uint32_t* colorTable)
	{
		if (height <= 0)
		{

			return;
		}

		std::uintptr_t sourceAddress = reinterpret_cast<std::uintptr_t>(source);
		std::uintptr_t destinationAddress = reinterpret_cast<std::uintptr_t>(destination);

		const std::uint32_t sourceRowTail = sourceRowPixelCount - static_cast<std::uint32_t>(width);
		const std::uint32_t destinationRowTail = (destinationRowPixelCount - static_cast<std::uint32_t>(width)) * 4u;

		do
		{
			std::int32_t remaining = width;

			while (remaining > 0)
			{
				const std::uint8_t index = *reinterpret_cast<const std::uint8_t*>(sourceAddress);
				const std::uintptr_t tableAddress = reinterpret_cast<std::uintptr_t>(colorTable) + static_cast<std::uint32_t>(index) * 4u;

				std::uint32_t color;
				std::memcpy(&color, reinterpret_cast<const void*>(tableAddress), sizeof(color));
				std::memcpy(reinterpret_cast<void*>(destinationAddress), &color, sizeof(color));

				destinationAddress += 4u;
				++sourceAddress;
				--remaining;
			}

			sourceAddress += sourceRowTail;
			destinationAddress += destinationRowTail;
			--height;
		} while (height != 0);
	}
}