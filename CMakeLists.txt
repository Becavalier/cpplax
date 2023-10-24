cmake_minimum_required(VERSION 3.20)
set(PROJECT_NAME cpplax)
project(${PROJECT_NAME} VERSION 0.1)

set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang++)                        
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CORE_LIB_PATH lib)
set(TEST_PATH test)
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
include(CTest)
file(GLOB children "${TEST_PATH}/*")
if("$ENV{TEST_TARGET}" STREQUAL "INTERPRETER")
  set(EXTRA_TEST_ARG "-i")
endif()
foreach(child ${children})
  get_filename_component(folderName "${child}" NAME)
  if(IS_DIRECTORY ${child})
    file(GLOB files "${child}/*.lax")
    foreach(file ${files})
      get_filename_component(fileName "${file}" NAME)
      add_test(NAME "${folderName}/${fileName}" COMMAND $<TARGET_FILE:cpplax> ${EXTRA_TEST_ARG} ${file})
    endforeach()
  endif()
endforeach()

# CTest will accidentally add a "\n" character at the end of each input, which actually does not belong to the original output of the test cases.
set_property(TEST assignment/associativity.lax PROPERTY PASS_REGULAR_EXPRESSION "^ccc5varvar\n$")
set_property(TEST assignment/global.lax PROPERTY PASS_REGULAR_EXPRESSION "^beforeafterargarg\n$")
set_property(TEST assignment/grouping.lax PROPERTY PASS_REGULAR_EXPRESSION "^\\\[Line 2\\\] Error: at \\\"'='\\\", invalid assignment target.\n")
set_property(TEST assignment/infix-operator.lax PROPERTY PASS_REGULAR_EXPRESSION "^\\\[Line 3\\\] Error: at \\\"'='\\\", invalid assignment target.\n$")
set_property(TEST assignment/prefix-operator.lax PROPERTY PASS_REGULAR_EXPRESSION "^\\\[Line 2\\\] Error: at \\\"'='\\\", invalid assignment target.\n$")
set_property(TEST assignment/local.lax PROPERTY PASS_REGULAR_EXPRESSION "^beforeafterargarg\n$")
set_property(TEST assignment/this.lax PROPERTY PASS_REGULAR_EXPRESSION "^\\\[Line 3\\\] Error: at \\\"'='\\\", invalid assignment target.\n$")
set_property(TEST assignment/undefined.lax PROPERTY PASS_REGULAR_EXPRESSION "undefined variable 'unknown'.")
set_property(TEST block/empty.lax PROPERTY PASS_REGULAR_EXPRESSION "^ok\n$")
set_property(TEST block/scope.lax PROPERTY PASS_REGULAR_EXPRESSION "^innerouter\n$")
set_property(TEST bool/equality.lax PROPERTY PASS_REGULAR_EXPRESSION "^truefalsefalsetruefalsefalsefalsefalsefalsefalsefalsetruetruefalsetruetruetruetruetrue\n$")
set_property(TEST bool/not.lax PROPERTY PASS_REGULAR_EXPRESSION "^falsetruetruetruefalsefalse\n$")
set_property(TEST call/bool.lax PROPERTY PASS_REGULAR_EXPRESSION "can only call functions and classes.")
set_property(TEST call/nil.lax PROPERTY PASS_REGULAR_EXPRESSION "can only call functions and classes.")
set_property(TEST call/num.lax PROPERTY PASS_REGULAR_EXPRESSION "can only call functions and classes.")
set_property(TEST call/object.lax PROPERTY PASS_REGULAR_EXPRESSION "can only call functions and classes.")
set_property(TEST call/string.lax PROPERTY PASS_REGULAR_EXPRESSION "can only call functions and classes.")
set_property(TEST class/empty.lax PROPERTY PASS_REGULAR_EXPRESSION "^<class Foo>\n$")
set_property(TEST class/inherit-self.lax PROPERTY PASS_REGULAR_EXPRESSION "^(\\\[Line 1\\\] Error: at \\\"'Foo'\\\", a class can't inherit from itself.|\\\[Line 1\\\] Error: at \\\"'Foo'\\\", undefined variable 'Foo'.)\n$")
set_property(TEST class/inherited-method.lax PROPERTY PASS_REGULAR_EXPRESSION "^in fooin barin baz\n$")
set_property(TEST class/local-inherit-other.lax PROPERTY PASS_REGULAR_EXPRESSION "^<class B>\n$")
set_property(TEST class/local-inherit-self.lax PROPERTY PASS_REGULAR_EXPRESSION "(can't read local variable in its own initializer|a class can't inherit from itself)")
set_property(TEST class/local-reference-self.lax PROPERTY PASS_REGULAR_EXPRESSION "^<class Foo>\n$")
set_property(TEST class/reference-self.lax PROPERTY PASS_REGULAR_EXPRESSION "^<class Foo>\n$")
set_property(TEST closure/assign-to-closure.lax PROPERTY PASS_REGULAR_EXPRESSION "^localafter fafter fafter g\n$")
set_property(TEST closure/assign-to-shadowed-later.lax PROPERTY PASS_REGULAR_EXPRESSION "^innerassigned\n$")
set_property(TEST closure/close-over-function-parameter.lax PROPERTY PASS_REGULAR_EXPRESSION "^param\n$")
set_property(TEST closure/close-over-later-variable.lax PROPERTY PASS_REGULAR_EXPRESSION "^ba\n$")
set_property(TEST closure/close-over-method-parameter.lax PROPERTY PASS_REGULAR_EXPRESSION "^param\n$")
set_property(TEST closure/closed-closure-in-function.lax PROPERTY PASS_REGULAR_EXPRESSION "^local\n$")
set_property(TEST closure/nested-closure.lax PROPERTY PASS_REGULAR_EXPRESSION "^abc\n$")
set_property(TEST closure/open-closure-in-function.lax PROPERTY PASS_REGULAR_EXPRESSION "^local\n$")
set_property(TEST closure/reference-closure-multiple-times.lax PROPERTY PASS_REGULAR_EXPRESSION "^aa\n$")
set_property(TEST closure/reuse-closure-slot.lax PROPERTY PASS_REGULAR_EXPRESSION "^a\n$")
set_property(TEST closure/shadow-closure-with-local.lax PROPERTY PASS_REGULAR_EXPRESSION "^closureshadowclosure\n$")
set_property(TEST closure/unused-closure.lax PROPERTY PASS_REGULAR_EXPRESSION "^ok\n$")
set_property(TEST closure/unused-later-closure.lax PROPERTY PASS_REGULAR_EXPRESSION "^a\n$")
