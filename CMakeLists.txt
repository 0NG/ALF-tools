cmake_minimum_required(VERSION 3.18...3.27)
project(ShortAES-Tools CXX)

option(CXX "enable C++ compilation" ON)
enable_language(CXX)

# use ccache
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
  message(STATUS "Found ccache")
  set_property(GLOBAL PROPERTY CXX_COMPILER_LAUNCHER ccache)
else()
  message(STATUS "Cannot find ccache")
endif()

# default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# custom cmake module
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
find_package(Threads REQUIRED)

find_package(GUROBI)
if (GUROBI_FOUND)
  message(STATUS "Found Gurobi")
  include_directories(${GUROBI_INCLUDE_DIRS})
else()
  message(WARNING "Cannot find Gurobi")
endif()

# 3rd libs
add_subdirectory(3rd)
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rd/or-tools)
include_directories(${CMAKE_CURRENT_LIST_DIR}/3rd/cxxopts/include)

# main src
add_subdirectory(src)

# idk
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  include(FeatureSummary)
  feature_summary(WHAT ALL)
endif()
