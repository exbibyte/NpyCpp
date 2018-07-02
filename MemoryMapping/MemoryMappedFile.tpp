#pragma once

#include <stdexcept>
#include <cstdio>

// OS-specific
#ifdef _MSC_VER
// Windows
#include <windows.h>
#else
// Linux
// enable large file support on 32 bit systems
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifdef  _FILE_OFFSET_BITS
#undef  _FILE_OFFSET_BITS
#endif
#define _FILE_OFFSET_BITS 64
// and include needed headers
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif

namespace mm
{
	/// open file
	template<CacheHint ch>
	bool MemoryMappedFile<ch>::Open(const std::string& filename, size_t mappedBytes)
	{
		// already open ?
		if (IsValid())
			return false;

		file = nullptr;
		filesize = 0;

#ifdef _MSC_VER
		mappedFile = nullptr;
#endif
		mappedView = nullptr;

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
		file = ::CreateFileA(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, winHint, NULL);
		if (!file)
			return false;

		// file size
		LARGE_INTEGER result;
		if (!GetFileSizeEx(file, &result))
			return false;
		filesize = static_cast<uint64_t>(result.QuadPart);

		// convert to mapped mode
		mappedFile = ::CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
		if (!mappedFile)
			return false;

#else

		// Linux

		// open file
		file = ::open(filename.c_str(), O_RDONLY | O_LARGEFILE);
		if (file == -1)
		{
			file = 0;
			return false;
		}

		// file size
		struct stat64 statInfo;
		if (fstat64(file, &statInfo) < 0)
			return false;

		_filesize = statInfo.st_size;
#endif

		// initial mapping
		ReMap(0, mappedBytes);

		if (!mappedView)
			return false;

		// everything's fine
		return true;
	}

	/// close file
	template<CacheHint ch>
	void MemoryMappedFile<ch>::Close()
	{
		// kill pointer
		if (mappedView)
		{
#ifdef _MSC_VER
			::UnmapViewOfFile(mappedView);
#else
			::munmap(_mappedView, _filesize);
#endif
			mappedView = nullptr;
		}

#ifdef _MSC_VER
		if (mappedFile)
		{
			::CloseHandle(mappedFile);
			mappedFile = nullptr;
		}
#endif

		// close underlying file
		if (file)
		{
#ifdef _MSC_VER
			::CloseHandle(file);
#else
			::close(_file);
#endif
			file = nullptr;
		}

		filesize = 0;
	}

	/// access position, including range checking
	template<CacheHint ch>
	unsigned char MemoryMappedFile<ch>::at(size_t offset) const
	{
		// checks
		if (!mappedView)
			throw std::invalid_argument("No view mapped");
		if (offset >= filesize)
			throw std::out_of_range("View is not large enough");

		return operator[](offset);
	}

	/// replace mapping by a new one of the same file, offset MUST be a multiple of the page size
	template<CacheHint ch>
	bool MemoryMappedFile<ch>::ReMap(uint64_t offset, size_t mappedBytes)
	{
		if (!file)
			return false;

		if (mappedBytes == 0)
			mappedBytes = filesize;

		// close old mapping
		if (mappedView)
		{
#ifdef _MSC_VER
			::UnmapViewOfFile(mappedView);
#else
			::munmap(_mappedView, _mappedBytes);
#endif
			mappedView = nullptr;
		}

		// don't go further than end of file
		if (offset > filesize)
			return false;
		if (offset + mappedBytes > filesize)
			mappedBytes = size_t(filesize - offset);

#ifdef _MSC_VER
		// Windows

		DWORD offsetLow = DWORD(offset & 0xFFFFFFFF);
		DWORD offsetHigh = DWORD(offset >> 32);
		mappedBytes = mappedBytes;

		// get memory address
		mappedView = ::MapViewOfFile(mappedFile, FILE_MAP_READ, offsetHigh, offsetLow, mappedBytes);

		if (!mappedView)
		{
			mappedBytes = 0;
			mappedView = nullptr;
			return false;
		}

		return true;

#else

		// Linux
		// new mapping
		mappedView = ::mmap64(NULL, mappedBytes, PROT_READ, MAP_SHARED, _file, offset);
		if (mappedView == MAP_FAILED)
		{
			mappedBytes = 0;
			mappedView = nullptr;
			return false;
		}

		this->mappedBytes = mappedBytes;

		// tweak performance
		int linuxHint = 0;
		switch (_hint)
		{
			case Normal:
				linuxHint = MADV_NORMAL;
				break;
			case SequentialScan:
				linuxHint = MADV_SEQUENTIAL;
				break;
			case RandomAccess:
				linuxHint = MADV_RANDOM;
				break;
			default:
				break;
		}

		::madvise(_mappedView, _mappedBytes, linuxHint);

		return true;
#endif
	}

	/// get OS page size (for remap)
	template<CacheHint ch>
	int MemoryMappedFile<ch>::GetPageSize()
	{
#ifdef _MSC_VER
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwAllocationGranularity;
#else
		return sysconf(_SC_PAGESIZE); //::getpagesize();
#endif
	}

	template<CacheHint ch>
	std::string MemoryMappedFile<ch>::ReadLine(const size_t maxCharToRead)
	{
		auto buffer = GetData();

		size_t increment = 0;
		while (*(buffer + increment) != '\n')
		{
			if (increment > maxCharToRead)
				return std::string(buffer, buffer + maxCharToRead);

			++increment;
		}

		std::string out(buffer, buffer + increment + 1); // including the newline char
		Advance(increment);

		return out;
	}

	template<CacheHint ch>
	template<typename T>
	void MemoryMappedFile<ch>::CopyTo(T* data, const size_t nElementsToRead)
	{	
		auto buffer = GetData();
		std::memcpy(data, buffer, sizeof(T) * nElementsToRead);
	}

	template<CacheHint ch>
	template<typename T>
	void MemoryMappedFile<ch>::Set(const T*& data)
	{
		auto buffer = GetData();
		data = reinterpret_cast<const T*>(buffer);
		int a = 0;
	}
}

