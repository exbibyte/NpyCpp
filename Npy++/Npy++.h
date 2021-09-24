#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>
#include <iostream>

#include <MemoryMappedFile.h>
#include <MemoryMapEnumerators.h>
#include <Enumerators.h>
#include <StringUtilities.h>

#ifndef _MSC_VER
    #ifdef NDEBUG
        #ifdef __clang__
            #define UNUSED __attribute__((unused))
            #define UNUSED_BUT_SET
        #else
            #define UNUSED __attribute__((unused))
            #define UNUSED_BUT_SET UNUSED
        #endif
    #else
        #define UNUSED
        #define UNUSED_BUT_SET
    #endif
#else
    #define UNUSED
    #define UNUSED_BUT_SET
#endif

namespace npypp
{
	// forward declaration as it's used in detail namespace
	template<typename T>
	class MultiDimensionalArray;

	namespace detail
	{
		static inline char IsBigEndian()
		{
			union
			{
				uint32_t i;
				char c[4];
			} u{ 0x01020304 };

			return u.c[0] == 1 ? '>' : '<';
		}

		template<typename T>
		struct Traits
		{
			static constexpr char id{ '?' };
		};

		template<typename T>
		static void appendBytes(std::string& out, const T in);  // in is by value as it's used only for primitive types

        #pragma region Npy Utilities

		template<typename T>
		static std::string GetNpyHeaderDescription();

		static inline std::string GetNpyHeaderFortranOrder()
		{
			return "'fortran_order': False";
		}

		static inline std::string GetNpyHeaderShape(const std::vector<size_t>& shape)
		{
			std::string ret;

			ret += "'shape': (";
			for (const auto i : shape)
			{
				ret += std::to_string(i);
				ret += ", ";
			}
			ret += ")";

			return ret;
		}

		static inline std::string SetNpyHeaderPadding(std::string& properties)
		{
			// pad with spaces so that preamble + dict is modulo 16 bytes. preamble is 10 bytes.
			// properties needs to end with newline

			auto remainder = 16 - (10 + properties.size()) % 16;
			properties.insert(properties.end(), remainder, ' ');
			properties.back() = '\n';

			return properties;
		}

		static inline auto GetMagic()
		{
		    return std::string("\x93NUMPY\x01\x00", 8);
		}

		template<typename T>
		static std::string GetNpyHeader(const std::vector<size_t>& shape);

		static inline bool ParseFortranOrder(const std::string& npyHeader)
		{
			auto position = npyHeader.find("fortran_order");
			assert(position != std::string::npos);

			return npyHeader.substr(position + 16, 4) == "True";
		}

		static inline void ParseShape(std::vector<size_t>& shape, const std::string& npyHeader)
		{
			auto loc1 = npyHeader.find("(");
			auto loc2 = npyHeader.find(")");
			assert(loc1 != std::string::npos && loc2 != std::string::npos);

			auto tokens = strutils::Tokenize<std::vector<std::string>>(npyHeader.substr(loc1 + 1, loc2 - loc1 - 1), ',');
			// avoid that last element is null
			int decrementer = 0;
			if (tokens[tokens.size() - 1].find_first_not_of(' ') == std::string::npos)
				decrementer = 1;

			shape.resize(tokens.size() - static_cast<size_t>(decrementer));
			for (size_t i = 0; i < shape.size(); ++i)
				shape[i] = static_cast<size_t>(std::atoi(tokens[i].c_str()));
		}

		/**
		* Returns the word size
		*/
		static inline size_t ParseDescription(const std::string& npyHeader)
		{
			auto position = npyHeader.find("descr");
			assert(position != std::string::npos);

			position += 9;
			//byte order code | stands for not applicable.
			//not sure when this applies except for byte array
			const bool UNUSED littleEndian = npyHeader[position] == '<' || npyHeader[position] == '|';
			assert(littleEndian);

			std::string wordSizeString = npyHeader.substr(position + 2);
			position = wordSizeString.find("'");
			assert(position != std::string::npos);
			return static_cast<size_t>(std::atoi(wordSizeString.substr(0, position).c_str()));
		}

		static inline void ParseNpyHeader(FILE* fp, size_t& wordSize, std::vector<size_t>& shape, bool& fortranOrder)
		{
			constexpr size_t expectedCharToRead{ 11 };
			char buffer[256];
			const size_t UNUSED charactersRead = fread(buffer, sizeof(char), expectedCharToRead, fp);
			assert(charactersRead == expectedCharToRead);

			std::string header = fgets(buffer, 256, fp);
			assert(header[header.size() - 1] == '\n');

			fortranOrder = ParseFortranOrder(header);
			ParseShape(shape, header);
			wordSize = ParseDescription(header);
		}

		template<typename mm::CacheHint ch = mm::CacheHint::SequentialScan, typename mm::MapMode mpm = mm::MapMode::ReadOnly>
		static void ParseNpyHeader(mm::MemoryMappedFile<ch, mpm>& mmf, size_t& wordSize, std::vector<size_t>& shape, bool& fortranOrder);

		template<typename T>
		static MultiDimensionalArray<T> LoadFull(FILE* fp);

		template<typename T, typename mm::CacheHint ch = mm::CacheHint::SequentialScan, typename mm::MapMode mpm = mm::MapMode::ReadOnly>
		static MultiDimensionalArray<T> LoadFull(mm::MemoryMappedFile<ch, mpm>& mmf);

		/**
		* WARNING: The data pointer must be nullptr when passed in, as this will not delete it
		*/
		template<typename T, typename mm::CacheHint ch = mm::CacheHint::SequentialScan, typename mm::MapMode mpm = mm::MapMode::ReadOnly>
		static void LoadNoCopy(const T*& data, mm::MemoryMappedFile<ch, mpm>& mmf);

        #pragma endregion

        #pragma region Npz Utilities

		static inline void ParseNpzFooter(FILE* fp, uint16_t& nRecords, size_t& globalHeaderSize, size_t& globalHeaderOffset)
		{
			constexpr int footerLength{ 22 };
			// footer is 22 chars long, seek the file pointer to 22 places from the end
			fseek(fp, -footerLength, SEEK_END);

			std::array<char, footerLength> footer {};
			size_t UNUSED elementsRead = fread(&footer[0], sizeof(char), footerLength, fp);
			assert(elementsRead == footerLength);

			std::memcpy(&nRecords, &footer[10], sizeof(nRecords));

                        uint32_t tmp;
                        std::memcpy(&tmp, &footer[12], sizeof(uint32_t));
                        globalHeaderSize = tmp;

			std::memcpy(&tmp, &footer[16], sizeof(uint32_t));
                        globalHeaderOffset = tmp;

			#ifndef NDEBUG
				uint16_t tmp2;
				std::memcpy(&tmp2, &footer[4], sizeof(tmp2)); assert(tmp2 == 0);
				std::memcpy(&tmp2, &footer[6], sizeof(tmp2)); assert(tmp2 == 0);
				std::memcpy(&tmp2, &footer[8], sizeof(tmp2)); assert(tmp2 == nRecords);
				std::memcpy(&tmp2, &footer[20], sizeof(tmp2)); assert(tmp2 == 0);
			#endif
		}

		template<typename T>
		static uint32_t GetCrcNpyFile(const std::string& npyHeader, const std::vector<T>& data);

		static inline std::string GetLocalHeader(const uint32_t crc, const uint32_t nBytes, const std::string& localNpyFileName)
		{
			std::string ret("PK"); // signature magic
			appendBytes<uint16_t>(ret, 0x0403); // signature magic

			appendBytes<uint16_t>(ret, 20); // min version to extract
			appendBytes<uint16_t>(ret, 0); // general purpose bit flag
			appendBytes<uint16_t>(ret, 0); // compression method
			appendBytes<uint16_t>(ret, 0); // file last mod time
			appendBytes<uint16_t>(ret, 0); // file last mod date

			appendBytes<uint32_t>(ret, crc);
			appendBytes<uint32_t>(ret, nBytes); // compressed size
			appendBytes<uint32_t>(ret, nBytes); // uncompressed size

			appendBytes<uint16_t>(ret, static_cast<uint16_t>(localNpyFileName.size())); // npy file size
			appendBytes<uint16_t>(ret, 0); // extra field length

			ret += localNpyFileName;

			return ret;
		}

		static inline void AppendGlobalHeader(std::string& out, const std::string& localHeader, const uint32_t globalHeaderOffset, const std::string& localNpyFileName)
		{
			out += "PK"; // signature magic
			appendBytes<uint16_t>(out, 0x0201); // signature magic

			appendBytes<uint16_t>(out, 20); // version made by

			out.insert(out.end(), localHeader.begin() + 4, localHeader.begin() + 30);  // insert local header

			appendBytes<uint16_t>(out, 0); // file comment length
			appendBytes<uint16_t>(out, 0); // disk number where file starts
			appendBytes<uint16_t>(out, 0); // internal file attributes

			appendBytes<uint32_t>(out, 0); // external file attributes
			appendBytes<uint32_t>(out, globalHeaderOffset); // relative offset of local file header, since it begins where the global header used to begin

			out += localNpyFileName;
		}

		static inline std::string GetNpzFooter(const std::string& globalHeader, const uint32_t globalHeaderOffset, const uint32_t localHeaderSize, const uint32_t nElementsInBytes, const uint16_t nNewRecords)
		{
			std::string ret("PK"); // signature magic
			appendBytes<uint16_t>(ret, 0x0605); // signature magic

			appendBytes<uint16_t>(ret, 0); // number of this disk
			appendBytes<uint16_t>(ret, 0); // disk where footer starts

			appendBytes<uint16_t>(ret, nNewRecords); // number of records on this disk
			appendBytes<uint16_t>(ret, nNewRecords); // total number of records

			appendBytes<uint32_t>(ret, static_cast<uint32_t>(globalHeader.size())); // nbytes of global headers
			appendBytes<uint32_t>(ret, globalHeaderOffset + nElementsInBytes + localHeaderSize); // offset of start of global headers, since global header now starts after newly written array

			appendBytes<uint16_t>(ret, 0); // zip file comment length

			return ret;
		}
		template<typename T>
		MultiDimensionalArray<T> LoadCompressedFull(FILE* fp, uint32_t compressedBytes, uint32_t uncompressedBytes);

        #pragma endregion
	}

    #pragma region Convenience Types

	template<typename T>
	class MultiDimensionalArray
	{
	public:
		MultiDimensionalArray(const std::vector<T>& data_, const std::vector<size_t>& shape_)
			: data(data_), shape(shape_)
		{
		}

		MultiDimensionalArray(const std::vector<T>& data_, std::vector<size_t>&& shape_)
			: data(data_), shape(shape_)
		{
		}

		MultiDimensionalArray() = default;
		~MultiDimensionalArray() = default;
		MultiDimensionalArray(const MultiDimensionalArray&) = default;
		MultiDimensionalArray(MultiDimensionalArray&&) = default;
		MultiDimensionalArray& operator=(const MultiDimensionalArray&) = default;
		MultiDimensionalArray& operator=(MultiDimensionalArray&&) = default;

		std::vector<T> data {};
		std::vector<size_t> shape {};
	};

	template<typename T>
	class Vector : public MultiDimensionalArray<T>
	{
	public:
		Vector(const std::vector<T>& data)
			: MultiDimensionalArray<T>(data, { data.size() })
		{
		}

		using MultiDimensionalArray<T>::MultiDimensionalArray;
	};

	template<typename T>
	class Matrix : public MultiDimensionalArray<T>
	{
	public:
		Matrix(const std::vector<T>& data(), const unsigned nRows, const unsigned nCols)
			: MultiDimensionalArray<T>(data, { nCols, nRows })
		{
		}

		using MultiDimensionalArray<T>::MultiDimensionalArray;
	};

	template<typename T>
	class Tensor : public MultiDimensionalArray<T>
	{
	public:
		Tensor(const std::vector<T>& data(), const unsigned nRows, const unsigned nCols, const unsigned nCubes)
			: MultiDimensionalArray<T>(data, { nCubes, nCols, nRows })
		{
		}

		using MultiDimensionalArray<T>::MultiDimensionalArray;
	};

    #pragma endregion

    #pragma region Load/Save Npy

	// Generic API
	/**
	* If appending, it will increase the number of rows (i.e. the first dimension in the shape vector)
	* for more flexibility use the Append method
	*/
	template<typename T>
	void Save(const std::string& fileName, const std::vector<T>& data, const std::vector<size_t>& shape, const std::string& mode = "w");

	template<typename T, typename mm::CacheHint ch = mm::CacheHint::SequentialScan, typename mm::MapMode mpm = mm::MapMode::ReadOnly>
	void Save(mm::MemoryMappedFile<ch, mpm>& mmf, const std::vector<T>& data, const std::vector<size_t>& shape);

	template<typename T>
	void Save(const std::string& fileName, const std::vector<T>& data, const std::vector<size_t>& shape, const FileOpenMode mode = FileOpenMode::Write)
	{
		Save(fileName, data, shape, ToString(mode));
	}

	template<typename T>
	void Save(const std::string& fileName, const MultiDimensionalArray<T>& array, const FileOpenMode mode = FileOpenMode::Write)
	{
		Save(fileName, array.data, array.shape, ToString(mode));
	}

	/**
	* Load the full info (data and shape) from the file
	*/
	template<typename T>
	MultiDimensionalArray<T> LoadFull(const std::string& fileName, const bool useMemoryMap = false);

	/**
	* Load the full info (data and shape) from the file using an externally set memory mapped file
	*/
	template<typename T, typename mm::CacheHint ch = mm::CacheHint::SequentialScan, typename mm::MapMode mpm = mm::MapMode::ReadOnly>
	MultiDimensionalArray<T> LoadFull(mm::MemoryMappedFile<ch, mpm>& mmf);

	template<typename T>
	std::vector<T> Load(const std::string& fileName, const bool useMemoryMap = false)
	{
		return LoadFull<T>(fileName, useMemoryMap).data;
	}

	template<typename T, typename mm::CacheHint ch = mm::CacheHint::SequentialScan, typename mm::MapMode mpm = mm::MapMode::ReadOnly>
	std::vector<T> Load(mm::MemoryMappedFile<ch, mpm>& mmf)
	{
		return LoadFull<T>(mmf).data;
	}

	template<typename T, typename mm::CacheHint ch = mm::CacheHint::SequentialScan, typename mm::MapMode mpm = mm::MapMode::ReadOnly>
	void Load(const T*& data, mm::MemoryMappedFile<ch, mpm>& mmf)
	{
		assert(data == nullptr);
		detail::LoadNoCopy<T>(data, mmf);
	}

	// API with convenience types
	template<typename T>
	void Save(const std::string& fileName, const MultiDimensionalArray<T>& array, const std::string& mode = "w")
	{
		Save(fileName, array.data, array.shape, mode);
	}

    #pragma endregion

	template <typename T>
	using CompressedMap = std::unordered_map<std::string, std::vector<T>>;

	template <typename T>
	using CompressedMapFull = std::unordered_map<std::string, MultiDimensionalArray<T>>;

    #pragma region Load/Save Npz

	/**
	* Since *.npz files supports multiple arrays in a single file, the variable name needs to be specified
	*/
	template<typename T>
	void SaveCompressed(const std::string& zipFileName, std::string vectorName, const std::vector<T>& data, const std::vector<size_t>& shape, const std::string& mode = "w");

	/**
	* If vector name is not provided, it's extracted by the zipfile name
	*/
	template<typename T>
	void SaveCompressed(const std::string& zipFileName, const std::vector<T>& data, const std::vector<size_t>& shape, const std::string& mode = "w")
	{
		std::string vectorName(zipFileName);
		vectorName.replace(vectorName.end() - 4, vectorName.end(), "");

		SaveCompressed(zipFileName, vectorName, data, shape, mode);
	}

	/**
	* Limitations: map has only one value type, so you cannot load different types in the same file
	*/
	template<typename T>
	CompressedMapFull<T> LoadCompressedFull(const std::string& zipFileName);

	/**
	* Limitations: map has only one value type, so you cannot load different types in the same file
	*/
	template<typename T>
	MultiDimensionalArray<T> LoadCompressedFull(const std::string& zipFileName, const std::string& vectorName)
	{
		CompressedMapFull<T> fullInfo = LoadCompressedFull<T>(zipFileName);

		auto iter = fullInfo.find(vectorName);
		if (iter == fullInfo.end())
			return MultiDimensionalArray<T>();

		return iter->second;
	}

	/**
	* Limitations: map has only one value type, so you cannot load different types in the same file
	*/
	template<typename T>
	CompressedMap<T> LoadCompressed(const std::string& zipFileName)
	{
		CompressedMapFull<T> fullInfo = LoadCompressedFull<T>(zipFileName);
		CompressedMap<T> ret;

		for (auto& iter : fullInfo)
			ret[iter.first] = std::move(iter.second.data);

		return ret;
	}

	/**
	* Limitations: map has only one value type, so you cannot load different types in the same file
	*/
	template<typename T>
	std::vector<T> LoadCompressed(const std::string& zipFileName, const std::string& vectorName)
	{
		return LoadCompressedFull<T>(zipFileName, vectorName).data;
	}

    #pragma endregion
}

#include <Npy++.tpp>

