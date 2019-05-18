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


find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

include_directories(MemoryMapping)
include_directories(Npy++)
include_directories(UnitTests)

add_library(cnpy
        Npy++/cnpy.cpp
)

add_library(Npy++
        Npy++/Npy++.cpp
)
target_compile_options(Npy++ PRIVATE ${WARNINGS})

find_package(ZLIB)
target_link_libraries(Npy++ ${ZLIB_LIBRARIES})

add_executable(Tests
        UnitTests/cnpyUnitTests.cpp
        UnitTests/main.cpp
        UnitTests/MmapNpyUnitTests.cpp
        UnitTests/NpyUnitTests.cpp
        UnitTests/NpzUnitTests.cpp
        UnitTests/pch.cpp
        UnitTests/pch.h
)
target_link_libraries(Tests Npy++ cnpy gtest pthread)
