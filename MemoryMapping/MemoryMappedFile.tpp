#pragma once

#include <cstdio>
#include <stdexcept>

// OS-specific
#ifdef _MSC_VER
	// Windows
	#include <windows.h>
#else
	// Linux
	// enable large file support on 32-bit systems
	#ifndef _LARGEFILE64_SOURCE
		#define _LARGEFILE64_SOURCE
	#endif

	#ifdef _FILE_OFFSET_BITS
		#undef _FILE_OFFSET_BITS
	#endif

	#define _FILE_OFFSET_BITS 64

	#include <cassert>	  // assert
	#include <cerrno>
	#include <cstring>	  // memcpy
	#include <fcntl.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <unistd.h>
#endif

namespace mm
{
	/// open file
	template<CacheHint ch, MapMode mpm>
	bool MemoryMappedFile<ch, mpm>::Open() noexcept
	{
		if (mpm != MapMode::ReadOnly)
			assert(_mappedBytes != 0);	  // size needs to be known when writing with mmap

		// already open ?
		if (IsValid())
			return false;

		_fileHandle = 0;
		_fileSize = 0;

#ifdef _MSC_VER
		_mappedFile = nullptr;
#endif
		_mappedView = nullptr;

#ifdef _MSC_VER
		DWORD winHint = 0;
		switch (ch)
		{
			case CacheHint::Normal:
				winHint = FILE_ATTRIBUTE_NORMAL;
				break;
			case CacheHint::SequentialScan:
				winHint = FILE_FLAG_SEQUENTIAL_SCAN;
				break;
			case CacheHint::RandomAccess:
				winHint = FILE_FLAG_RANDOM_ACCESS;
				break;
			default:
				break;
		}

		// open file
		switch (mpm)
		{
			case MapMode::ReadOnly:
				_file = ::CreateFileA(_filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, winHint, NULL);
				break;
			case MapMode::WriteOnly:
			case MapMode::ReadAndWrite:
				_file = ::CreateFileA(_filename.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, winHint, NULL);
				break;
			default:
				break;
		}
		if (!_file)
			return false;

		// file size
		LARGE_INTEGER result;
		if (!GetFileSizeEx(_file, &result))
			return false;
		_fileSize = static_cast<uint64_t>(result.QuadPart);

		// convert to mapped mode
		switch (mpm)
		{
			case MapMode::ReadOnly:
				_mappedFile = ::CreateFileMapping(_file, NULL, PAGE_READONLY, 0, 0, NULL);
				break;
			case MapMode::WriteOnly:
			case MapMode::ReadAndWrite:
				_mappedFile = ::CreateFileMapping(_file, NULL, PAGE_READWRITE, 0, 0, NULL);
				break;
			default:
				break;
		}
		if (!_mappedFile)
		{
			// Get the error message, if any.
			DWORD errorMessageID = ::GetLastError();
			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

			std::string message(messageBuffer, size);

			// Free the buffer.
			LocalFree(messageBuffer);

			return false;
		}

#else

		// Linux

		// open file
		switch (mpm)
		{
			case MapMode::ReadOnly:
				// NOLINTNEXTLINE(*)
				_fileHandle = ::open(_filename.c_str(), O_RDONLY | O_LARGEFILE);
				break;
			case MapMode::WriteOnly:
			case MapMode::ReadAndWrite:
				// NOLINTNEXTLINE(*)
				_fileHandle = ::open(_filename.c_str(), O_RDWR | O_LARGEFILE);
				break;
			default:
				_fileHandle = -1;
				break;
		}
		if (_fileHandle == -1)
		{
			_fileHandle = 0;
			return false;
		}

		// file size
		struct stat64 statInfo
		{
		};
		if (fstat64(_fileHandle, &statInfo) < 0)
			return false;

		_fileSize = static_cast<uint64_t>(statInfo.st_size);
#endif

		// initial mapping
		ReMap(0, _mappedBytes);

		return _mappedView != nullptr;
	}

	/// close file
	template<CacheHint ch, MapMode mpm>
	void MemoryMappedFile<ch, mpm>::Close() noexcept
	{
		Rewind();
		// kill pointer
		if (_mappedView != nullptr)
		{
#ifdef _MSC_VER
			::UnmapViewOfFile(_mappedView);
#else
			::munmap(_mappedView, _fileSize);
#endif
			_mappedView = nullptr;
		}

#ifdef _MSC_VER
		if (_mappedFile)
		{
			::CloseHandle(_mappedFile);
			_mappedFile = nullptr;
		}
#endif

		// close underlying file
		if (_fileHandle != 0)
		{
#ifdef _MSC_VER
			::CloseHandle(_file);
#else
			::close(_fileHandle);
#endif
			_fileHandle = 0;
		}

		_fileSize = 0;
	}

	/// replace mapping by a new one of the same file, offset MUST be a multiple of the page size
	template<CacheHint ch, MapMode mpm>
	bool MemoryMappedFile<ch, mpm>::ReMap(uint64_t offset, size_t mappedBytes)
	{
		if (_fileHandle == 0)
			return false;

		if (mappedBytes == 0)
			mappedBytes = _fileSize;

		// close old mapping
		if (_mappedView != nullptr)
		{
			Rewind();
#ifdef _MSC_VER
			::UnmapViewOfFile(_mappedView);
#else
			::munmap(_mappedView, mappedBytes);
#endif
			_mappedView = nullptr;
		}

		// don't go further than end of file
		if (offset > _fileSize)
			return false;
		if (offset + mappedBytes > _fileSize)
			mappedBytes = size_t(_fileSize - offset);

		_mappedBytes = mappedBytes;

#ifdef _MSC_VER
		// Windows

		DWORD offsetLow = DWORD(offset & 0xFFFFFFFF);
		DWORD offsetHigh = DWORD(offset >> 32);

		// get memory address
		switch (mpm)
		{
			case MapMode::ReadOnly:
				_mappedView = ::MapViewOfFile(_mappedFile, FILE_MAP_READ, offsetHigh, offsetLow, _mappedBytes);
				break;
			case MapMode::WriteOnly:
				_mappedView = ::MapViewOfFile(_mappedFile, FILE_MAP_WRITE, offsetHigh, offsetLow, _mappedBytes);
				break;
			case MapMode::ReadAndWrite:
				_mappedView = ::MapViewOfFile(_mappedFile, FILE_MAP_ALL_ACCESS, offsetHigh, offsetLow, _mappedBytes);
				break;
			default:
				break;
		}

		_originMappedView = _mappedView;

		if (!_mappedView)
		{
			_mappedBytes = 0;
			_mappedView = nullptr;
			_originMappedView = nullptr;
			return false;
		}

		return true;

#else

		// Linux
		// new mapping
		switch (mpm)
		{
			case MapMode::ReadOnly:
				// NOLINTNEXTLINE(*)
				_mappedView = ::mmap64(nullptr, _mappedBytes, PROT_READ, MAP_SHARED, _fileHandle, static_cast<long>(offset));
				break;
			case MapMode::WriteOnly:
				// NOLINTNEXTLINE(*)
				_mappedView = ::mmap64(nullptr, _mappedBytes, PROT_WRITE, MAP_SHARED, _fileHandle, static_cast<long>(offset));
				break;
			case MapMode::ReadAndWrite:
				// NOLINTNEXTLINE(*)
				_mappedView = ::mmap64(nullptr, _mappedBytes, PROT_READ | PROT_WRITE, MAP_SHARED, _fileHandle, static_cast<long>(offset));
				break;
			default:
				// NOLINTNEXTLINE(*)
				_mappedView = MAP_FAILED;
				break;
		}

		_originMappedView = _mappedView;

		// NOLINTNEXTLINE(*)
		if (_mappedView == MAP_FAILED)
		{
			perror("mmap");
			_mappedBytes = 0;
			_mappedView = nullptr;
			_originMappedView = nullptr;
			std::abort();
		}

		// tweak performance
		int linuxHint = 0;
		switch (ch)
		{
			case CacheHint::Normal:
				linuxHint = MADV_NORMAL;
				break;
			case CacheHint::SequentialScan:
				linuxHint = MADV_SEQUENTIAL;
				break;
			case CacheHint::RandomAccess:
				linuxHint = MADV_RANDOM;
				break;
			default:
				break;
		}

		::madvise(_mappedView, _mappedBytes, linuxHint);

		return true;
#endif
	}

	template<CacheHint ch, MapMode mpm>
	std::string MemoryMappedFile<ch, mpm>::ReadLine(const size_t maxCharToRead) noexcept
	{
		auto buffer = GetData();

		size_t increment = 0;
		while (*(buffer + increment) != '\n')
		{
			if (increment > maxCharToRead)
				return std::string(buffer, buffer + maxCharToRead);

			++increment;
		}

		std::string out(buffer, buffer + increment + 1);	// including the newline char
		Advance(increment);

		return out;
	}

	template<CacheHint ch, MapMode mpm>
	template<typename T>
	[[maybe_unused]] void MemoryMappedFile<ch, mpm>::CopyTo(T* data, const size_t nElementsToRead) noexcept
	{
		auto buffer = GetData();
		std::memcpy(data, buffer, sizeof(T) * nElementsToRead);
	}

	template<CacheHint ch, MapMode mpm>
	template<typename T>
	void MemoryMappedFile<ch, mpm>::Set(const T*& data) noexcept
	{
		auto buffer = GetData();
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		data = reinterpret_cast<const T*>(buffer);
	}

	template<CacheHint ch, MapMode mpm>
	void MemoryMappedFile<ch, mpm>::ReadFrom(unsigned const char* data, const size_t nElementsToWrite) noexcept
	{
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		auto* buffer = reinterpret_cast<unsigned char*>(_mappedView);
		std::memcpy(buffer, data, nElementsToWrite);

		Advance(nElementsToWrite);
	}
}	 // namespace mm
