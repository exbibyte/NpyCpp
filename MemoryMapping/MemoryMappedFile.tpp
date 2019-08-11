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
    
    #include <sys/stat.h>
    #include <sys/mman.h>
    #include <fcntl.h>
    #include <errno.h>
    #include <unistd.h>
    #include <cstring>  // memcpy
    #include <cassert> // assert
#endif

namespace mm
{
	/// open file
	template<CacheHint ch, MapMode mpm>
	bool MemoryMappedFile<ch, mpm>::Open()
	{
		if (mpm != MapMode::ReadOnly)
			assert(_mappedBytes != 0);  // size needs to be known when writing with mmap

		// already open ?
		if (IsValid())
			return false;

		_file = 0;
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
			//Get the error message, if any.
			DWORD errorMessageID = ::GetLastError();
			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
										 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

			std::string message(messageBuffer, size);

			//Free the buffer.
			LocalFree(messageBuffer);

			return false;
		}

#else

		// Linux

		// open file
		switch (mpm)
		{
			case MapMode::ReadOnly:
				_file = ::open(_filename.c_str(), O_RDONLY | O_LARGEFILE);
				break;
			case MapMode::WriteOnly:
			case MapMode::ReadAndWrite:
				_file = ::open(_filename.c_str(), O_RDWR | O_LARGEFILE);
				break;
			default:
				break;
		}
		if (_file == -1)
		{
			_file = 0;
			return false;
		}

		// file size
		struct stat64 statInfo{};
		if (fstat64(_file, &statInfo) < 0)
			return false;

		_fileSize = statInfo.st_size;
#endif

		// initial mapping
		ReMap(0, _mappedBytes);

		return _mappedView != nullptr;
	}

	/// close file
	template<CacheHint ch, MapMode mpm>
	void MemoryMappedFile<ch, mpm>::Close()
	{
		// kill pointer
		if (_mappedView)
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
		if (_file)
		{
#ifdef _MSC_VER
			::CloseHandle(_file);
#else
			::close(_file);
#endif
			_file = 0;
		}

		_fileSize = 0;
	}

	/// access position, including range checking
	template<CacheHint ch, MapMode mpm>
	unsigned char MemoryMappedFile<ch, mpm>::at(size_t offset) const
	{
		// checks
		if (!_mappedView)
			throw std::invalid_argument("No view mapped");
		if (offset >= _fileSize)
			throw std::out_of_range("View is not large enough");

		return operator[](offset);
	}

	/// replace mapping by a new one of the same file, offset MUST be a multiple of the page size
	template<CacheHint ch, MapMode mpm>
	bool MemoryMappedFile<ch, mpm>::ReMap(uint64_t offset, size_t mappedBytes)
	{
		if (!_file)
			return false;

		if (mappedBytes == 0)
			mappedBytes = _fileSize;

		// close old mapping
		if (_mappedView)
		{
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
				_mappedView = ::mmap64(NULL, _mappedBytes, PROT_READ, MAP_SHARED, _file, offset);
				break;
			case MapMode::WriteOnly:
				_mappedView = ::mmap64(NULL, _mappedBytes, PROT_WRITE, MAP_SHARED, _file, offset);
				break;
			case MapMode::ReadAndWrite:
				_mappedView = ::mmap64(NULL, _mappedBytes, PROT_READ | PROT_WRITE, MAP_SHARED, _file, offset);
				break;
			default:
				break;
		}

		_originMappedView = _mappedView;

		if (_mappedView == MAP_FAILED)
		{
			perror("mmap"); exit(EXIT_FAILURE);
			_mappedBytes = 0;
			_mappedView = nullptr;
			_originMappedView = nullptr;
			return false;
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

	/// get OS page size (for remap)
	template<CacheHint ch, MapMode mpm>
	int MemoryMappedFile<ch, mpm>::GetPageSize()
	{
#ifdef _MSC_VER
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);
		return sysInfo.dwAllocationGranularity;
#else
		return sysconf(_SC_PAGESIZE); //::getpagesize();
#endif
	}

	template<CacheHint ch, MapMode mpm>
	std::string MemoryMappedFile<ch, mpm>::ReadLine(const size_t maxCharToRead)
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

	template<CacheHint ch, MapMode mpm>
	template<typename T>
	void MemoryMappedFile<ch, mpm>::CopyTo(T* data, const size_t nElementsToRead)
	{	
		auto buffer = GetData();
		std::memcpy(data, buffer, sizeof(T) * nElementsToRead);
	}

	template<CacheHint ch, MapMode mpm>
	template<typename T>
	void MemoryMappedFile<ch, mpm>::Set(const T*& data)
	{
		auto buffer = GetData();
		data = reinterpret_cast<const T*>(buffer);
	}

	template<CacheHint ch, MapMode mpm>
	void MemoryMappedFile<ch, mpm>::ReadFrom(unsigned const char* data, const size_t nElementsToWrite)
	{
		unsigned char* buffer = reinterpret_cast<unsigned char*>(_mappedView);
		std::memcpy(buffer, data, nElementsToWrite);

		Advance(nElementsToWrite);
	}
}

