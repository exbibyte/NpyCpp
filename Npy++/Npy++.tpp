#pragma once

#include <complex>
#include <numeric>

#include <StringUtilities.h>
#include <zlib.h>

#ifdef WIN32
	#define FOPEN(FILE_POINTER, FILE_NAME, MODE) fopens(&FILE_POINTER, FILE_NAME, MODE)
#else
	#define FOPEN(FILE_POINTER, FILE_NAME, MODE) FILE_POINTER = fopen(FILE_NAME, MODE)
#endif

namespace npypp
{
	namespace detail
	{
#pragma region Type Traits

#define MAKE_TYPE_TRAITS(TYPE, CHAR)                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           \
	template<>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 \
	struct Traits<TYPE>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        \
	{                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          \
		static constexpr char id { CHAR };                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     \
	}

		MAKE_TYPE_TRAITS(float, 'f');
		MAKE_TYPE_TRAITS(double, 'f');
		MAKE_TYPE_TRAITS(long double, 'f');

		MAKE_TYPE_TRAITS(short, 'i');
		MAKE_TYPE_TRAITS(char, 'i');
		MAKE_TYPE_TRAITS(int, 'i');
		MAKE_TYPE_TRAITS(long, 'i');
		MAKE_TYPE_TRAITS(long long, 'i');

		MAKE_TYPE_TRAITS(unsigned short, 'u');
		MAKE_TYPE_TRAITS(unsigned char, 'u');
		MAKE_TYPE_TRAITS(unsigned int, 'u');
		MAKE_TYPE_TRAITS(unsigned long, 'u');
		MAKE_TYPE_TRAITS(unsigned long long, 'u');

		MAKE_TYPE_TRAITS(bool, 'b');

		template<typename T>
		struct Traits<std::complex<T>>
		{
			static constexpr char id { 'c' };
		};

#undef MAKE_TYPE_TRAITS

#pragma endregion

		template<typename T>
		void appendBytes(std::string& out, const T in)
		{
			// NOLINTNEXTLINE
			const char* inPtr = reinterpret_cast<const char*>(&in);
			for (size_t byteIndex = 0; byteIndex < sizeof(T); ++byteIndex)
				out += *(inPtr + byteIndex);
		}

#pragma region Npy Header

		template<typename T>
		[[maybe_unused]] std::string GetNpyHeaderDescription()
		{
			std::string ret;
			ret += "'descr': '";
			ret += SysEndianness();
			ret += Traits<T>::id;
			ret += std::to_string(sizeof(T));
			ret += "'";

			return ret;
		}

		template<typename T>
		[[maybe_unused]] std::string GetNpyHeader(const std::vector<size_t>& shape)
		{
			std::string properties = "{";

			properties += GetNpyHeaderDescription<T>();
			properties += ", ";

			properties += GetNpyHeaderFortranOrder();
			properties += ", ";

			properties += GetNpyHeaderShape(shape);
			properties += ", }";

			SetNpyHeaderPadding(properties);

			std::string header = GetMagic();
			appendBytes<uint16_t>(header, static_cast<uint16_t>(properties.size()));

			header.insert(header.end(), properties.begin(), properties.end());

			return header;
		}

		template<typename mm::CacheHint ch, typename mm::MapMode mpm>
		[[maybe_unused]] void ParseNpyHeader(mm::MemoryMappedFile<ch, mpm>& mmf, size_t& wordSize, std::vector<size_t>& shape, bool& fortranOrder, char& endianness)
		{
			constexpr size_t expectedCharToRead { 11 };
			mmf.Advance(expectedCharToRead);

			std::string header = mmf.ReadLine();
			assert(header[header.size() - 1] == '\n');

			fortranOrder = ParseFortranOrder(header);
			ParseShape(shape, header);
			ParseDescription(header, wordSize, endianness);
		}

#pragma endregion

		template<typename T>
		[[maybe_unused]] static void SwapEndianness(T* buffer, size_t size)
		{
			constexpr size_t sizeOfT = sizeof(T);
			for (size_t i = 0; i < size; i++)
			{
				// NOLINTNEXTLINE
				const auto* src = reinterpret_cast<uint8_t*>(buffer + i);
				std::array<uint8_t, sizeOfT> dst;
				for (size_t j = 0; j < sizeOfT; j++)
					dst[j] = src[sizeOfT - j - 1];

				// NOLINTNEXTLINE
				buffer[i] = *reinterpret_cast<T*>(dst.data());
			}
		}

		template<typename T>
		[[maybe_unused]] static void SwapEndianness(std::vector<T>& v)
		{
			SwapEndianness(v.data(), v.size());
		}

		template<typename T>
		[[maybe_unused]] MultiDimensionalArray<T> LoadFull(FILE* fp)
		{
			assert(fp != nullptr);

			std::vector<T> data;

			std::vector<size_t> shape;
			size_t wordSize = 0;
			bool fortranOrder = false;
			char endianness = 0;
			detail::ParseNpyHeader(fp, wordSize, shape, fortranOrder, endianness);

			const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1u, std::multiplies<>());
			data.resize(nElements);

			[[maybe_unused]] const auto charactersRead = fread(data.data(), sizeof(T), nElements, fp);
			assert(charactersRead == nElements);

			if (endianness != '|' && (endianness != SysEndianness()))
				SwapEndianness(data);

			return MultiDimensionalArray<T>(data, shape);
		}

		template<typename T, typename mm::CacheHint ch, typename mm::MapMode mpm>
		[[maybe_unused]] MultiDimensionalArray<T> LoadFull(mm::MemoryMappedFile<ch, mpm>& mmf)
		{
			std::vector<T> data;

			std::vector<size_t> shape;
			size_t wordSize = 0;
			bool fortranOrder = false;
			char endianness = 0;
			detail::ParseNpyHeader(mmf, wordSize, shape, fortranOrder, endianness);
			mmf.Advance(1);	   // getting rid of the newline char

			const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1u, std::multiplies<>());
			data.resize(nElements);

			mmf.CopyTo(data);

			return MultiDimensionalArray<T>(data, shape);
		}

		/**
		 * WARNING: The data pointer must be nullptr when passed in, as this will not delete it
		 */
		template<typename T, typename mm::CacheHint ch, typename mm::MapMode mpm>
		[[maybe_unused]] void LoadNoCopy(const T*& data, mm::MemoryMappedFile<ch, mpm>& mmf)
		{
			assert(mpm != mm::MapMode::WriteOnly);
			std::vector<size_t> shape;
			size_t wordSize = 0;
			bool fortranOrder = false;
			char endianness = 0;
			detail::ParseNpyHeader(mmf, wordSize, shape, fortranOrder, endianness);
			mmf.Advance(1);	   // getting rid of the newline char
			mmf.Set(data);	  // no copy, just a reinterpret cast
		}

#pragma region Npz Utilities

		template<typename T>
		[[maybe_unused]] uint32_t GetCrcNpyFile(const std::string& npyHeader, const std::vector<T>& data)
		{
			// NOLINTNEXTLINE
			const auto crc = crc32(0L, reinterpret_cast<const uint8_t*>(&npyHeader[0]), static_cast<unsigned int>(npyHeader.size()));

			// NOLINTNEXTLINE
			return static_cast<uint32_t>(crc32(crc, reinterpret_cast<const uint8_t*>(&data[0]), static_cast<unsigned>(data.size() * sizeof(T))));
		}

		template<typename T>
		MultiDimensionalArray<T> LoadCompressedFull(FILE* fp, uint32_t compressedBytes, uint32_t uncompressedBytes)
		{
			std::vector<unsigned char> bufferCompressed(compressedBytes);
			std::vector<unsigned char> bufferUncompressed(uncompressedBytes);
			size_t UNUSED elementsRead = fread(bufferCompressed.data(), 1, compressedBytes, fp);
			assert(elementsRead == compressedBytes);

			z_stream stream;
			stream.zalloc = nullptr;
			stream.zfree = nullptr;
			stream.opaque = nullptr;
			stream.avail_in = 0;
			stream.next_in = nullptr;
			inflateInit2(&stream, -MAX_WBITS);

			stream.avail_in = compressedBytes;
			stream.next_in = bufferCompressed.data();
			stream.avail_out = uncompressedBytes;
			stream.next_out = bufferUncompressed.data();

			inflate(&stream, Z_FINISH);
			inflateEnd(&stream);

			std::vector<size_t> shape;
			size_t wordSize = 0;
			bool fortranOrder = false;
			char endianness = 0;
			detail::ParseNpyHeader(bufferUncompressed, wordSize, shape, fortranOrder, endianness);

			MultiDimensionalArray<T> array;
			array.shape = shape;
			size_t nElements = std::accumulate(shape.begin(), shape.end(), 1u, std::multiplies<>());
			array.data.resize(nElements);
			size_t nElementsInBytes = nElements * sizeof(T);
			size_t offset = uncompressedBytes - nElementsInBytes;
			memcpy(array.data.data(), bufferUncompressed.data() + offset, nElementsInBytes);

			if (endianness != '|' && (endianness != SysEndianness()))
				SwapEndianness(array.data);

			return array;
		}

#pragma endregion
	}	 // namespace detail

#pragma region Load / Save Npy

	template<typename T>
	void Save(const std::string& fileName, const std::vector<T>& data, const std::vector<size_t>& shape, const std::string& mode)
	{
		FILE* fp = nullptr;
		std::vector<size_t> actualShape;	// if appending, the shape of existing + new data

		if (mode == "a")
			FOPEN(fp, fileName.c_str(), "r+b");

		if (fp)
		{
			// file exists. we need to append to it. read the header, modify the array size
			size_t wordSize = 0;
			bool fortranOrder = 0;
			char endianness = 0;
			detail::ParseNpyHeader(fp, wordSize, actualShape, fortranOrder, endianness);
			assert(!fortranOrder);
			assert(wordSize == sizeof(T));
			assert(actualShape.size() == shape.size());

			for (size_t i = 0; i < shape.size(); i++)
				assert(shape[i] == actualShape[i]);
			actualShape[0] += shape[0];
		}
		else
		{
			// file doesn't exist, needs to be created
			FOPEN(fp, fileName.c_str(), "wb");
			actualShape = shape;
		}
		assert(fp != nullptr);

		const std::string header = detail::GetNpyHeader<T>(actualShape);
		const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1u, std::multiplies<size_t>());

		fseek(fp, 0, SEEK_SET);
		fwrite(header.data(), sizeof(char), header.size(), fp);
		fseek(fp, 0, SEEK_END);

		const size_t UNUSED charactersWritten = fwrite(data.data(), sizeof(T), nElements, fp);
		assert(charactersWritten == nElements);
		std::fclose(fp);
	}

	template<typename T, typename mm::CacheHint ch, typename mm::MapMode mpm>
	void Save(mm::MemoryMappedFile<ch, mpm>& mmf, const std::vector<T>& data, const std::vector<size_t>& shape)
	{
		assert(mmf.IsValid());

		const std::string header = detail::GetNpyHeader<T>(shape);
		// NOLINTNEXTLINE
		const auto* headerBuffer = reinterpret_cast<unsigned const char*>(header.c_str());
		mmf.ReadFrom(headerBuffer, header.size());

		const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1u, std::multiplies<>());
		// NOLINTNEXTLINE
		const auto* dataBuffer = reinterpret_cast<unsigned const char*>(data.data());
		mmf.ReadFrom(dataBuffer, sizeof(T) * nElements);
	}

	/**
	 * Load the full info (data and shape) from the file
	 */
	template<typename T>
	MultiDimensionalArray<T> LoadFull(const std::string& fileName, const bool useMemoryMap)
	{
		if (!useMemoryMap)
		{
			FILE* fp = nullptr;
			FOPEN(fp, fileName.c_str(), "rb");
			const auto ret = detail::LoadFull<T>(fp);
			fclose(fp);

			return ret;
		}

		mm::MemoryMappedFile<mm::CacheHint::SequentialScan, mm::MapMode::ReadOnly> mmf(fileName);
		assert(mmf.IsValid());
		return detail::LoadFull<T, mm::CacheHint::SequentialScan>(mmf);
	}

	/**
	 * Load the full info (data and shape) from the file using an externally set memory mapped file without copying memory
	 */
	template<typename T, typename mm::CacheHint ch, typename mm::MapMode mpm>
	MultiDimensionalArray<T> LoadFull(mm::MemoryMappedFile<ch, mpm>& mmf)
	{
		assert(mpm != mm::MapMode::WriteOnly);
		auto ret = detail::LoadFull<T, ch, mpm>(mmf);
		return ret;
	}

#pragma endregion

#pragma region Load / Save Npz

	template<typename T>
	void SaveCompressed(const std::string& zipFileName, std::string vectorName, const std::vector<T>& data, const std::vector<size_t>& shape, const std::string& mode)
	{
		// first, append a .npy to the vector name, since the .npz file stores multiple npy files
		vectorName += ".npy";

		// now, on with the show
		FILE* fp = nullptr;
		uint16_t nRecords = 0;
		size_t globalHeaderOffset = 0;
		std::string globalHeader;

		if (mode == "a")
			FOPEN(fp, zipFileName.c_str(), "r+b");

		if (fp)
		{
			// zip file exists. we need to add a new npy file to it

			// read the footer. this gives us the offset and size of the global header
			size_t globalHeaderSize;
			detail::ParseNpzFooter(fp, nRecords, globalHeaderSize, globalHeaderOffset);
			globalHeader.resize(globalHeaderSize);
			fseek(fp, static_cast<long>(globalHeaderOffset), SEEK_SET);

			// read and store the global header.
			const size_t UNUSED elementsRead = fread(&globalHeader[0], sizeof(char), globalHeaderSize, fp);
			assert(elementsRead == globalHeaderSize);

			// below, we will write the the new data at the start of the global header then append the global header and footer below it
			fseek(fp, static_cast<long>(globalHeaderOffset), SEEK_SET);
		}
		else
			FOPEN(fp, zipFileName.c_str(), "wb");
		assert(fp != nullptr);

		const std::string npyHeader = detail::GetNpyHeader<T>(shape);

		const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1u, std::multiplies<>());
		const size_t nElementsInBytes = nElements * sizeof(T);
		size_t totalNpyFileBytes = nElementsInBytes + npyHeader.size();

		// get the CRC of the data to be added
		uint32_t crc = detail::GetCrcNpyFile(npyHeader, data);

		// get Npz info
		std::string localHeader = detail::GetLocalHeader(crc, static_cast<uint32_t>(totalNpyFileBytes), vectorName);
		detail::AppendGlobalHeader(globalHeader, localHeader, static_cast<uint32_t>(globalHeaderOffset), vectorName);
		std::string footer = detail::GetNpzFooter(globalHeader, static_cast<uint32_t>(globalHeaderOffset), static_cast<uint32_t>(localHeader.size()), static_cast<uint32_t>(totalNpyFileBytes), static_cast<uint16_t>(nRecords + 1));

		// write
		[[maybe_unused]] size_t elementsWritten = fwrite(localHeader.data(), sizeof(char), localHeader.size(), fp);
		assert(elementsWritten == localHeader.size());

		elementsWritten = fwrite(npyHeader.data(), sizeof(char), npyHeader.size(), fp);
		assert(elementsWritten == npyHeader.size());

		elementsWritten = fwrite(data.data(), sizeof(T), nElements, fp);
		assert(elementsWritten == nElements);

		elementsWritten = fwrite(globalHeader.data(), sizeof(char), globalHeader.size(), fp);
		assert(elementsWritten == globalHeader.size());

		elementsWritten = fwrite(footer.data(), sizeof(char), footer.size(), fp);
		assert(elementsWritten == footer.size());

		std::fclose(fp);
	}

	template<typename T>
	CompressedMapFull<T> LoadCompressedFull(const std::string& zipFileName)
	{
		FILE* fp = nullptr;
		FOPEN(fp, zipFileName.c_str(), "rb");
		assert(fp != nullptr);

		CompressedMapFull<T> ret;

		while (true)
		{
			constexpr size_t localHeaderSize { 30 };
			std::array<char, localHeaderSize> localHeader {};
			UNUSED_ON_NDEBUG(size_t elementsRead = ) fread(localHeader.data(), sizeof(char), localHeaderSize, fp);
			assert(elementsRead == localHeaderSize);

			// if we've reached the global header, stop reading
			if (localHeader[2] != 0x03 || localHeader[3] != 0x04)
				break;

			// read in the variable name
			uint16_t vectorNameLength = 0;
			std::memcpy(&vectorNameLength, &localHeader[26], sizeof(vectorNameLength));
			std::string vectorName(vectorNameLength, ' ');
			UNUSED_ON_NDEBUG(elementsRead = ) fread(vectorName.data(), sizeof(char), vectorNameLength, fp);
			assert(elementsRead == vectorNameLength);

			// remove the extenstion (i.e. ".npy")
			vectorName.erase(vectorName.end() - 4, vectorName.end());

			// read in the extra field
			uint16_t extraFieldsLength = 0;
			std::memcpy(&extraFieldsLength, &localHeader[28], sizeof(extraFieldsLength));
			if (extraFieldsLength > 0)
			{
				std::string tmp(extraFieldsLength, ' ');
				UNUSED_ON_NDEBUG(elementsRead = ) fread(tmp.data(), sizeof(char), extraFieldsLength, fp);
				assert(elementsRead == extraFieldsLength);
			}

			uint16_t compressionMethod = 0;
			std::memcpy(&compressionMethod, &localHeader[8], sizeof(compressionMethod));
			uint32_t compressedBytes = 0;
			std::memcpy(&compressedBytes, &localHeader[18], sizeof(compressedBytes));
			uint32_t uncompressedBytes = 0;
			std::memcpy(&uncompressedBytes, &localHeader[22], sizeof(uncompressedBytes));

			if (compressionMethod == 0)
				ret[vectorName] = detail::LoadFull<T>(fp);
			else
				ret[vectorName] = detail::LoadCompressedFull<T>(fp, compressedBytes, uncompressedBytes);
		}

		std::fclose(fp);

		return ret;
	}

#pragma endregion
}	 // namespace npypp
