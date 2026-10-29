#include "C2DataIds.h"
#include "FileCompressor.h"
#include "XCrtFileAccess.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace NNetwork
{
	namespace
	{
		constexpr std::uint32_t MaximumSize = 64 * 1024 * 1024;

		struct SCompressedFileHeader
		{
			std::uint32_t signature, headerSize, version, originalSize, compressedSize;
			std::uint8_t method, marker, reserved[6];
		};

		static_assert(sizeof(SCompressedFileHeader) == 28);
		static_assert(offsetof(SCompressedFileHeader, method) == 0x14);

		struct Stream
		{
			NXBasics::RuntimeFormatStream* value;

			Stream(const char* path, const char* mode) : value(NXBasics::Runtime_OpenStream(path, mode))
			{
			}

			~Stream()
			{
				Close();
			}

			int Close()
			{
				auto* current = value;
				value = nullptr;

				return current != nullptr ? NXBasics::Runtime_CloseStream(current) : 0;
			}
		};

		struct Temporary
		{
			std::string path;
			bool owned = false;

			explicit Temporary(const std::string& destination)
			{
				GUID id{ };

				if (FAILED(CoCreateGuid(&id)))
				{

					return;
				}

				path = destination + ".ov-";
				constexpr char hex[] = "0123456789abcdef";
				const auto* bytes = reinterpret_cast<const unsigned char*>(&id);

				for (unsigned i = 0; i < sizeof(id); ++i)
				{
					path += hex[bytes[i] >> 4];
					path += hex[bytes[i] & 15];
				}

				path += ".tmp";
			}

			~Temporary()
			{
				if (owned)
					DeleteFileA(path.c_str());
			}
		};

		bool Length(Stream& stream, std::uint32_t& size)
		{
			if (stream.value == nullptr || _fseeki64(stream.value->nativeFile, 0, SEEK_END) != 0)
				return false;

			const auto length = _ftelli64(stream.value->nativeFile);

			if (length < 0 || length > MaximumSize + sizeof(SCompressedFileHeader) || _fseeki64(stream.value->nativeFile, 0, SEEK_SET) != 0)
				return false;

			size = static_cast<std::uint32_t>(length);

			return true;
		}

		bool Write(Stream& output, const void* data, std::size_t size)
		{

			return size == 0 || NXBasics::Runtime_WriteStream(data, 1, static_cast<std::uint32_t>(size), output.value) == size;
		}

		bool Commit(Stream& output, Temporary& temporary, const std::string& destination)
		{
			const bool flushed = NXBasics::Runtime_FlushStream(output.value) == 0;
			const bool closed = output.Close() == 0;

			if (!flushed || !closed || !MoveFileExA(temporary.path.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
				return false;

			temporary.owned = false;

			return true;
		}

		std::vector<std::uint8_t> Encode(const std::vector<std::uint8_t>& input, std::uint8_t marker)
		{
			std::vector<std::uint8_t> output;

			for (std::size_t position = 0; position < input.size();)
			{
				const auto value = input[position];
				std::size_t count = 1;

				while (count < 255 && position + count < input.size() && input[position + count] == value)
					++count;

				if (value == marker || count >= 4)
				{
					output.push_back(marker);
					output.push_back(value);
					output.push_back(static_cast<std::uint8_t>(count));
				}
				else
				{
					output.insert(output.end(), count, value);
				}

				position += count;

				if (output.size() >= input.size())
					return { };
			}

			return output;
		}
	}

	CFileCompressor::CFileCompressor() : mInput(nullptr), mOutput(nullptr), mTemporary(nullptr)
	{
	}

	// Modern replacement of 004E0B4E; the in-process class and .c2p header layouts remain stable.
	std::int32_t CFileCompressor::CompressFile(const char* fileName)
	{
		if (fileName == nullptr || *fileName == 0 || std::strlen(fileName) > 240)
			return 1;

		try
		{
			Stream input(fileName, "rb");
			std::uint32_t size;

			if (!Length(input, size) || size > MaximumSize)
				return 1;

			std::vector<std::uint8_t> bytes(size);

			if ((size != 0 && NXBasics::Runtime_ReadStream(bytes.data(), 1, size, input.value) != size) || std::ferror(input.value->nativeFile))
				return 1;

			std::array<std::uint32_t, 256> frequencies{ };

			for (const auto value : bytes)
			{
				++frequencies[value];
			}

			const auto marker = static_cast<std::uint8_t>(std::min_element(frequencies.begin(), frequencies.end()) - frequencies.begin());
			const auto encoded = Encode(bytes, marker);
			const auto& payload = encoded.empty() ? bytes : encoded;
			SCompressedFileHeader header{ };
			header.signature = NC2DataIds::CompressedFileHeader::Signature;
			header.headerSize = sizeof(header);
			header.version = 1;
			header.originalSize = size;
			header.compressedSize = static_cast<std::uint32_t>(payload.size() + sizeof(header));
			header.method = encoded.empty() ? 0 : 1;
			header.marker = marker;
			const std::string destination = std::string(fileName) + ".c2p";
			Temporary temporary(destination);

			if (temporary.path.empty())
				return 1;

			Stream output(temporary.path.c_str(), "wbx");

			if (output.value == nullptr)
				return 1;

			temporary.owned = true;

			if (!Write(output, &header, sizeof(header)) || !Write(output, payload.data(), payload.size()) || !Commit(output, temporary, destination))
				return 2;

			return 0;
		}
		catch (...)
		{

			return 1;
		}
	}

	// Modern replacement of 004E0D5C/004E0F99; reject malformed input before replacing a map/save.
	std::int32_t CFileCompressor::DecompressFile(const char* fileName)
	{
		if (fileName == nullptr)
			return 1;

		const auto length = std::strlen(fileName);

		if (length < 5 || length > 244 || _stricmp(fileName + length - 4, ".c2p") != 0)
			return 3;

		try
		{
			Stream input(fileName, "rb");
			std::uint32_t size;

			if (!Length(input, size))
				return 1;

			SCompressedFileHeader header{ };

			if (NXBasics::Runtime_ReadStream(&header, sizeof(header), 1, input.value) != 1 || header.signature != NC2DataIds::CompressedFileHeader::Signature || header.headerSize != sizeof(header) || header.version != 1 || header.compressedSize != size || header.originalSize > MaximumSize || header.method > 1)
				return 3;

			if (header.method == 0 && header.originalSize != size - sizeof(header))
				return 3;

			const std::string destination(fileName, length - 4);
			Temporary temporary(destination);

			if (temporary.path.empty())
				return 1;

			Stream output(temporary.path.c_str(), "wbx");

			if (output.value == nullptr)
				return 1;

			temporary.owned = true;
			std::uint32_t produced = 0, used = 0;
			std::array<std::uint8_t, 65536> buffer{ };

			for (;;)
			{
				const int value = NXBasics::Runtime_ReadStreamCharacter(input.value);

				if (value == EOF)
					break;

				int byte = value, count = 1;

				if (header.method == 1 && value == header.marker)
				{
					byte = NXBasics::Runtime_ReadStreamCharacter(input.value);
					count = NXBasics::Runtime_ReadStreamCharacter(input.value);

					if (byte == EOF || count <= 0)
						return 4;
				}

				if (static_cast<std::uint32_t>(count) > header.originalSize - produced)
					return 4;

				for (int i = 0; i < count; ++i)
				{
					buffer[used++] = static_cast<std::uint8_t>(byte);

					if (used == buffer.size())
					{
						if (!Write(output, buffer.data(), used))
							return 4;

						used = 0;
					}
				}

				produced += count;
			}

			if (std::ferror(input.value->nativeFile) || produced != header.originalSize || !Write(output, buffer.data(), used))
				return 4;

			if (input.Close() != 0 || !Commit(output, temporary, destination))
				return 4;

			DeleteFileA(fileName);
			return 0;
		}
		catch (...)
		{

			return 1;
		}
	}
}