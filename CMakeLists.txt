cmake_minimum_required(VERSION 3.31.6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (WIN32)
    message("haha, you stinky loser - this doesn't work on windows")
endif (WIN32)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

project(hoshimi)

set(HOSHIMI_VERSION "0.2.2")
set(HOSHIMI_RELEASE_DATE "20th September")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"  # Generate to build dir
    @ONLY
)

find_package(Boost REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(jsoncpp REQUIRED jsoncpp)

pkg_check_modules(cJSON REQUIRED libcjson)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

add_executable(hoshimi src/main.cpp)

set_target_properties(hoshimi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_compile_options(hoshimi PRIVATE
    "$<${gcc_like_cxx}:-Wall;-Werror;-Wunused;-Wformat=2>"
    "$<${msvc_cxx}:-Wall;-Werror;-Wunused;-Wformat=2>"
)
target_include_directories(hoshimi PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(hoshimi PRIVATE Boost::boost cjson ${jsoncpp_LIBRARIES})
target_include_directories(hoshimi PRIVATE ${jsoncpp_INCLUDE_DIRS} ${cJSON_INCLUDE_DIRS})
target_link_directories(hoshimi PRIVATE ${cJSON_LIBRARY_DIRS})
