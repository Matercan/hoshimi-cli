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

find_package(Boost REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(jsoncpp REQUIRED jsoncpp)

add_executable(Hoshimi src/main.cpp)

set_target_properties(Hoshimi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_include_directories(Hoshimi PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(Hoshimi PRIVATE Boost::boost ${jsoncpp_LIBRARIES})
target_include_directories(Hoshimi PRIVATE ${jsoncpp_INCLUDE_DIRS})
