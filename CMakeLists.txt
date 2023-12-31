cmake_minimum_required(VERSION 3.20)
set(PROJECT_NAME cpplax)
project(${PROJECT_NAME} VERSION 0.1)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)                        
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CORE_LIB_PATH lib)
set(EXECUTABLE_OUTPUT_PATH bin)

# Handle different build types.
if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
  set(DEBUG_PRINT_CODE True)
  set(DEBUG_TRACE_EXECUTION True)
  set(DEBUG_LOG_GC True)
endif()

# Replace constants.
configure_file(${CORE_LIB_PATH}/common.h.in "${PROJECT_SOURCE_DIR}/${CORE_LIB_PATH}/common.h")

# Add subdirectory of the core library.
add_subdirectory(${CORE_LIB_PATH})

# Add executable.
aux_source_directory(./src DIR_SRCS)
add_executable(${PROJECT_NAME} ${DIR_SRCS})

# Add compiling flags.
set(DEBUG_OPTIONS -v -O0 -Wall -g -ggdb)
set(RELEASE_OPTIONS -O3)
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:DEBUG>:${DEBUG_OPTIONS}>")
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<CONFIG:RELEASE>:${RELEASE_OPTIONS}>")

# Add necessary definition to C++ code.
target_compile_definitions(${PROJECT_NAME} PUBLIC "DEBUG_MODE=$<IF:$<CONFIG:Debug>,1,0>")

# Set header files.
target_include_directories(${PROJECT_NAME} PUBLIC "${PROJECT_SOURCE_DIR}/${CORE_LIB_PATH}")
message(STATUS "[Info] PROJECT_SOURCE_DIR: " "${PROJECT_SOURCE_DIR}")

# Set linking files.
target_link_libraries(${PROJECT_NAME} PUBLIC cpplax-core)

# Add tests.
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/TestConfig.cmake)
