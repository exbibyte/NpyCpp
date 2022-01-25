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
#add_subdirectory(zlib ${CMAKE_BINARY_DIR}/NpyCpp/zlib EXCLUDE_FROM_ALL)
# sudo apt install zlib1g zlib1g-dev
create_library(
    NAME
        Npy++
    SOURCES
        Npy++/dummy.cpp
    PUBLIC_INCLUDE_DIRECTORIES
        Npy++ MemoryMapping
    SYSTEM_DEPENDENCIES
        z
)

# gtest
add_subdirectory(gtest ${CMAKE_BINARY_DIR}/NpyCpp/gtest EXCLUDE_FROM_ALL)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(gtest PRIVATE -Wno-unknown-cuda-version)
endif()
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
add_custom_command(TARGET NpyTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/UnitTests/0123.npz ${CMAKE_BINARY_DIR}/0123.npz
)