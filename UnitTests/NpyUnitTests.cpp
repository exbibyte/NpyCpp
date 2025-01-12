#include "pch.h"

#include "IgnoreWarning.h"

#ifdef __clang__
	#define __IGNORE_CNPY_WARNINGS                                                                                                                                                                     \
		__IGNORE_WARNING__("-Wold-style-cast")                                                                                                                                                         \
		__IGNORE_WARNING__("-Wzero-as-null-pointer-constant")                                                                                                                                          \
		__IGNORE_WARNING__("-Wsign-conversion")                                                                                                                                                        \
		__IGNORE_WARNING__("-Wcast-qual")                                                                                                                                                              \
		__IGNORE_WARNING__("-Wshorten-64-to-32")
#else
	#define __IGNORE_CNPY_WARNINGS                                                                                                                                                                     \
		__IGNORE_WARNING__("-Wold-style-cast")                                                                                                                                                         \
		__IGNORE_WARNING__("-Wzero-as-null-pointer-constant")                                                                                                                                          \
		__IGNORE_WARNING__("-Wsign-conversion")                                                                                                                                                        \
		__IGNORE_WARNING__("-Wcast-qual")                                                                                                                                                              \
		__IGNORE_WARNING__("-Weffc++")

#endif

__START_IGNORING_WARNINGS__
__IGNORE_CNPY_WARNINGS
#include "cnpy/cnpy.h"
__STOP_IGNORING_WARNINGS__

#include <Npy++.h>

#include <complex>
#include <cstdlib>
#include <map>

constexpr size_t Nx { 128 };
constexpr size_t Ny { 64 };
constexpr size_t Nz { 32 };
constexpr size_t TotalSize { Nx * Ny * Nz };
const std::vector<size_t> shape { Nz, Ny, Nx };

class NpyTests: public ::testing::Test
{
public:
	NpyTests() : data(Nx * Ny * Nz) {}

	void SetUp() override
	{
		for (size_t i = 0; i < Nx * Ny * Nz; i++)
			data[i] = std::complex<double>(rand(), rand());
	}

protected:
	std::vector<std::complex<double>> data;
};

TEST_F(NpyTests, OriginalLibraryConsistency)
{
	npypp::Save("v.npy", data, shape, "w");
	cnpy::npy_save("v.cnpy", data.data(), shape, "w");

	auto loadedData = npypp::Load<std::complex<double>>("v.npy");
	auto loadedData2 = npypp::Load<std::complex<double>>("v.cnpy");

	for (size_t i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData[i]);
		ASSERT_TRUE(data[i] == loadedData2[i]);
	}
}

TEST_F(NpyTests, ReadAndSave)
{
	npypp::Save("arr1.npy", data, shape, "w");

	auto loadedData = npypp::Load<std::complex<double>>("arr1.npy");

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData[i]);
}

TEST_F(NpyTests, ReadAndSaveFull)
{
	npypp::Save("arr1.npy", data, shape, "w");

	auto loadedData = npypp::LoadFull<std::complex<double>>("arr1.npy");
	ASSERT_EQ(loadedData.shape.size(), 3);
	ASSERT_EQ(loadedData.shape[0], Nz);
	ASSERT_EQ(loadedData.shape[1], Ny);
	ASSERT_EQ(loadedData.shape[2], Nx);
	ASSERT_EQ(loadedData.data.size(), TotalSize);

	for (size_t i = 0; i < TotalSize; i++)
		ASSERT_TRUE(data[i] == loadedData.data[i]);
}

TEST_F(NpyTests, Append)
{
	npypp::Save("arr1.npy", data, shape, "w");
	npypp::Save("arr1.npy", data, shape, "a");

	auto loadedData = npypp::Load<std::complex<double>>("arr1.npy");

	ASSERT_EQ(loadedData.size(), 2 * TotalSize);

	for (size_t i = 0; i < TotalSize; i++)
	{
		ASSERT_TRUE(data[i] == loadedData[i]);
		ASSERT_TRUE(data[i] == loadedData[i + TotalSize]);
	}
}

TEST_F(NpyTests, AppendFull)
{
	npypp::Save("arr1.npy", data, shape, "w");
	npypp::Save("arr1.npy", data, shape, "a");

	auto loadedData = npypp::LoadFull<std::complex<double>>("arr1.npy");
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
