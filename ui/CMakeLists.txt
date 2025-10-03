project(ui)

add_library(ui src/ui.cpp src/render.cpp src/shader.cpp)

target_include_directories(ui
    PUBLIC
    src/include
    ${INCLUDE_DIR}
)

target_link_libraries(patram ${LIB_DIR})

