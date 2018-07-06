# Npy++
This library has been forked from [this](https://github.com/rogersce/cnpy/) one, for the purpose of using more C++ infrastructure. The original README is [here](https://github.com/rogersce/cnpy/blob/master/README.md).

## Original Functionalities
- Interface for `numpy.save` and `numpy.load` from C++.
- Support for `*.npz` files

## New functionalities
- Removed support for `-rtti`:  I'm using type traits rather than `type_info`
- Loading from an `*.npz` file doesn't support heterogenous data, but they must be of the same type
- Introduced support for memory mapped files (only for `*.npy` files) 
- Implemented unit tests using the `gtest` framework

## Sample Usage

``` c++
    // Serialize (using fwrite, i.e. no memory mapping)
    std::vector<double> vec(128, 0.01);
    npypp::Save("arr1.npy", vec, { 4, 4, 8 }, "w");

    // Deserialize mapping file into memory
    const bool useMemoryMapping = true;
    auto loadedData = npypp::LoadFull<double>("arr1.npy", useMemoryMapping);

    assert(loadedData.shape.size() == 3);
    assert(loadedData.shape[0] == 4);
    assert(loadedData.shape[1] == 4);
    assert(loadedData.shape[2] == 8);
    assert(loadedData.data.size(), TotalSize);

    for (int i = 0; i < vec.size(); i++)
      assert(vec[i] == loadedData.data[i]);
      
    // Compressed files
    npypp::SaveCompressed("arr1.npz", vec, { 4, 4, 8 }, "w");
    auto loadedCompressedData = npypp::LoadCompressedFull<double>("arr1.npz", "arr1");
    assert(loadedData.shape.size() == 3);
    assert(loadedData.shape[0] == 4);
    assert(loadedData.shape[1] == 4);
    assert(loadedData.shape[2] == 8);
    assert(loadedData.data.size(), TotalSize);

    for (int i = 0; i < vec.size(); i++)
      assert(vec[i] == loadedData.data[i]);
```

### Sample Usage with Read/Write Memory Mapping
```c++
    std::vector<size_t> shape = { 4, 4, 8 };
    std::vector<double> vec(128, 0.01);
    // need to specify the file size beforehand
    const size_t nElements = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
    const auto npyHeader = npypp::detail::GetNpyHeader<double>(shape);
    const size_t fileSizeBytes = npyHeader.size() + nElements * sizeof(double);
    
    // open the file with read/write permissions
    mm::MemoryMappedFile<mm::CacheHint::SequentialScan, mm::MapMode::ReadAndWrite> mmf("arr1.npy", fileSizeBytes);
    assert(mmf.IsValid());
    npypp::Save(mmf, vec, shape);

    // rewind the mapped view pointer to its original status
    mmf.Rewind();

    // load file from shared memory
    const std::complex<double>* loadedData = nullptr;
    npypp::Load<std::complex<double>>(loadedData, mmf);

    for (int i = 0; i < TotalSize; i++)
        assert(vec[i] == loadedData[i]);
```
