#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "include/shader.hpp"

using namespace std;

string vertexShaders[] = {
    "#version 330 core\n"
    "layout (location = 0) in vec2 aPos;\n"
    "uniform vec2 resolution;\n"
    "void main() {\n"
        "vec2 uv = aPos / resolution;"
        "gl_Position = vec4(uv.x * 2 - 1, uv.y * (-2) + 1, 0.0f, 1.0f);\n"
    "}\0"
};

string fragmentShaders[] = {
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "uniform vec4 solidColor;\n"
    "uniform float time;\n"
    "void main() {\n"
        "FragColor = solidColor;\n"
        // "FragColor = solidColor * sin(time * 10.0f);\n"
    "}\0"
};

Shader shaders[] = {
    { 0, 0, 0 },
};

uint32_t compile(uint8_t index, uint8_t vs, uint8_t fs) {
    cout << "\nCompiling shaders (" << (int)index + 1 << " out of " << TOTAL_SHADERS << ")";
    uint32_t vShaderID = glCreateShader(GL_VERTEX_SHADER);

    if (vShaderID == 0) {
        cerr << "\nCould not create vertex shader " << index;
    }

    int success;

    const char * vsCode = vertexShaders[vs].c_str();
    const char * fsCode = fragmentShaders[fs].c_str();

    cout << "\nVertex shader code: \n" << vsCode << "\n";
    cout << "\nFragment shader code: \n" << fsCode << "\n";

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

    cout << "\ncompiled vertex shader";

    uint32_t fShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    if (fShaderID == 0) {
        cerr << "\nCould not create fragment shader " << index;
    }

	glShaderSource(fShaderID, 1, (char const* const*)&fsCode, NULL);
	glCompileShader(fShaderID);
	glGetShaderiv(fShaderID, GL_COMPILE_STATUS, &success);

	if (!success) {
		char log[1024];
		glGetShaderInfoLog(fShaderID, 1024, NULL, log);
		cerr << "\nError: fragmentShader(" << fShaderID << ", index: " << index
            << ")'s compilation failed:\n" << log;

        return 0;
	}

    cout << "\ncompiled fragment shader";

	uint32_t programID = glCreateProgram();

	glAttachShader(programID, vShaderID);
	glAttachShader(programID, fShaderID);

	glLinkProgram(programID);

	cout << "\nLinking... Program ID: " << programID;

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
        shaders->programId = compile(i, shaders->vsIndex, shaders->fsIndex);
    }
}

