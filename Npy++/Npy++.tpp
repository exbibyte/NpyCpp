#pragma once

#include <complex>
#include <StringUtilities.h>

#ifdef WIN32
    #define fopen(FILE_POINTER, FILE_NAME, MODE) fopen_s(&FILE_POINTER, FILE_NAME, MODE)
#else
    #define fopen(FILE_POINTER, FILE_NAME, MODE) FILE_POINTER = fopen(FILE_NAME, MODE)
#endif

namespace npypp
{
	namespace detail
	{
        #pragma region Type Traits

		#define MAKE_TYPE_TRAITS(TYPE, CHAR)\
		template<>\
		struct Traits<TYPE>\
		{\
			static constexpr char id{ CHAR };\
		};

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
			static constexpr char id{ 'c' };
		};

        #undef MAKE_TYPE_TRAITS

        #pragma endregion

        #pragma region Npy Header

		template<typename T>
		std::string GetNpyHeaderDescription()
		{
			std::string ret;
			ret += "'descr': '";
			ret += IsBigEndian();
			ret += Traits<T>::id;
			ret += std::to_string(sizeof(T));
			ret += "'";

			return ret;
		}

		template<typename T>
		std::string GetNpyHeader(const std::vector<size_t>& shape)
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

			auto sz = (uint16_t)properties.size();
			char* szPtr = (char*)&sz;
			for (size_t byte = 0; byte < sizeof(uint16_t); byte++)
				header += *(szPtr + byte);

			header.insert(header.end(), properties.begin(), properties.end());

			return header;
		}

        #pragma endregion
	}

	template<typename T>
	void Save(const std::string& fileName, const std::vector<T> data, const std::vector<size_t> shape, const std::string& mode)
	{
		FILE* fp = NULL;
		std::vector<size_t> actualShape; //if appending, the shape of existing + new data

		if (mode == "a") 
			fopen(fp, fileName.c_str(), "r+b");

		if (fp)
		{
			std::abort();
		}
		else
		{
			fopen(fp, fileName.c_str(), "wb");
			actualShape = shape;
		}

		std::string header = detail::GetNpyHeader<T>(actualShape);
		size_t nels = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

		fseek(fp, 0, SEEK_SET);
		fwrite(&header[0], sizeof(char), header.size(), fp);
		fseek(fp, 0, SEEK_END);

		fwrite(&data[0], sizeof(T), nels, fp);
		fclose(fp);
	}

	template<typename T>
	MultiDimensionalArray<T> LoadFull(const std::string& fileName)
	{
		std::vector<T> data;

		FILE* fp = nullptr;
		fopen(fp, fileName.c_str(), "rb");

		std::vector<size_t> shape;
		size_t wordSize = 0;
		bool fortranOrder = false;
		detail::ParseNpyHeader(fp, wordSize, shape, fortranOrder);

		size_t nElements = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
		data.resize(nElements);

		assert(fread(data.data(), sizeof(T), nElements, fp) == nElements);

		MultiDimensionalArray<T> out(data, shape);
		return out;
	}
}