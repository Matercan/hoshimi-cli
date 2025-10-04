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

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_MODE ON)  
  configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/config.h.in"
      "${CMAKE_CURRENT_BINARY_DIR}/config.h"
      @ONLY
  ) 
endif()

project(osu-exe)
project(hoshimi)

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wno-unused-parameter
    -Wno-unused-value
    -Wno-missing-field-initializers
    -Wno-gnu-zero-variadic-macro-arguments
    -Wno-narrowing
    -Wno-pointer-arith
    -fdiagnostics-color=always
)

set(HOSHIMI_VERSION "0.3.0")
set(HOSHIMI_RELEASE_DATE "3rd October")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"  # Generate to build dir
    @ONLY
)

find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED)
pkg_check_modules(cJSON REQUIRED libcjson)
pkg_check_modules(SndFile REQUIRED sndfile)


add_library(json_handler SHARED
    src/common/json/json.hpp
    src/common/json/json_wrapper.cpp
)
set_target_properties(json_handler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)
target_compile_options(json_handler PRIVATE 
    "$<${gcc_like_cxx}:-Wall;-Wconversion-null;-Wunused-result;-Wunused;-Wformat=2>"
    "$<${msvc_cxx}:-Wall;-Wunused;-Wformat=2>"
)

target_link_libraries(json_handler cjson)
target_include_directories(json_handler PRIVATE ${cJSON_INCLUDE_DIRS})



add_executable(osu-exe 
    src/osu/osu_assembler.c 
    src/osu/stb_impl.c
)

set_target_properties(osu-exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_compile_options(osu-exe PRIVATE 
    "$<${gcc_like_cxx}:-Wall;-Wconversion-null;-Wunused-result;-Wunused;-Wformat=2>"
    "$<${msvc_cxx}:-Wall;-Wconversion-null;-Wunused-result;-Wunused;-Wformat=2>"
)

target_link_libraries(osu-exe
    m
    json_handler
    ${SndFile_LIBRARIES}
)

target_include_directories(osu-exe PRIVATE ${SndFile_INCLUDE_DIRS})
target_link_directories(osu-exe PRIVATE ${SndFile_LIBRARY_DIRS})


add_library(osu SHARED
    src/osu/osu.c
    src/osu/osu.h
    src/osu/stb_impl.c
)

set_target_properties(osu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_include_directories(osu PUBLIC src/osu)

target_compile_options(osu PRIVATE 
    "$<${gcc_like_cxx}:-Wall;-Wconversion-null;-Wunused-result;-Wunused;-Wformat=2>"
    "$<${msvc_cxx}:-Wall;-Wconversion-null;-Wunused-result;-Wunused;-Wformat=2>"
)

target_link_libraries(osu
    m
    json_handler
    ${SndFile_LIBRARIES}
)



add_executable(hoshimi
    src/main.cpp
    src/osu/stb_impl.c
)

set_target_properties(hoshimi PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)

target_compile_options(hoshimi PRIVATE
    "$<${gcc_like_cxx}:-Wall;-Wconversion-null;-Wunused-result;-Wunused;-Wformat=2>"
    "$<${msvc_cxx}:-Wall;-Wconversion-null;-Wunused-result;-Wunused;-Wformat=2>"
)
target_include_directories(hoshimi PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(hoshimi
    json_handler
    osu
    cjson
    m
    ${SndFile_LIBRARIES}
)
target_include_directories(hoshimi PRIVATE Boost::headers)
target_include_directories(hoshimi PRIVATE ${cJSON_INCLUDE_DIRS})
target_link_directories(hoshimi PRIVATE ${cJSON_LIBRARY_DIRS})
target_include_directories(hoshimi PRIVATE ${SndFile_INCLUDE_DIRS})
target_link_directories(hoshimi PRIVATE ${SndFile_LIBRARY_DIRS})

install(TARGETS hoshimi
    RUNTIME DESTINATION bin
)

# Fish completions
install(FILES completions/hoshimi.fish
    DESTINATION share/fish/vendor_completions.d
)

# Zsh completions
install(FILES completions/_hoshimi
    DESTINATION share/zsh/site-functions
)
