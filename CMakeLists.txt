cmake_minimum_required(VERSION 4.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Hoshimi)

set(HOSHIMI_VERSION "0.1.6")
set(HOSHIMI_RELEASE_DATE "11th September")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"  # Generate to build dir
    @ONLY
)

add_executable(Hoshimi src/main.cpp)
target_include_directories(Hoshimi PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
