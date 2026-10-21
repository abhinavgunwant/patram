# Building patram

**Note:** I'm making efforts to make the build better. More changes coming soon...

### Prerequisites

Please make sure you have the following:

#### Tools
- [cmake](https://cmake.org/download/) version 4 or later.

#### Libraries
- [GLFW 3.4](https://www.glfw.org/download.html) or later
- [FreeType](https://freetype.org/download.html)
- [libcurl](https://curl.se/download.html)

The build uses cmake's `find_package()` to find these libraries.

### Steps

1. Clone this repository
    ```bash
    git clone https://gihub.com/abhinavgunwant/patram.git
    ```

2. `cd` into the cloned directory
    ```bash
    cd patram
    ```

3. Run following commands:
    ```bash
    cmake .

    cmake --build .
    ```

    **Note:** You might want to pass your own `CMAKE_MODULE_PATH` with
    `Find####.cmake` files if you have setup your libraries in custom locations
    as: `cmake -DCMAKE_MODULE_PATH=<your-module-path> .` instead of the first
    command listed above.

The executable file: `<patram-dir>/core/Debug/patram.exe`.

