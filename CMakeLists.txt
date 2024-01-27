cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(RFCSIM LANGUAGES CXX)

# yaml-cpp
add_subdirectory(yaml-cpp)

include_directories(include)
include_directories(yaml-cpp/include)
file(GLOB SOURCES "src/*.cpp")

add_executable(RFCSIM ${SOURCES})

target_link_libraries(RFCSIM yaml-cpp)
