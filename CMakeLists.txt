cmake_minimum_required(VERSION 4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

project(patram CXX)

add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(note)

