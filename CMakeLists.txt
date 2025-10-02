cmake_minimum_required(VERSION 3.31.6)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)


set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_STATIC_RUNTIME OFF)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Os")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -s")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

if (WIN32)
    message("haha, you stinky loser - this doesn't work on windows")
endif (WIN32)


project(hoshimi)

set(HOSHIMI_VERSION "0.2.9")
set(HOSHIMI_RELEASE_DATE "30th September")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"  # Generate to build dir
    @ONLY
)

find_package(Boost REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(cJSON REQUIRED libcjson)

add_library(json_handler SHARED
    src/common/json/json.hpp
    src/common/json/json_wrapper.cpp
)
target_link_libraries(json_handler cjson)
target_include_directories(json_handler PRIVATE ${cJSON_INCLUDE_DIRS})

add_executable(hoshimi
    src/main.cpp
)

set_target_properties(hoshimi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_compile_options(hoshimi PRIVATE
    "$<${gcc_like_cxx}:-Wall;-Werror;-Wunused;-Wformat=2>"
    "$<${msvc_cxx}:-Wall;-Werror;-Wunused;-Wformat=2>"
)
target_include_directories(hoshimi PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(hoshimi
    json_handler
    cjson
)
target_include_directories(hoshimi PRIVATE Boost::headers)
target_include_directories(hoshimi PRIVATE ${cJSON_INCLUDE_DIRS})
target_link_directories(hoshimi PRIVATE ${cJSON_LIBRARY_DIRS})

project(osu)

add_executable(osu 
    src/osu/osu_assembler.c
    src/osu/stb_impl.c
)

set_target_properties(osu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_compile_options(osu PRIVATE 
    "$<${gcc_like_cxx}:-Wall;-Werror;-Wunused;-Wformat=2>"
    "$<${msvc_cxx}:-Wall;-Werror;-Wunused;-Wformat=2>"
)

target_link_libraries(osu
    m
    json_handler    
)
