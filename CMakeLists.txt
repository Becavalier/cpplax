cmake_minimum_required (VERSION 3.11)

set (CMAKE_C_COMPILER /usr/bin/clang)
set (CMAKE_CXX_COMPILER /usr/bin/clang++)

project (os-paw)

include_directories(.)

# Setup C++ configure.
set (CMAKE_VERBOSE_MAKEFILE "OFF")
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} \
  -v          \
  -O0         \
  -Wall       \
  -g          \
  -ggdb")
set (CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} \
  -v          \
  -Ofast      \
  -DNDEBUG    \
  -Wall")

set (OUTPUT "cpplox")
set (N_OUTPUT_SRC "bin")

# Resolve release source files;
aux_source_directory (./src DIR_SRCS)
set (EXECUTABLE_OUTPUT_PATH ${N_OUTPUT_SRC})

# Add subdirectory of the core library.
add_subdirectory (lib/)

# Add executable.
add_executable(${OUTPUT} ${DIR_SRCS})

target_link_libraries (${OUTPUT} cpplox-core pthread)
 