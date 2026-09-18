cmake_minimum_required(VERSION 3.10)
project(rg)

set(CMAKE_CXX_STANDARD 17)

# Traženje sistemskih paketa
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(glad/include)

add_subdirectory(glad)

add_executable(rg main.cpp
        shader.h
        camera.h
        Object.cpp
        Object.h)

target_link_libraries(rg PRIVATE glad glfw OpenGL::GL ${CMAKE_DL_LIBS})

#
#cmake_minimum_required(VERSION 3.10)
#project(rg)
#
#set(CMAKE_CXX_STANDARD 17)
#find_package(glfw3 REQUIRED)
#find_package(OpenGL REQUIRED)
#
#include_directories(.)
#include_directories(glad/include)
#
#add_subdirectory(glad/)
#add_executable(rg main.cpp)
#
#target_link_libraries(rg PRIVATE glad glfw OpenGL::GL pthread X11 Xrandr Xi dl ${CMAKE_DL_LIBS})
