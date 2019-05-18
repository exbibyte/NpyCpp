#pragma once

/**
* Inspired by:
	http://create.stephan-brumme.com/portable-memory-mapping/#git1
*/

#ifdef _MSC_VER
    typedef unsigned __int64 uint64_t;
#else
    #include <stdint.h>
#endif

#include <MemoryMapEnumerators.h>
#include <string>
#include <vector>

namespace mm
{
	template<CacheHint cacheHint = CacheHint::SequentialScan, MapMode mapMode = MapMode::ReadOnly>
	class MemoryMappedFile
	{
	public:
		/// don't copy/move object
		MemoryMappedFile(const MemoryMappedFile&) = delete;
		MemoryMappedFile(MemoryMappedFile&&) = delete;
		MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
		MemoryMappedFile& operator=(MemoryMappedFile&&) = delete;

		/// open file, mappedBytes = 0 maps the whole file
		MemoryMappedFile(const std::string& filename, size_t mappedBytes = 0)
		    : _filename(filename), _mappedBytes(mappedBytes)
		{
			Open();
		}

		/// close file (see close() )
		~MemoryMappedFile()
		{
			Close();
		}

		/// mappedBytes = 0 maps the whole file
		bool Open();

		void Close();

		/// access position, no range checking (faster)
		inline unsigned char operator[](size_t offset) const
		{
			return GetData()[offset];
		}

		/// access position, including range checking
		unsigned char at(size_t offset) const;

		/// get file size
		uint64_t size() const
		{
			return _fileSize;
		}

		/// get number of actually mapped bytes
		size_t MappedSize() const
		{
			return _mappedBytes;
		}

		/// raw access
		inline const unsigned char* GetData() const
		{
			return reinterpret_cast<unsigned char*>(_mappedView);
		}

		/// advance the mappedView pointer
		inline void Advance(const size_t nBytes)
		{
			_mappedView = reinterpret_cast<unsigned char*>(_mappedView) + nBytes;
		}

		std::string ReadLine(const size_t maxCharToRead = 256);

		/// copy current mappedView pointer up to nElementsToRead using memcpy
		template<typename T>
		void CopyTo(T* data, const size_t nElementsToRead);

		template<typename T>
		inline void CopyTo(std::vector<T>& data)
		{
			CopyTo(data.data(), data.size());
		}

		/// reinterpret mappedView pointer into data
		template<typename T>
		void Set(const T*& data);

		template<typename T>
		inline void Set(std::vector<T>& data)
		{
			Set(&data[0]);
		}

		void ReadFrom(unsigned const char* data, const size_t nElementsToWrite);

		/// reqwind to the original mapped view pointer
		void Rewind()
		{
			_mappedView = _originMappedView;
		}

		/// true, if file successfully opened
		bool IsValid() const
		{
			return _mappedView != nullptr;
		}

		/// replace mapping by a new one of the same file, offset MUST be a multiple of the page size
		bool ReMap(uint64_t offset, size_t mappedBytes);

	private:

		/// get OS page size (for remap)
		static int GetPageSize();

		/// file name
		std::string _filename;
		
		/// file size
		uint64_t _fileSize = 0;

		/// mapped size
		size_t _mappedBytes = 0;

		/// define handle
#ifdef _MSC_VER
		typedef void* FileHandle;
		/// Windows handle to memory mapping of _file
		void* _mappedFile = nullptr;
#else
		typedef int FileHandle;
#endif

		/// file handle
		FileHandle _file = 0;
		/// pointer to the file contents mapped into memory
		void* _mappedView = nullptr;

		void* _originMappedView = nullptr;
	};
}

#include <MemoryMappedFile.tpp>

