include_guard()

cmake_minimum_required(VERSION 3.14)
project(NpyCpp)

include(${CMAKE_SOURCE_DIR}/cmake/All.cmake)

add_subdirectory(Npy++)

if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	add_subdirectory(cnpy)
	add_subdirectory(UnitTests)
endif ()
