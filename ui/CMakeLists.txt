project(ui)

add_executable(rope src/test_rope.cpp src/rope.cpp)

add_library(ui
    lib/stb_image.cpp
    lib/stb_image_write.cpp
    src/ui.cpp
    src/render.cpp
    src/shader.cpp
    src/text.cpp
    src/command.cpp
    src/rope.cpp
)

target_include_directories(ui
    PUBLIC
    src/include
    ${INCLUDE_DIR}
)

target_link_libraries(patram ${LIB_DIR})

