include_guard()

cmake_minimum_required(VERSION 3.14)
project(NpyCpp)

include(${CMAKE_SOURCE_DIR}/cmake/All.cmake)

create_library(
    NAME
        cnpy
    SOURCES
        Npy++/cnpy.cpp
	PUBLIC_INCLUDE_DIRECTORIES
		Npy++
    DO_NOT_USE_WARNINGS
	DO_NOT_USE_PEDANTIC_WARNINGS
)

# zlib
add_subdirectory(zlib ${CMAKE_BINARY_DIR}/NpyCpp/zlib EXCLUDE_FROM_ALL)
create_library(
    NAME
        Npy++
    SOURCES
        Npy++/dummy.cpp
    PUBLIC_INCLUDE_DIRECTORIES
        Npy++ MemoryMapping
    SYSTEM_DEPENDENCIES
        zlib
)

# gtest
add_subdirectory(gtest ${CMAKE_BINARY_DIR}/NpyCpp/gtest EXCLUDE_FROM_ALL)
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
	DO_NOT_USE_PEDANTIC_WARNINGS
    PUBLIC_INCLUDE_DIRECTORIES
        ${GTEST_INCLUDE_DIR}
    DEPENDENCIES
        Npy++ cnpy
    SYSTEM_DEPENDENCIES
        gtest pthread
)
