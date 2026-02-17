set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(note)

add_library(note src/note.cpp)

target_include_directories(note PUBLIC src/include)

