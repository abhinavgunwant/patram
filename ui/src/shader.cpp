#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/shader.hpp"

using namespace std;

string vertexShaders[] = {
    // Default Shader
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform vec2 resolution;\n"
    "void main() {\n"
        "vec2 uv = aPos / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
    "}\0",

    // Text Shader
    "#version 330 core\n"
    "layout (location = 0) in vec4 aPos;\n"
    "out vec2 TexCoords;\n"
    "uniform vec2 resolution;\n"
    "void main() {\n"
        "vec2 uv = aPos.xy / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
        "TexCoords = aPos.zw;\n"
    "}\0",

    // Icon Button Shader
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform vec2 resolution;\n"
    "uniform vec2 topLeft;\n"
    "void main() {\n"
        "vec2 uv = (aPos + topLeft) / resolution * 2;\n"
        "gl_Position = vec4(uv.x - 1, 1 - uv.y, 0.0f, 1.0f);\n"
    "}\0",
};

string fragmentShaders[] = {
    // Default Shader
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 solidColor;\n"
    "uniform float time;\n"
    "void main() {\n"
        "FragColor = solidColor;\n"
    "}\0",

    // Text Shader
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D text;\n"
    "uniform vec3 textColor;\n"
    "void main() {\n"
        "vec4 sampled = vec4(1, 1, 1, texture(text, TexCoords).r);\n"
        "color = vec4(textColor, 1) * sampled;\n"
        // "color = vec4(1.0, 0, 0, 1.0);\n"
    "}\0",

    // Icon Button Shader
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 solidColor;\n"
    "uniform vec4 hoverColor;\n"
    "uniform float time;\n"
    "uniform float width;\n"
    "uniform float height;\n"
    "uniform vec2 topLeft;\n"
    "uniform vec2 resolution;\n"
    "uniform vec2 cursor;\n"
    "void main() {\n"
        "vec2 uv = gl_FragCoord.xy;\n"
        "uv.y = resolution.y - uv.y;\n"
        "float width_2 = width/2;\n"
        "vec2 mid = topLeft;\n"
        "mid.x += width_2;\n"
        "mid.y += height/2;\n"

        "float d_uv_mid = distance(uv, mid);\n"

        "if (distance(uv, mid) > width_2) {\n"
            "FragColor = vec4(0);\n"
            "return;\n"
        "}\n"

        "const float PI_2 = 1.5708;\n"
        "const float PI_4 = 0.7854;"

        "float d_cursor_uv = distance(cursor, uv);\n"

        "if (d_cursor_uv > width * 1.5) {\n"
            "FragColor = solidColor;\n"
            "return;\n"
        "}\n"

        "float radians = d_cursor_uv / (width * 1.5) * 0.9;\n"

        "FragColor = hoverColor * cos(radians);\n"
    "}\0",
};

Shader shaders[] = {
    { 0, 0, 0 },
    { 0, 1, 1 },
    { 0, 2, 2 },
};

uint32_t compile(uint8_t index, uint8_t vs, uint8_t fs) {
    cout << "\nCompiling shaders (" << (int)index + 1 << " out of " << TOTAL_SHADERS << ")";
    uint32_t vShaderID = glCreateShader(GL_VERTEX_SHADER);

    if (vShaderID == 0) {
        cerr << "\nCould not create vertex shader " << (int)index;
    }

    int success;

    const char * vsCode = vertexShaders[(int)vs].c_str();
    const char * fsCode = fragmentShaders[(int)fs].c_str();

    // cout << "\nVertex shader code ("<< (int) vs << "): \n" << vsCode << "\n";
    // cout << "\nFragment shader code (// " << (int) fs << "): \n" << fsCode << "\n";

	glShaderSource(vShaderID, 1, (char const* const*)&vsCode, NULL);
	glCompileShader(vShaderID);
	glGetShaderiv(vShaderID, GL_COMPILE_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetShaderInfoLog(vShaderID, 1024, NULL, log);
		cerr << "\nError: vertexShader(" << vShaderID << ") index " << (int)index
            << "'s compilation failed:\n" << log;

        return 0;
	}

    // cout << "\ncompiled vertex shader";

    uint32_t fShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    if (fShaderID == 0) {
        cerr << "\nCould not create fragment shader " << (int)index;
    }

	glShaderSource(fShaderID, 1, (char const* const*)&fsCode, NULL);
	glCompileShader(fShaderID);
	glGetShaderiv(fShaderID, GL_COMPILE_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetShaderInfoLog(fShaderID, 1024, NULL, log);
		cerr << "\nError: fragmentShader(" << fShaderID << ", index: " << (int)index
            << ")'s compilation failed:\n" << log;

        return 0;
	}

    // cout << "\ncompiled fragment shader";

	uint32_t programID = glCreateProgram();

	glAttachShader(programID, vShaderID);
	glAttachShader(programID, fShaderID);

	glLinkProgram(programID);

	// cout << "\nLinking... Program ID: " << programID;

	glGetProgramiv(programID, GL_LINK_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetProgramInfoLog(programID, 1024, NULL, log);
		cerr << "\nError while linking shader program:\n" << log;
	}

	glDeleteShader(vShaderID);
	glDeleteShader(fShaderID);

    return programID;
}

void initShaders() {
    for (uint8_t i=0; i<TOTAL_SHADERS; ++i) {
        shaders[i].programId = compile(i, shaders[i].vsIndex, shaders[i].fsIndex);
    }
}

