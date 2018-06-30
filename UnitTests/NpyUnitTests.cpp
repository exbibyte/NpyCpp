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

class NpyTests : public ::testing::Test
{
public:
	NpyTests()
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

TEST_F(NpyTests, ReadAndSave)
{
	//save it to file
	npypp::Save("arr1.npy", data, shape, "w");

	//load it into a new array
	auto loadedData = npypp::Load<std::complex<double>>("arr1.npy");

	for (int i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData[i]);
}

TEST_F(NpyTests, ReadAndSaveFull)
{
	//save it to file
	npypp::Save("arr1.npy", data, shape, "w");

	//load it into a new array
	auto loadedData = npypp::LoadFull<std::complex<double>>("arr1.npy");
	ASSERT_EQ(loadedData.shape.size(), 3);
	ASSERT_EQ(loadedData.shape[0], Nz);
	ASSERT_EQ(loadedData.shape[1], Ny);
	ASSERT_EQ(loadedData.shape[2], Nx);
	ASSERT_EQ(loadedData.data.size(), TotalSize);

	for (int i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData.data[i]);
}

TEST_F(NpyTests, AppendFromSave)
{
	//save it to file
	npypp::Save("arr1.npy", data, shape, "w");
	npypp::Save("arr1.npy", data, shape, "a");

	//load it into a new array
	auto loadedData = npypp::Load<std::complex<double>>("arr1.npy");

	ASSERT_EQ(loadedData.size(), 2 * TotalSize);

	for (int i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData[i]);
		ASSERT_TRUE(data[i] == loadedData[i + TotalSize]);
	}
}

TEST_F(NpyTests, AppendFromSaveFull)
{
	//save it to file
	npypp::Save("arr1.npy", data, shape, "w");
	npypp::Save("arr1.npy", data, shape, "a");

	//load it into a new array
	auto loadedData = npypp::LoadFull<std::complex<double>>("arr1.npy");
	ASSERT_EQ(loadedData.shape.size(), 3);
	ASSERT_EQ(loadedData.shape[0], 2 * Nz);
	ASSERT_EQ(loadedData.shape[1], Ny);
	ASSERT_EQ(loadedData.shape[2], Nx);
	ASSERT_EQ(loadedData.data.size(), 2 * TotalSize);

	for (int i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData.data[i]);
		ASSERT_TRUE(data[i] == loadedData.data[i + TotalSize]);
	}
}