# patram

Simple secure note-taking app.

This project is a Work-in-progress!

## Building

I'm making efforts to make the build better. Changes coming soon...

### Prerequisites

Please make sure you have the following:
- GLFW 3.4 or compatible version
- FreeType

### Steps

1. Create `CMakeUserPresets.json` in the root directory
2. Create `default` preset containing `INCLUDE_DIR` and `LIB_DIR` in `cacheVariables`.
   e.g.:
   ```json
   {
     "version": 10,
     "configurePresets": [
       {
         "name": "default",
         "cacheVariables": {
           "INCLUDE_DIR": "<path/to/glfw/include;path/to/freetype/include;path/to/glm/include>",
           "LIB_DIR": "<path/to/glfw.lib;path/to/freetyp.lib>"
            }
        }
     ]
   }
   ```

4. Run following commands:
    ```bash
    cmake --preset default .

    cmake --build .
    ```

