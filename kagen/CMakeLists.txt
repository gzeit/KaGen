################################################################################
# include/CMakeLists.txt
#
# CMake file for generator library
#
# Copyright (C) 2017 Sebastian Lamm <lamm@kit.edu>
#
# All rights reserved. Published under the BSD-2 license in the LICENSE file.
################################################################################

file(GLOB_RECURSE KAGEN_SOURCE_FILES CONFIGURE_DEPENDS 
	*.cpp *.h *.hpp)

# If we do not have CGal, exclude Delaunay generators 
if (NOT CGAL_FOUND)
    list(FILTER KAGEN_SOURCE_FILES EXCLUDE REGEX "delaunay.*")
endif ()

add_library(kagen_obj OBJECT ${KAGEN_SOURCE_FILES})
target_compile_features(kagen_obj PRIVATE cxx_std_17)
target_link_libraries(kagen_obj PUBLIC ${KAGEN_LINK_LIBRARIES})
target_include_directories(kagen_obj PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_options(kagen_obj PRIVATE ${KAGEN_WARNING_FLAGS})
