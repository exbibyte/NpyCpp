include_guard()

cmake_minimum_required(VERSION 3.14)
project(NeuralNetworks)

include(cmake/All.cmake)

create_library(
    NAME
        cnpy
    SOURCES
        Npy++/cnpy.cpp
    DO_NOT_USE_WARNINGS
)

find_package(ZLIB REQUIRED)
create_library(
    NAME
        Npy++
    SOURCES
        Npy++/Npy++.cpp
    PUBLIC_INCLUDE_DIRECTORIES
        Npy++ MemoryMapping
    SYSTEM_DEPENDENCIES
        ${ZLIB_LIBRARIES}
)

find_package(GTest REQUIRED)
create_test(
    NAME
        NpyTests
    SOURCES
        UnitTests/cnpyUnitTests.cpp
        UnitTests/main.cpp
        UnitTests/MmapNpyUnitTests.cpp
        UnitTests/NpyUnitTests.cpp
        UnitTests/NpzUnitTests.cpp
        UnitTests/pch.cpp
    DO_NOT_USE_WARNINGS
    PUBLIC_INCLUDE_DIRECTORIES
        ${GTEST_INCLUDE_DIR}
    DEPENDENCIES
        Npy++ cnpy
    SYSTEM_DEPENDENCIES
        gtest pthread
)
