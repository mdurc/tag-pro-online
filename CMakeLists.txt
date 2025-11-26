cmake_minimum_required(VERSION 3.16)
project(TagPro)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

set(PROJECT_SOURCES src/main.cpp)
file(GLOB_RECURSE PROJECT_SOURCES src/*.cpp src/*.h)

add_executable(TagPro ${PROJECT_SOURCES})
target_link_libraries(TagPro Qt6::Core Qt6::Widgets Qt6::Network)
