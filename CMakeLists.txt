include_guard()

cmake_minimum_required(VERSION 3.14)
project(NpyCpp)

set(CMAKE_CXX_STANDARD 14)

# set warnings
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(WARNINGS -Weverything -Werror -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-reserved-id-macro -Wno-switch-enum -Wno-unknown-pragmas -Wno-disabled-macro-expansion -Wno-unneeded-internal-declaration)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(WARNINGS -Wall -Wextra -Werror -Wno-unknown-pragmas -Wno-unused-function)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
    set(WARNINGS -Wall -Wextra -Werror -Wno-unknown-pragmas -Wfatal-errors -fstrict-aliasing -pedantic -pedantic-errors)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    message(FATAL "MSVC not supported")
endif()

set(DEBUG_FLAGS "")
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(DEBUG_FLAGS "-D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
endif()

find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

include_directories(MemoryMapping)
include_directories(Npy++)
include_directories(UnitTests)

add_library(cnpy
        Npy++/cnpy.cpp
)
target_compile_options(cnpy PRIVATE ${DEBUG_FLAGS})

add_library(Npy++
        Npy++/Npy++.cpp
)
target_compile_options(Npy++ PRIVATE ${WARNINGS} ${DEBUG_FLAGS})

find_package(ZLIB)
target_link_libraries(Npy++ ${ZLIB_LIBRARIES})

add_executable(NpyTests
        UnitTests/cnpyUnitTests.cpp
        UnitTests/main.cpp
        UnitTests/MmapNpyUnitTests.cpp
        UnitTests/NpyUnitTests.cpp
        UnitTests/NpzUnitTests.cpp
        UnitTests/pch.cpp
        UnitTests/pch.h
)
target_compile_options(NpyTests PRIVATE ${DEBUG_FLAGS})
target_link_libraries(NpyTests Npy++ cnpy gtest pthread)
