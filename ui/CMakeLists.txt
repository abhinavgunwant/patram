set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(ui)

add_executable(rope src/test_rope.cpp src/rope.cpp)
add_executable(md src/test_md.cpp src/md.cpp src/rope.cpp)

add_library(ui
    lib/stb_image.cpp
    lib/stb_image_write.cpp
    lib/stb_image_resize2.cpp
    src/ui.cpp
    src/render.cpp
    src/shader.cpp
    src/text.cpp
    src/command.cpp
    src/rope.cpp
    src/md.cpp
)

target_include_directories(ui
    PUBLIC
    src/include
    ${INCLUDE_DIR}
)

target_link_libraries(ui
    sys
    ${LIB_DIR}
)

target_link_libraries(rope sys ${LIB_DIR})
target_link_libraries(md sys ${LIB_DIR})

