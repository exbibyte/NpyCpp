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

class cnpyTests : public ::testing::Test
{
public:
	cnpyTests()
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

TEST_F(cnpyTests, ReadAndSave)
{
	//save it to file
	cnpy::npy_save("arr1.npy", &data[0], shape, "w");

	//load it into a new array
	cnpy::NpyArray arr = cnpy::npy_load("arr1.npy");
	std::complex<double>* loaded_data = arr.data<std::complex<double>>();

	//make sure the loaded data matches the saved data
	ASSERT_TRUE(arr.word_size == sizeof(std::complex<double>));
	ASSERT_TRUE(arr.shape.size() == 3 && arr.shape[0] == Nz && arr.shape[1] == Ny && arr.shape[2] == Nx);

	for (int i = 0; i < Nx*Ny*Nz; i++)
		ASSERT_TRUE(data[i] == loaded_data[i]);
}

TEST_F(cnpyTests, Append)
{
	//save it to file
	cnpy::npy_save("arr1.npy", &data[0], shape, "w");

	//append the same data to file
	//npy array on file now has shape (Nz+Nz,Ny,Nx)
	cnpy::npy_save("arr1.npy", &data[0], { Nz,Ny,Nx }, "a");

	//now write to an npz file
	//non-array variables are treated as 1D arrays with 1 element
	double myVar1 = 1.2;
	char myVar2 = 'a';
	cnpy::npz_save("out.npz", "myVar1", &myVar1, { 1 }, "w"); //"w" overwrites any existing file
	cnpy::npz_save("out.npz", "myVar2", &myVar2, { 1 }, "a"); //"a" appends to the file we created above
	cnpy::npz_save("out.npz", "arr1", &data[0], { Nz,Ny,Nx }, "a"); //"a" appends to the file we created above

	cnpy::NpyArray arr2 = cnpy::npz_load("out.npz", "arr1");

	//load the entire npz file
	cnpy::npz_t my_npz = cnpy::npz_load("out.npz");

	//check that the loaded myVar1 matches myVar1
	cnpy::NpyArray arr_mv1 = my_npz["myVar1"];
	double* mv1 = arr_mv1.data<double>();
	ASSERT_TRUE(arr_mv1.shape.size() == 1 && arr_mv1.shape[0] == 1);
	ASSERT_TRUE(mv1[0] == myVar1);
}