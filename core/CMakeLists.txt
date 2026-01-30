project(core)

add_executable(patram
    src/main.cpp
    src/argparser.cpp
    src/window-utils.cpp
    src/setup-ui.cpp
    src/glad.c
)

target_include_directories(patram PRIVATE ${INCLUDE_DIR})

target_link_libraries(patram
    ui
    note
    sys
    ${LIB_DIR}
)

