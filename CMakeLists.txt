include_guard()

cmake_minimum_required(VERSION 3.14)
project(NpyCpp)

include(${CMAKE_SOURCE_DIR}/cmake/All.cmake)

add_subdirectory(cnpy)
add_subdirectory(Npy++)
add_subdirectory(UnitTests)
