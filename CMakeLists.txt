cmake_minimum_required(VERSION 3.16)
project(TagPro)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)

set(PROJECT_SOURCES src/main.cpp)
file(GLOB_RECURSE PROJECT_SOURCES
     src/*.cpp src/*.h
     src/game/*.cpp src/game/*.h
     src/network/*.cpp src/network/*.h
     src/gui/*.cpp src/gui/*.h
)

add_executable(TagPro ${PROJECT_SOURCES})
target_link_libraries(TagPro Qt6::Core Qt6::Widgets Qt6::Network)
