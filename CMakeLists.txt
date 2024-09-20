cmake_minimum_required(VERSION 3.10)

# Project name
project(HydroSheds)

if(POLICY CMP0148)
  cmake_policy(SET CMP0148 NEW)
endif()

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Include directories
include_directories(include)

# Eigen3
find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIR})

# Find GDAL
find_package(GDAL REQUIRED)
include_directories(${GDAL_INCLUDE_DIRS})

# Find pybind11
find_package(pybind11 REQUIRED)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# Create the pybind11 module
pybind11_add_module(hydrosheds ${SOURCES})
target_link_libraries(hydrosheds PRIVATE ${GDAL_LIBRARIES})

# Install
install(TARGETS hydrosheds DESTINATION .)
