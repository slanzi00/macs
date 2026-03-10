cmake_minimum_required(VERSION 3.15)
project(macs VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find libcurl
find_package(CURL REQUIRED)

# Find nlohmann_json
find_package(nlohmann_json 3.2.0 REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CURL_INCLUDE_DIRS})

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)

# Enable testing
enable_testing()
