#include "C2DataIds.h"
#include "SwitchIdentifiers.h"
#include "XNumericConstants.h"
#include "XPack.h"

#include <cstddef>
#include <cstring>

namespace NXBasics
{
	namespace
	{
		std::uint32_t ReadPackedLong(const void* block, std::uint32_t offset)
		{
			std::uint32_t value;
			std::memcpy(&value, static_cast<const std::byte*>(block) + offset, sizeof(value));

			return value;
		}

		// Remaster: validate every packet before changing the destination.
		template <class T>
		bool UnpackSimpleRLE(const void* packedBlock, std::uint32_t packedBytes, void* destination, std::uint32_t destinationBytes, std::uint32_t method)
		{
			if (packedBlock == nullptr || packedBytes < 16u)
				return false;

			const auto outputBytes = ReadPackedLong(packedBlock, 8);

			if (ReadPackedLong(packedBlock, 0) != NC2DataIds::PackingHeader::Signature || ReadPackedLong(packedBlock, 4) != method || ReadPackedLong(packedBlock, 12) != packedBytes || outputBytes > destinationBytes || outputBytes % sizeof(T) != 0 || (outputBytes != 0 && destination == nullptr))
				return false;

			const auto* block = static_cast<const std::byte*>(packedBlock);

			for (unsigned pass = 0; pass < 2; ++pass)
			{
				std::uint32_t input = 16, output = 0;

				while (input < packedBytes)
				{
					const auto token = static_cast<std::uint8_t>(block[input++]);
					const auto count = static_cast<std::uint32_t>(token & 127u);
					const auto bytes = count * sizeof(T);
					const auto inputBytes = (token & 128u) != 0 ? sizeof(T) : bytes;

					if (bytes > outputBytes - output || inputBytes > packedBytes - input)
						return false;

					if (pass != 0 && bytes != 0)
					{
						auto* target = static_cast<std::byte*>(destination) + output;

						if ((token & 128u) != 0)
						{
							for (std::uint32_t i = 0; i < count; ++i)
							{
								std::memcpy(target + i * sizeof(T), block + input, sizeof(T));
							}
						}
						else
						{
							std::memcpy(target, block + input, bytes);
						}
					}

					input += static_cast<std::uint32_t>(inputBytes);
					output += static_cast<std::uint32_t>(bytes);
				}

				if (output != outputBytes)
					return false;
			}

			return true;
		}
	}

	// Original function: FUN_00458890; callers of this native interface own a complete block.
	bool XB_Pack_Simple8BitRLEUnPack(const void* packedBlock, void* destination, std::uint32_t destinationBytes)
	{

		return packedBlock != nullptr && UnpackSimpleRLE<std::uint8_t>(packedBlock, ReadPackedLong(packedBlock, 12), destination, destinationBytes, NC2DataIds::PackingType::Rle8Unsigned);
	}

	// Original function: FUN_00458c53.
	bool XB_Pack_Simple16BitRLEUnPack(const void* packedBlock, void* destination, std::uint32_t destinationBytes)
	{

		return packedBlock != nullptr && UnpackSimpleRLE<std::uint16_t>(packedBlock, ReadPackedLong(packedBlock, 12), destination, destinationBytes, NC2DataIds::PackingType::Rle16Unsigned);
	}

	// Remaster overload: serialized input must carry its actual allocation size.
	bool XB_Pack_UnPack(const void* packedBlock, std::uint32_t packedBytes, void* destination, std::uint32_t destinationBytes)
	{
		if (packedBlock == nullptr || packedBytes < 16u || ReadPackedLong(packedBlock, 0) != NC2DataIds::PackingHeader::Signature || ReadPackedLong(packedBlock, 12) != packedBytes)
			return false;

		const auto outputBytes = ReadPackedLong(packedBlock, 8);

		if (outputBytes > destinationBytes || (outputBytes != 0 && destination == nullptr))
			return false;

		switch (ReadPackedLong(packedBlock, 4))
		{
			case SwitchIds::PackingType::Rle8:
				return UnpackSimpleRLE<std::uint8_t>(packedBlock, packedBytes, destination, destinationBytes, NC2DataIds::PackingType::Rle8Unsigned);
			case SwitchIds::PackingType::Rle16:
				return UnpackSimpleRLE<std::uint16_t>(packedBlock, packedBytes, destination, destinationBytes, NC2DataIds::PackingType::Rle16Unsigned);
			case SwitchIds::PackingType::None:
				if (outputBytes != packedBytes - 16u)
					return false;

				if (outputBytes != 0)
					std::memcpy(destination, static_cast<const std::byte*>(packedBlock) + 16, outputBytes);

				return true;
			default:

				return false;
		}
	}

	// Original function: FUN_00458d60; complete-block contract retained for native callers.
	bool XB_Pack_UnPack(const void* packedBlock, void* destination, std::uint32_t destinationBytes)
	{

		return packedBlock != nullptr && XB_Pack_UnPack(packedBlock, ReadPackedLong(packedBlock, 12), destination, destinationBytes);
	}

	namespace
	{
		// Shared packet state, translated from the separate Windows byte/word packers.
		template <class T>
		std::uint32_t PackSimpleRLE(const void* source, std::uint32_t sourceBytes, void* destination, std::uint32_t destinationBytes, std::uint32_t packingType)
		{
			if (destination == nullptr || destinationBytes < 16u || (source == nullptr && sourceBytes != 0) || sourceBytes % sizeof(T) != 0)
				return 0;

			auto* output = static_cast<std::byte*>(destination);
			const auto* input = static_cast<const std::byte*>(source);
			const std::uint32_t header[3] = { NC2DataIds::PackingHeader::SignatureSigned, packingType, sourceBytes };
			std::memcpy(output, header, sizeof(header));
			std::uint32_t used = 16;
			T literals[127];
			T last{ };
			std::uint32_t literalCount = 0, runCount = 0;
			const auto writeByte = [&](std::uint8_t value)
			{
				if (used >= destinationBytes)
					return false;

				output[used++] = static_cast<std::byte>(value);

				return true;
			};
			const auto writeValue = [&](T value)
			{
				if constexpr (sizeof(T) == 1)
				{
					if (used >= destinationBytes)
						return false;
				}
				else if (used >= destinationBytes - 1u)
				{
					return false;
				}

				std::memcpy(output + used, &value, sizeof(T));
				used += sizeof(T);

				return true;
			};
			const auto flushLiterals = [&]()
			{
				if (!writeByte(static_cast<std::uint8_t>(literalCount)))
					return false;

				for (std::uint32_t index = 0; index < literalCount; ++index)
				{
					if (!writeValue(literals[index]))
						return false;
				}

				literalCount = 0;

				return true;
			};
			const auto flushRun = [&]()
			{
				if (!writeByte(static_cast<std::uint8_t>(runCount | NXBasics::BitMask::Bit7Signed)) || !writeValue(last))
					return false;

				runCount = 0;

				return true;
			};
			const auto appendLiteral = [&](T value)
			{
				literals[literalCount++] = value;

				return literalCount != 127 || flushLiterals();
			};

			for (std::uint32_t offset = 0; offset != sourceBytes; offset += sizeof(T))
			{
				T value;
				std::memcpy(&value, input + offset, sizeof(T));

				if (literalCount == 0)
				{
					if (runCount == 0)
					{
						last = value;
						runCount = 1;
					}
					else if (value == last)
					{
						if (++runCount == 127 && !flushRun())
							return 0;
					}
					else if (runCount > 2)
					{
						if (!flushRun())
							return 0;

						last = value;
						runCount = 1;
					}
					else
					{
						const std::uint32_t count = runCount;

						for (std::uint32_t index = 0; index < count; ++index)
						{
							if (!appendLiteral(last))
								return 0;
						}

						runCount = 0;

						if (!appendLiteral(value))
							return 0;
					}
				}
				else if (runCount != 0)
				{
					if (value == last)
					{
						if (++runCount == 127 && (!flushLiterals() || !flushRun()))
							return 0;
					}
					else
					{
						if (!flushLiterals() || !flushRun())
							return 0;

						last = value;
						runCount = 1;
					}
				}
				else if (literals[literalCount - 1] == value)
				{
					--literalCount;
					last = value;
					runCount = 2;
				}
				else if (!appendLiteral(value))
				{
					return 0;
				}
			}

			if (literalCount != 0 && !flushLiterals())
				return 0;

			if (runCount != 0 && !flushRun())
				return 0;

			std::memcpy(output + 12, &used, 4);

			return used;
		}
	}

	// Original function: FUN_004585c6.
	std::uint32_t XB_Pack_Simple8BitRLEPack(const void* source, std::uint32_t sourceBytes, void* destination, std::uint32_t destinationBytes)
	{

		return PackSimpleRLE<std::uint8_t>(source, sourceBytes, destination, destinationBytes, NC2DataIds::PackingType::Rle8);
	}

	// Original function: FUN_0045892a; odd source lengths are rejected before writing the header.
	std::uint32_t XB_Pack_Simple16BitRLEPack(const void* source, std::uint32_t sourceBytes, void* destination, std::uint32_t destinationBytes)
	{
		if ((sourceBytes & 1u) != 0)
			return 0;

		return PackSimpleRLE<std::uint16_t>(source, sourceBytes, destination, destinationBytes, NC2DataIds::PackingType::Rle16);
	}

	// Original function: FUN_00458ce2.
	std::uint32_t XB_Pack_Pack(std::uint32_t packingType, const void* source, std::uint32_t sourceBytes, void* destination, std::uint32_t destinationBytes)
	{
		switch (packingType)
		{
			case SwitchIds::PackingType::Rle8Signed:
				return XB_Pack_Simple8BitRLEPack(source, sourceBytes, destination, destinationBytes);
			case SwitchIds::PackingType::Rle16Signed:
				return XB_Pack_Simple16BitRLEPack(source, sourceBytes, destination, destinationBytes);
			case SwitchIds::PackingType::NoneSigned:
			{
				const std::uint32_t header[4] = { NC2DataIds::PackingHeader::SignatureSigned, packingType, sourceBytes, sourceBytes + 16u };

				if (destination == nullptr || destinationBytes < 16u || sourceBytes > destinationBytes - 16u || (source == nullptr && sourceBytes != 0))
					return 0;

				std::memcpy(destination, header, sizeof(header));

				if (sourceBytes != 0)
					std::memcpy(static_cast<std::byte*>(destination) + 16, source, sourceBytes);

				return sourceBytes + 16u;
			}
			default:

				return 0;
		}
	}

}