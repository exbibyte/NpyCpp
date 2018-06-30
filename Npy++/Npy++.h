#pragma once

#include <string>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <cstdio>
#include <typeinfo>
#include <iostream>
#include <cassert>
#include <zlib.h>
#include <map>
#include <memory>
#include <stdint.h>
#include <numeric>

#include <Enumerators.h>

namespace npypp
{
	namespace detail
	{
		char IsBigEndian();

		template<typename T>
		struct Traits
		{
			static constexpr char id{ '?' };
		};

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
		void Save(const std::string& fileName, 
				  const std::vector<T> data, 
				  const std::vector<size_t> shape, 
				  const std::string& mode = "w",
				  const size_t dimensionToIncrease = 0);
	}

	// Generic API
	/**
	* If appending, it will increase the number of rows (i.e. the first dimension in the shape vector)
	* for more flexibility use the Append method
	*/
	template<typename T>
	void Save(const std::string& fileName, const std::vector<T> data, const std::vector<size_t> shape, const std::string& mode = "w")
	{
		detail::Save(fileName, data, shape, mode, 0);
	}

	template<typename T>
	void Save(const std::string& fileName, const std::vector<T> data, const std::vector<size_t> shape, const FileOpenMode mode = FileOpenMode::Write)
	{
		Save(fileName, data, shape, ToString(mode));
	}

	template<typename T>
	void Append(const std::string& fileName, const std::vector<T> data, const std::vector<size_t> shape, const size_t dimensionToIncrease = 0)
	{
		assert(dimensionToIncrease < shape.size());
		Save(fileName, data, shape, "a", dimensionToIncrease);
	}

	template<typename T>
	std::vector<T> Load(const std::string& fileName)
	{
		return LoadFull<T>(fileName).data;
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
	class Vector: public MultiDimensionalArray<T>
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

	template<typename T>
	void Append(const std::string& fileName, const MultiDimensionalArray<T>& array, const size_t dimensionToIncrease = 0)
	{
		Append(fileName, array.data, array.shape, dimensionToIncrease);
	}

	/**
	* Load the full info (data and shape) from the file
	*/
	template<typename T>
	MultiDimensionalArray<T> LoadFull(const std::string& fileName);
}

#include <Npy++.tpp>

