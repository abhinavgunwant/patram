project(sys)

add_library(sys
    src/utils.cpp
    src/files.cpp
)

target_include_directories(sys
    PUBLIC
    src/include
    ${INCLUDE_DIR}
)

target_link_libraries(patram ${LIB_DIR})

