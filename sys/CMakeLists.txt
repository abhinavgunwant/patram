set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(sys)

add_library(sys
    src/utils.cpp
    src/files.cpp
    src/arena.cpp
    src/logger.cpp
    src/test.cpp
    src/http.cpp
)

target_compile_definitions(sys PRIVATE CURL_STATICLIB)

target_include_directories(sys
    PUBLIC
    src/include
    lib/
    ${INCLUDE_DIR}
)

target_link_libraries(sys
    ${LIB_DIR}
    ws2_32
    crypt32
    wldap32
    normaliz
    bcrypt
    iphlpapi
    secur32
)

