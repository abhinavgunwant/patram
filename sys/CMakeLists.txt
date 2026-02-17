set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(sys)

add_library(sys
    src/utils.cpp
    src/files.cpp
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

