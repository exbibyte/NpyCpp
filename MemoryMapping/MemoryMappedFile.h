#pragma once

/**
* Inspired by:
	http://create.stephan-brumme.com/portable-memory-mapping/#git1
*/

#ifdef _MSC_VER
typedef unsigned __int64 uint64_t;
#else
	#include <cstdint>
#endif

#include <MemoryMapEnumerators.h>
#include <string>
#include <utility>
#include <vector>

namespace mm
{
	template<CacheHint = CacheHint::SequentialScan, MapMode = MapMode::ReadOnly>
	class MemoryMappedFile
	{
	public:
		/// don't copy/move object
		MemoryMappedFile(const MemoryMappedFile&) = delete;
		MemoryMappedFile(MemoryMappedFile&&) = delete;
		MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;
		MemoryMappedFile& operator=(MemoryMappedFile&&) = delete;

		/// open file, mappedBytes = 0 maps the whole file
		explicit MemoryMappedFile(std::string filename, size_t mappedBytes = 0) noexcept : _filename(std::move(filename)), _mappedBytes(mappedBytes) { Open(); }

		/// close file (see close() )
		~MemoryMappedFile() noexcept { Close(); }

		/// mappedBytes = 0 maps the whole file
		bool Open() noexcept;

		void Close() noexcept;

		/// access position, no range checking (faster)
		// NOLINTNEXTLINE(fuchsia-overloaded-operator)
		inline unsigned char operator[](size_t offset) const noexcept { return GetData()[offset]; }

		/// get file size
		[[nodiscard]] uint64_t size() const noexcept { return _fileSize; }

		/// raw access
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		[[nodiscard]] inline const unsigned char* GetData() const noexcept { return reinterpret_cast<unsigned char*>(_mappedView); }

		/// advance the mappedView pointer
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		inline void Advance(const size_t nBytes) noexcept { _mappedView = reinterpret_cast<unsigned char*>(_mappedView) + nBytes; }

		[[nodiscard]] std::string ReadLine(const size_t maxCharToRead = 256) noexcept;

		/// copy current mappedView pointer up to nElementsToRead using memcpy
		template<typename T>
		void CopyTo(T* data, const size_t nElementsToRead) noexcept;

		template<typename T>
		inline void CopyTo(std::vector<T>& data) noexcept
		{
			CopyTo(data.data(), data.size());
		}

		/// reinterpret mappedView pointer into data
		template<typename T>
		void Set(const T*& data) noexcept;

		void ReadFrom(unsigned const char* data, const size_t nElementsToWrite) noexcept;

		/// reqwind to the original mapped view pointer
		void Rewind() noexcept { _mappedView = _originMappedView; }

		/// true, if file successfully opened
		[[nodiscard]] bool IsValid() const noexcept { return _mappedView != nullptr; }

		/// replace mapping by a new one of the same file, offset MUST be a multiple of the page size
		bool ReMap(uint64_t offset, size_t mappedBytes);

	private:
		std::string _filename;
		uint64_t _fileSize = 0;
		size_t _mappedBytes = 0;

#ifdef _MSC_VER
		typedef void* FileHandle;
		/// Windows handle to memory mapping of _file
		void* _mappedFile = nullptr;
#else
		using FileHandle = int;
#endif

		FileHandle _fileHandle = 0;

		void* _mappedView = nullptr;
		void* _originMappedView = nullptr;
	};
}	 // namespace mm

#include <MemoryMappedFile.tpp>
