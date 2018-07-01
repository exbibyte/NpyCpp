# Npy++
This library has been forked from [this](https://github.com/rogersce/cnpy/) one, for the purpose of using more C++ infrastructure. The original README is [here](https://github.com/rogersce/cnpy/blob/master/README.md).

## Original Functionalities
- Interface for `numpy.save` and `numpy.load` from C++.
- Support for `*.npz` files

## New functionalities
- Removed support for `-rtti`:  I'm using type traits rather than `type_info`
- Loading from an `*.npz` file doesn't support heterogenous data, but they must be of the same type
- Introduced support for memory mapped files (only for `*.npy` files
- Implemented unit tests using the `gtest` framework

## Sample Usage

``` c++
    // Serialize
    std::vector<double> vec(128, 0.01);
    npypp::Save("arr1.npy", vec, { 4, 4, 8 }, "w");

    // Deserialize
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
