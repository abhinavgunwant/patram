set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(sys)

add_library(sys
    src/utils.cpp
    src/files.cpp
    src/arena.cpp
    src/logger.cpp
    src/test.cpp
)

target_include_directories(sys
    PUBLIC
    src/include
    lib/
    ${INCLUDE_DIR}
)

target_link_libraries(sys
    ${LIB_DIR}
)

