cmake_minimum_required(VERSION 2.8)

################################
#            libuv             #
################################
include(ExternalProject)
ExternalProject_Add(libuv
  GIT_REPOSITORY "https://github.com/libuv/libuv.git"
  GIT_TAG "v1.x"
  CONFIGURE_COMMAND ./autogen.sh && ./configure
  BUILD_IN_SOURCE 1
  BUILD_COMMAND $(MAKE)
  INSTALL_COMMAND "" # remove install step
  TEST_COMMAND "" # remove test step
  UPDATE_COMMAND "" # remove update step
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  )
ExternalProject_Get_Property(libuv source_dir)
set(UV_INCLUDE_DIR ${source_dir}/include)
set(UV_LIBRARY ${source_dir}/.libs/libuv.a)
file(MAKE_DIRECTORY ${UV_INCLUDE_DIR})

add_library(uv STATIC IMPORTED)
add_dependencies(uv libuv)
set_target_properties(uv PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${UV_INCLUDE_DIR}
  IMPORTED_LOCATION ${UV_LIBRARY}
  INTERFACE_LINK_LIBRARIES "pthread"
  )

add_library(uvcpp INTERFACE)
target_include_directories(uvcpp INTERFACE src)
