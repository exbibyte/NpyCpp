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

class NpzTests : public ::testing::Test
{
public:
	NpzTests()
		: data(Nx*Ny*Nz)
	{
	}

	void SetUp()
	{
		for (int i = 0; i < Nx*Ny*Nz; i++)
			data[i] = std::complex<double>(rand(), rand());
	}
protected:
	std::vector<std::complex<double>> data;
};

TEST_F(NpzTests, ReadAndSave)
{
	npypp::SaveCompressed("arr1.npz", data, shape, "w");

	auto loadedData = npypp::LoadCompressed<std::complex<double>>("arr1.npz", "arr1");

	for (int i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData[i]);
}

TEST_F(NpzTests, ReadAndSaveFull)
{
	npypp::SaveCompressed("arr1.npz", data, shape, "w");

	auto loadedData = npypp::LoadCompressedFull<std::complex<double>>("arr1.npz", "arr1");
	ASSERT_EQ(loadedData.shape.size(), 3);
	ASSERT_EQ(loadedData.shape[0], Nz);
	ASSERT_EQ(loadedData.shape[1], Ny);
	ASSERT_EQ(loadedData.shape[2], Nx);
	ASSERT_EQ(loadedData.data.size(), TotalSize);

	for (int i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData.data[i]);
}

TEST_F(NpzTests, Append)
{
	npypp::SaveCompressed("out.npz", "arr1", data, shape, "w");
	npypp::SaveCompressed("out.npz", "arr2", data, shape, "a");

	auto dataDictionary = npypp::LoadCompressed<std::complex<double>>("out.npz");

	const auto& loadedData1 = dataDictionary["arr1"];
	const auto& loadedData2 = dataDictionary["arr2"];

	ASSERT_EQ(loadedData1.size(), TotalSize);
	ASSERT_EQ(loadedData2.size(), TotalSize);

	for (int i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData1[i]);
		ASSERT_TRUE(data[i] == loadedData2[i]);
	}
}


TEST_F(NpzTests, AppendFull)
{
	npypp::SaveCompressed("out.npz", "arr1", data, shape, "w");
	npypp::SaveCompressed("out.npz", "arr2", data, shape, "a");

	auto dataDictionary = npypp::LoadCompressedFull<std::complex<double>>("out.npz");

	const auto& loadedData1 = dataDictionary["arr1"];
	const auto& loadedData2 = dataDictionary["arr2"];

	ASSERT_EQ(loadedData1.shape.size(), 3);
	ASSERT_EQ(loadedData1.shape[0], Nz);
	ASSERT_EQ(loadedData1.shape[1], Ny);
	ASSERT_EQ(loadedData1.shape[2], Nx);
	ASSERT_EQ(loadedData1.data.size(), TotalSize);

	ASSERT_EQ(loadedData2.shape.size(), 3);
	ASSERT_EQ(loadedData2.shape[0], Nz);
	ASSERT_EQ(loadedData2.shape[1], Ny);
	ASSERT_EQ(loadedData2.shape[2], Nx);
	ASSERT_EQ(loadedData2.data.size(), TotalSize);

	for (int i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData1.data[i]);
		ASSERT_TRUE(data[i] == loadedData2.data[i]);
	}
}