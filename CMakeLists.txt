################################################################################
# CMake Configuration: Set up project and compiler settings
################################################################################
cmake_minimum_required(VERSION 3.23)
project(bxt C CXX)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall -Wextra")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
set(FETCHCONTENT_QUIET FALSE)

################################################################################
# Dependencies: Fetch and configure external libraries not available in Conan
################################################################################
include("${CMAKE_SOURCE_DIR}/cmake/bundled-deps.cmake")

################################################################################
# Dependencies: Configure dependencies available via Conan
################################################################################
include("${CMAKE_SOURCE_DIR}/cmake/deps.cmake")


################################################################################
# Project Structure: Add subdirectories for different components
################################################################################
add_subdirectory(daemon)
add_subdirectory(web)
add_subdirectory(dbcli)
