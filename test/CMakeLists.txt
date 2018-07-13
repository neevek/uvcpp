cmake_minimum_required(VERSION 2.8)

enable_testing()

set(CMAKE_CXX_FLAGS "-g -Wall -std=c++14 -DLOG_VERBOSE -DLOG_TAG_NAME='\"uvcpp\"'")
set(UVCPP_SRC_DIR ${CMAKE_SOURCE_DIR}/../src)
include_directories(${UVCPP_SRC_DIR})

find_package(Threads REQUIRED)

# Enable ExternalProject CMake module
include(ExternalProject)
# Download and install GoogleTest
ExternalProject_Add(gtest
  PREFIX "${CMAKE_BINARY_DIR}/deps"
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "master"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/deps/installed
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  UPDATE_DISCONNECTED 1
)
# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest SOURCE_DIR BINARY_DIR)

include_directories(
  "${SOURCE_DIR}/googletest/include"
  "${SOURCE_DIR}/googlemock/include"
  ) 

macro(SET_GTEST_PROP LIB_NAME LIB_FILE)
  # Create a libgmock target to be used as a dependency by test programs
  add_library(${LIB_NAME} IMPORTED STATIC GLOBAL)
  add_dependencies(${LIB_NAME} gtest)

  # Set libgmock properties
  set_target_properties(${LIB_NAME} PROPERTIES
    "IMPORTED_LOCATION" "${BINARY_DIR}/${LIB_FILE}"
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    )
endmacro()

SET_GTEST_PROP(libgtest "googlemock/gtest/libgtest.a")
SET_GTEST_PROP(libgmock "googlemock/libgmock.a")

set(COMMON_SRCS main.cc)
set(COMMON_LINK_LIBS libgtest libgmock uv)

macro(ADD_UVCPP_TEST TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${COMMON_SRCS} ${TEST_SOURCE})
  target_link_libraries(${TEST_NAME} ${COMMON_LINK_LIBS})
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

ADD_UVCPP_TEST(req uvcpp/req.cc)
ADD_UVCPP_TEST(tcp uvcpp/tcp.cc)
ADD_UVCPP_TEST(timer uvcpp/timer.cc)
ADD_UVCPP_TEST(prepare uvcpp/prepare.cc)
ADD_UVCPP_TEST(work uvcpp/work.cc)
ADD_UVCPP_TEST(buffer uvcpp/buffer.cc)
