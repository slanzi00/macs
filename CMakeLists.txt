cmake_minimum_required(VERSION 3.20...3.28)

if (NOT DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(FATAL_ERROR "You must set VCPKG_ROOT env variable or pass CMAKE_TOOLCHAIN_FILE")
endif()

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project(MACS
  VERSION 1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(macs-compiler-warnings INTERFACE)
target_compile_options(macs-compiler-warnings INTERFACE
  -Wall -Wextra -Wpedantic
  -Wconversion -Wsign-conversion
  -Wcast-qual -Wformat=2 -Wundef
  -Werror=float-equal
  -Wcast-align -Wunused
  -Wnull-dereference -Wdouble-promotion
  -Wimplicit-fallthrough -Wextra-semi
  -Woverloaded-virtual
  -Wold-style-cast
)

option(MACS_SANITIZE_ADDRESS   "Enable address sanitizer")
option(MACS_SANITIZE_UNDEFINED "Enable undefined-behaviour sanitizer")
option(MACS_SANITIZE_THREAD    "Enable thread sanitizer")

add_library(macs-sanitizers INTERFACE)

if (MACS_SANITIZE_ADDRESS)
  target_compile_options(macs-sanitizers INTERFACE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(macs-sanitizers INTERFACE -fsanitize=address -fno-omit-frame-pointer)
endif()

if (MACS_SANITIZE_UNDEFINED)
  target_compile_options(macs-sanitizers INTERFACE -fsanitize=undefined -fno-omit-frame-pointer)
  target_link_options(macs-sanitizers INTERFACE -fsanitize=undefined -fno-omit-frame-pointer)
endif()

if (MACS_SANITIZE_THREAD)
  target_compile_options(macs-sanitizers INTERFACE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(macs-sanitizers INTERFACE -fsanitize=thread -fno-omit-frame-pointer)
endif()

find_package(lyra REQUIRED)
find_package(doctest REQUIRED)
find_package(spdlog REQUIRED)
find_package(cpr REQUIRED)
find_package(nlohmann_json REQUIRED)

add_subdirectory(src)

include(CTest)
add_subdirectory(tests)
