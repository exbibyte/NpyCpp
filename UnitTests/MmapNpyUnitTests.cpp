#include "pch.h"
#include <cnpy.h>
#include <Npy++.h>

#include<complex>
#include<cstdlib>
#include<iostream>
#include<map>
#include<string>
#include<array>

constexpr size_t Nx{ 128 };
constexpr size_t Ny{ 64 };
constexpr size_t Nz{ 32 };
constexpr size_t TotalSize{ Nx * Ny * Nz };
const std::vector<size_t> shape{ Nz, Ny, Nx };

class MmapNpyTests : public ::testing::Test
{
public:
	MmapNpyTests()
		: data(Nx*Ny*Nz)
	{
	}

	void SetUp()
	{
		for (size_t i = 0; i < Nx*Ny*Nz; i++)
			data[i] = std::complex<double>(rand(), rand());
	}
protected:
	std::vector<std::complex<double>> data;
};

TEST_F(MmapNpyTests, SaveAndReadFromMappedFile)
{
	npypp::Save("arr1.npy", data, shape, "w");

	auto loadedData = npypp::Load<std::complex<double>>("arr1.npy", true);

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData[i]);
}

TEST_F(MmapNpyTests, SaveFromMappedFileAndRead)
{
	// need to specify the file size beforehand
	const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
	const auto npyHeader = npypp::detail::GetNpyHeader<std::complex<double>>(shape);
	const size_t fileSizeBytes = npyHeader.size() + nElements * sizeof(std::complex<double>);
	auto mmf = new mm::MemoryMappedFile<mm::CacheHint::SequentialScan, mm::MapMode::WriteOnly>("arr1.npy", fileSizeBytes);
	ASSERT_TRUE(mmf->IsValid());
	npypp::Save(*mmf, data, shape);

	delete mmf;  // close file as it needs to be used later on

	auto loadedData = npypp::Load<std::complex<double>>("arr1.npy");

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData[i]);
}

TEST_F(MmapNpyTests, SaveFromMappedFileAndReadFull)
{
	// need to specify the file size beforehand
	const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
	const auto npyHeader = npypp::detail::GetNpyHeader<std::complex<double>>(shape);
	const size_t fileSizeBytes = npyHeader.size() + nElements * sizeof(std::complex<double>);
	auto mmf = new mm::MemoryMappedFile<mm::CacheHint::SequentialScan, mm::MapMode::WriteOnly>("arr1.npy", fileSizeBytes);
	ASSERT_TRUE(mmf->IsValid());
	npypp::Save(*mmf, data, shape);

	delete mmf;  // close file as it needs to be used later on

	auto loadedData = npypp::LoadFull<std::complex<double>>("arr1.npy");
	ASSERT_EQ(loadedData.shape.size(), 3);
	ASSERT_EQ(loadedData.shape[0], Nz);
	ASSERT_EQ(loadedData.shape[1], Ny);
	ASSERT_EQ(loadedData.shape[2], Nx);
	ASSERT_EQ(loadedData.data.size(), TotalSize);

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData.data[i]);
}

TEST_F(MmapNpyTests, SaveFromMappedFileAndReadFromMappedFile)
{
	// need to specify the file size beforehand
	const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
	const auto npyHeader = npypp::detail::GetNpyHeader<std::complex<double>>(shape);
	const size_t fileSizeBytes = npyHeader.size() + nElements * sizeof(std::complex<double>);
	mm::MemoryMappedFile<mm::CacheHint::SequentialScan, mm::MapMode::ReadAndWrite> mmf("arr1.npy", fileSizeBytes);
	ASSERT_TRUE(mmf.IsValid());
	npypp::Save(mmf, data, shape);

	//rewind
	mmf.Rewind();
	ASSERT_TRUE(mmf.IsValid());

	const std::complex<double>* loadedData = nullptr;
	npypp::Load<std::complex<double>>(loadedData, mmf);

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData[i]);
}

TEST_F(MmapNpyTests, SaveAndReadFromMappedFileNoCopy)
{
	npypp::Save("arr1.npy", data, shape, "w");

	mm::MemoryMappedFile<mm::CacheHint::SequentialScan> mmf("arr1.npy");
	const std::complex<double>* loadedData = nullptr;
	npypp::Load<std::complex<double>>(loadedData, mmf);

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData[i]);
}

TEST_F(MmapNpyTests, SaveAndReadFullFromMappedFileNoCopy)
{
	npypp::Save("arr1.npy", data, shape, "w");

	auto loadedData = npypp::LoadFull<std::complex<double>>("arr1.npy", true);
	ASSERT_EQ(loadedData.shape.size(), 3);
	ASSERT_EQ(loadedData.shape[0], Nz);
	ASSERT_EQ(loadedData.shape[1], Ny);
	ASSERT_EQ(loadedData.shape[2], Nx);
	ASSERT_EQ(loadedData.data.size(), TotalSize);

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData.data[i]);
}

TEST_F(MmapNpyTests, Append)
{
	npypp::Save("arr1.npy", data, shape, "w");
	npypp::Save("arr1.npy", data, shape, "a");

	auto loadedData = npypp::Load<std::complex<double>>("arr1.npy", true);

	ASSERT_EQ(loadedData.size(), 2 * TotalSize);

	for (size_t i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData[i]);
		ASSERT_TRUE(data[i] == loadedData[i + TotalSize]);
	}
}

TEST_F(MmapNpyTests, AppendAndReadFromMappedFileWithNoCopy)
{
	npypp::Save("arr1.npy", data, shape, "w");
	npypp::Save("arr1.npy", data, shape, "a");

	mm::MemoryMappedFile<mm::CacheHint::SequentialScan> mmf("arr1.npy");
	const std::complex<double>* loadedData = nullptr;
	npypp::Load<std::complex<double>>(loadedData, mmf);

	for (size_t i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData[i]);
		ASSERT_TRUE(data[i] == loadedData[i + TotalSize]);
	}
}

TEST_F(MmapNpyTests, AppendFull)
{
	npypp::Save("arr1.npy", data, shape, "w");
	npypp::Save("arr1.npy", data, shape, "a");

	auto loadedData = npypp::LoadFull<std::complex<double>>("arr1.npy", true);
	ASSERT_EQ(loadedData.shape.size(), 3);
	ASSERT_EQ(loadedData.shape[0], 2 * Nz);
	ASSERT_EQ(loadedData.shape[1], Ny);
	ASSERT_EQ(loadedData.shape[2], Nx);
	ASSERT_EQ(loadedData.data.size(), 2 * TotalSize);

	for (size_t i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData.data[i]);
		ASSERT_TRUE(data[i] == loadedData.data[i + TotalSize]);
	}
}
