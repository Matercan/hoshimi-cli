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

project(hoshimi)

# Common compile options
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

set(HOSHIMI_VERSION "0.3.1")
set(HOSHIMI_RELEASE_DATE "7th October")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.h"
    @ONLY
)

# Find packages once
find_package(PkgConfig REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(cJSON REQUIRED libcjson)
pkg_check_modules(SndFile REQUIRED sndfile)
pkg_check_modules(LIBZIP REQUIRED libzip)


# Create stb_impl as an OBJECT library (compiled once, linked many times)
add_library(stb_impl OBJECT src/osu/stb_impl.c)
set_target_properties(stb_impl PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(utils OBJECT
   src/common/utils/utils.c
   src/common/utils/utils.h 
   src/common/utils/utils.hpp
)

# json_handler library
add_library(json_handler SHARED
    src/common/json/json.hpp
    src/common/json/json_wrapper.cpp
)
set_target_properties(json_handler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)
target_compile_options(json_handler PRIVATE
    "$<${gcc_like_cxx}:-Wconversion-null;-Wunused-result;-Wformat=2>"
    "$<${msvc_cxx}:-Wformat=2>"
)
target_link_libraries(json_handler cjson)
target_include_directories(json_handler PRIVATE ${cJSON_INCLUDE_DIRS})
target_link_libraries(json_handler zip)
target_include_directories(json_handler PRIVATE ${LIBZIP_INCLUDE_DIRS})

# osu library
add_library(osu SHARED
    src/osu/osu.c
    src/osu/osu.h
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:stb_impl>
)
set_target_properties(osu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)
target_include_directories(osu PUBLIC src/osu)
target_compile_options(osu PRIVATE
    "$<${gcc_like_cxx}:-Wconversion-null;-Wunused-result;-Wformat=2>"
    "$<${msvc_cxx}:-Wformat=2>"
)
target_link_libraries(osu
    m
    json_handler
    Threads::Threads
    ${SndFile_LIBRARIES}
    $<TARGET_OBJECTS:utils>
)

# osu-exe executable
add_executable(osu-exe
    src/osu/osu_assembler.c
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:stb_impl>
)
set_target_properties(osu-exe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)
target_compile_options(osu-exe PRIVATE
    "$<${gcc_like_cxx}:-Wconversion-null;-Wunused-result;-Wformat=2>"
    "$<${msvc_cxx}:-Wconversion-null;-Wunused-result;-Wformat=2>"
)
target_include_directories(osu-exe PRIVATE ${SndFile_INCLUDE_DIRS})
target_link_libraries(osu-exe
    osu
    m
    json_handler
    ${SndFile_LIBRARIES}
)

# hoshimi executable
add_executable(hoshimi
    src/main.cpp
    $<TARGET_OBJECTS:utils>
    $<TARGET_OBJECTS:stb_impl>
)
set_target_properties(hoshimi PROPERTIES
    INSTALL_RPATH "$ORIGIN/../lib"
    BUILD_RPATH "${CMAKE_CURRENT_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin"
)
target_compile_options(hoshimi PRIVATE
    "$<${gcc_like_cxx}:-Wconversion-null;-Wunused-result;-Wformat=2>"
    "$<${msvc_cxx}:-Wformat=2>"
)
target_include_directories(hoshimi PRIVATE 
    "${CMAKE_CURRENT_BINARY_DIR}"
    Boost::headers
    ${cJSON_INCLUDE_DIRS}
    ${SndFile_INCLUDE_DIRS}
)
target_link_libraries(hoshimi
    json_handler
    osu
    cjson
    m
    Threads::Threads
    ${SndFile_LIBRARIES}
)

# Install targets
install(TARGETS json_handler osu LIBRARY DESTINATION lib)
install(TARGETS hoshimi RUNTIME DESTINATION bin)
install(FILES completions/hoshimi.fish DESTINATION share/fish/vendor_completions.d)
install(FILES completions/_hoshimi DESTINATION share/zsh/site-functions)
