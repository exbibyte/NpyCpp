# Npy++
This library has been forked from [this](https://github.com/rogersce/cnpy/) one, for the purpose of using more C++ infrastructure. The original README is [here](https://github.com/rogersce/cnpy/blob/master/README.md).

## Original Functionalities
- Interface for `numpy.save` and `numpy.load` from C++.
- Support for `*.npz` files

## New functionalities
- Removed support for `-rtti`:  I'm using type traits rather than `type_info`
- Loading from an `*.npz` file doesn't support heterogenous data, but they must be of the same type
- Implemented unit tests using the `gtest` framework
