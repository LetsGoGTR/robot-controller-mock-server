cmake_minimum_required(VERSION 3.10)
project(workspace-controller)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find libarchive
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBARCHIVE REQUIRED libarchive)

add_executable(workspace-controller
  src/main.cc
  src/utils/utils.cc
  src/services/workspaceService.cc
  src/controllers/httpController.cc
)

target_include_directories(workspace-controller PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${LIBARCHIVE_INCLUDE_DIRS}
)
target_link_libraries(workspace-controller ${LIBARCHIVE_LIBRARIES} stdc++fs)
