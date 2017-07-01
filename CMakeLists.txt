cmake_minimum_required(VERSION 2.8.12)

project(whiteboard)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(whiteboard whiteboard.cpp)
target_link_libraries(whiteboard ${OpenCV_LIBS})
