cmake_minimum_required(VERSION 3.0)
project(impero)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_AUTOMOC ON)

set(WARNING_FLAGS -Wall -Wextra -Wpedantic)
add_library(warning_flags INTERFACE)
target_compile_options(
    warning_flags INTERFACE
    "$<$<CXX_COMPILER_ID:Clang,GNU>:$<BUILD_INTERFACE:${WARNING_FLAGS}>>")

find_package(Qt5 COMPONENTS Core Widgets REQUIRED)
add_executable(
    impero
    src/conf/conf.cpp
    src/ui/panel.cpp
    src/ui/widget.cpp
    src/main.cpp)
target_include_directories(impero PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_link_libraries(impero PRIVATE Qt5::Widgets warning_flags)
