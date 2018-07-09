cmake_minimum_required(VERSION 2.8)

################################
#            libuv             #
################################
include(ExternalProject)
ExternalProject_Add(libuv
  GIT_REPOSITORY "https://github.com/libuv/libuv.git"
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
set(uv_INCLUDE_DIRS ${source_dir}/include)
set(uv_LIBRARIES ${source_dir}/.libs/libuv.a)
file(MAKE_DIRECTORY ${uv_INCLUDE_DIRS})

add_library(uv STATIC IMPORTED)
set_target_properties(uv PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES ${uv_INCLUDE_DIRS}
  IMPORTED_LOCATION ${uv_LIBRARIES}
  INTERFACE_LINK_LIBRARIES "pthread"
  )

add_dependencies(uv libuv)

add_library(uvcpp INTERFACE)
add_dependencies(uvcpp uv)
target_include_directories(uvcpp INTERFACE src)
