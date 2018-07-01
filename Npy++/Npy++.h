#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <unordered_map>

#include <Enumerators.h>
////#include <stdexcept>
////#include <sstream>
//#include <vector>
//#include <cstdio>
////#include <typeinfo>
//#include <iostream>
//#include <cassert>
////#include <zlib.h>
////#include <map>
////#include <memory>
////#include <stdint.h>
////#include <numeric>
//
//#include <Enumerators.h>

namespace npypp
{
	// forward declaration as it's used in detail namespace
	template<typename T>
	class MultiDimensionalArray;

	namespace detail
	{
		char IsBigEndian();

		template<typename T>
		struct Traits
		{
			static constexpr char id{ '?' };
		};

		template<typename T>
		void appendBytes(std::string& out, const T in);  // in is by value as it's used only for primitive types

        #pragma region Npy Utilities

		template<typename T>
		std::string GetNpyHeaderDescription();

		std::string GetNpyHeaderFortranOrder();

		std::string GetNpyHeaderShape(const std::vector<size_t>& shape);

		std::string SetNpyHeaderPadding(std::string& properties);

		std::string GetMagic();

		template<typename T>
		std::string GetNpyHeader(const std::vector<size_t>& shape);

		bool ParseFortranOrder(const std::string& npyHeader);

		void ParseShape(std::vector<size_t>& shape, const std::string& npyHeader);

		/**
		* Returns the word size
		*/
		size_t ParseDescription(const std::string& npyHeader);

		void ParseNpyHeader(FILE* fp, size_t& word_size, std::vector<size_t>& shape, bool& fortran_order);

		template<typename T>
		MultiDimensionalArray<T> LoadFull(FILE* fp);

        #pragma endregion

        #pragma region Npz Utilities

		void ParseNpzFooter(FILE* fp, uint16_t& nrecs, size_t& global_header_size, size_t& global_header_offset);

		template<typename T>
		uint32_t GetCrcNpyFile(const std::string& npyHeader, const std::vector<T>& data);

		std::string GetLocalHeader(const uint32_t crc, const uint32_t nBytes, const std::string& localNpyFileName);

		void AppendGlobalHeader(std::string& out, const std::string& localHeader, const uint32_t globalHeaderOffset, const std::string& localNpyFileName);

		std::string GetNpzFooter(const std::string& globalHeader, const uint32_t globalHeaderOffset, const uint32_t localHeaderSize, const uint32_t nElementsInBytes, const uint16_t nNewRecords);

		template<typename T>
		MultiDimensionalArray<T> LoadCompressedFull(FILE* fp, uint32_t compressedBytes, uint32_t uncompressedBytes);

        #pragma endregion
	}

    #pragma region Convenience Types

	template<typename T>
	class MultiDimensionalArray
	{
	public:
		MultiDimensionalArray(const std::vector<T>& data, const std::vector<size_t>& shape)
			: data(data), shape(shape)
		{
		}

		MultiDimensionalArray() = default;
		~MultiDimensionalArray() = default;
		MultiDimensionalArray(const MultiDimensionalArray&) = default;
		MultiDimensionalArray(MultiDimensionalArray&&) = default;
		MultiDimensionalArray& operator=(const MultiDimensionalArray&) = default;
		MultiDimensionalArray& operator=(MultiDimensionalArray&&) = default;

		std::vector<T> data;
		std::vector<size_t> shape;
	};

	template<typename T>
	class Vector : public MultiDimensionalArray<T>
	{
	public:
		Vector(const std::vector<T>& data())
			: MultiDimensionalArray(data, { data.size() })
		{
		}

		using MultiDimensionalArray::MultiDimensionalArray;
	};

	template<typename T>
	class Matrix : public MultiDimensionalArray<T>
	{
	public:
		Matrix(const std::vector<T>& data(), const unsigned nRows, const unsigned nCols)
			: MultiDimensionalArray(data, { nCols, nRows })
		{
		}

		using MultiDimensionalArray::MultiDimensionalArray;
	};

	template<typename T>
	class Tensor : public MultiDimensionalArray<T>
	{
	public:
		Tensor(const std::vector<T>& data(), const unsigned nRows, const unsigned nCols, const unsigned nCubes)
			: MultiDimensionalArray(data, { nCubes, nCols, nRows })
		{
		}

		using MultiDimensionalArray::MultiDimensionalArray;
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

	template<typename T>
	void Save(const std::string& fileName, const std::vector<T>& data, const std::vector<size_t>& shape, const FileOpenMode mode = FileOpenMode::Write)
	{
		Save(fileName, data, shape, ToString(mode));
	}

	template<typename T>
	std::vector<T> Load(const std::string& fileName)
	{
		return LoadFull<T>(fileName).data;
	}

	// API with convenience types
	template<typename T>
	void Save(const std::string& fileName, const MultiDimensionalArray<T>& array, const std::string& mode = "w")
	{
		Save(fileName, array.data, array.shape, mode);
	}

	template<typename T>
	void Save(const std::string& fileName, const MultiDimensionalArray<T>& array, const FileOpenMode mode = FileOpenMode::Write)
	{
		Save(fileName, array.data, array.shape, mode);
	}

	/**
	* Load the full info (data and shape) from the file
	*/
	template<typename T>
	MultiDimensionalArray<T> LoadFull(const std::string& fileName);

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

