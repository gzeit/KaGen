################################################################################
# CMakeLists.txt
#
# Root CMake build script for generator
#
# Copyright (C) 2016-2017 Sebastian Lamm <lamm@ira.uka.de>
#
# All rights reserved. Published under the BSD-2 license in the LICENSE file.
################################################################################

cmake_minimum_required(VERSION 3.16)
project(kagen LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

################################################################################

option(KAGEN_USE_MARCH_NATIVE "Compile with -march=native." OFF)
option(KAGEN_USE_CGAL "If available, link against CGal to enable RDG generators." ON)
option(KAGEN_WARNINGS_ARE_ERRORS "Make compiler warnings compiler errors." OFF)
option(KAGEN_USE_FAST_MATH "Use -ffast-math." OFF)
option(KAGEN_BUILD_TESTS "Build unit tests." OFF)

################################################################################

include(FetchContent) 

FetchContent_Declare(googletest
    SYSTEM 
    GIT_REPOSITORY https://github.com/google/googletest.git 
    GIT_TAG release-1.12.1)

################################################################################

# Prohibit in-source builds
if ("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
    message(SEND_ERROR "In-source builds are not allowed.")
endif ()

# Default to Release building for single-config generators
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Defaulting CMAKE_BUILD_TYPE to Release")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type")
endif ()

# Warning flags
list(APPEND KAGEN_WARNING_FLAGS
    "-W"
    "-Wall"
    "-Wextra"
    "-Wcast-align"
    "-Wpedantic"
    "-Wno-unused-local-typedefs"
    )

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    list(APPEND KAGEN_WARNING_FLAGS
        "-Wextra-semi"
        "-fcolor-diagnostics"
        "-Wdeprecated"
        )
endif ()
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    list(APPEND KAGEN_WARNING_FLAGS
        "-Wcast-align"
        "-Wnoexcept"
        "-Wsuggest-attribute=noreturn"
        "-Wsuggest-override"
        "-fdiagnostics-color=always"
        "-Wcast-qual"
        "-Winit-self"
        "-Woverloaded-virtual"
        "-Wredundant-decls"
        )
endif ()

if (KAGEN_WARNINGS_ARE_ERRORS)
    list(APPEND KAGEN_WARNING_FLAGS "-Werror")
endif ()

# Enable -march=native on Debug and Release builds
if (KAGEN_USE_MARCH_NATIVE)
    include(CheckCXXCompilerFlag)
    check_cxx_compiler_flag("-march=native" KAGEN_HAS_MARCH_NATIVE)
    if(KAGEN_HAS_MARCH_NATIVE)
        list(APPEND CMAKE_CXX_FLAGS "-march=native")
    endif ()
endif ()

# Remove -rdynamic from linker flags (smaller binaries which cannot be loaded
# with dlopen() -- something no one needs)
string(REGEX REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS}")
string(REGEX REPLACE "-rdynamic" "" CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS}")

# Enable AddressSanitizer
if (OFF)
    list(APPEND CMAKE_CXX_FLAGS "-fsanitize=address")
endif ()

# Enable UndefinedBehaviorSanitizer
if (OFF)
    list(APPEND CMAKE_CXX_FLAGS "-fsanitize=undefined")
endif ()

# Use -ffast-math
if (KAGEN_USE_FAST_MATH) 
    list(APPEND CMAKE_CXX_FLAGS "-ffast-math")
endif ()

###############################################################################
# libmorton
###############################################################################
add_library(morton INTERFACE)
target_include_directories(morton SYSTEM INTERFACE extlib/libmorton/include)
list(APPEND KAGEN_LINK_LIBRARIES morton)

###############################################################################
# MPI
###############################################################################
set(MPI_DETERMINE_LIBRARY_VERSION TRUE) # needed for KaTestrophe
find_package(MPI REQUIRED)
list(APPEND KAGEN_LINK_LIBRARIES MPI::MPI_CXX)

###############################################################################
# Google Sparsehash
###############################################################################
find_package(Sparsehash REQUIRED)
list(APPEND KAGEN_LINK_LIBRARIES Sparsehash::Sparsehash)

###############################################################################
# CGAL
###############################################################################
if (KAGEN_USE_CGAL)
    set(CGAL_DO_NOT_WARN_ABOUT_CMAKE_BUILD_TYPE TRUE CACHE BOOL "Do not warn about Debug mode")
    set(CGAL_DONT_OVERRIDE_CMAKE_FLAGS TRUE CACHE BOOL "Force CGAL to maintain CMAKE flags")
    find_package(CGAL QUIET)
    if (CGAL_FOUND)
        add_definitions(-DKAGEN_CGAL_FOUND) 
        include(${CGAL_USE_FILE})
    else ()
        message(STATUS "Could not find the CGAL library: Random Delaunay Graphs will not be available")
    endif ()
endif ()

###############################################################################
# Sampling library 
###############################################################################
# Do not use MKL by default
set(SAMPLING_USE_MKL OFF CACHE BOOL "Use MKL for sampling.")

add_subdirectory(extlib/sampling EXCLUDE_FROM_ALL SYSTEM)
list(APPEND KAGEN_LINK_LIBRARIES sampling spooky)

################################################################################

add_subdirectory(kagen)
add_subdirectory(app)
add_subdirectory(library)
add_subdirectory(examples)

################################################################################

add_library(KaGen::KaGen ALIAS kagen)
add_library(KaGen::cKaGen ALIAS ckagen)

################################################################################

if (KAGEN_BUILD_TESTS)
    add_subdirectory(tests)
endif ()

