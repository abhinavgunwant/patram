#include "include/text.hpp"

#include <iostream>
#include <cstdlib>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "include/shader.hpp"

using namespace std;

Character characters[MAX_CHARACTERS];
Shader textShader;
unsigned int textVAO, textVBO;

void initCharGlyph(FT_Face& face, uint16_t c) {
	if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
		std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
		return;
	}

	unsigned int texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(
		GL_TEXTURE_2D,
		0,
		GL_RED,
		face->glyph->bitmap.width,
		face->glyph->bitmap.rows,
		0,
		GL_RED,
		GL_UNSIGNED_BYTE,
		face->glyph->bitmap.buffer
	);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	characters[c] = {
		glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
		glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
		texture,
		static_cast<unsigned int>(face->glyph->advance.x)
	};
}

void initFonts() {
	FT_Library ft;

	if (FT_Init_FreeType(&ft)) {
		std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return;
	}

	// load font as face
	FT_Face face;

	// if (FT_New_Face(ft, "assets/fonts/inter/Inter-Regular.ttf", 0, &face)) {
	if (FT_New_Face(ft, "assets/fonts/poppins/Poppins-Regular.ttf", 0, &face)) {
		std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
		return;
	}

	FT_Select_Charmap(face, FT_ENCODING_UNICODE);

	// set size to load glyphs as
	FT_Set_Pixel_Sizes(face, 0, 48);

	// disable byte-alignment restriction
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	cout << "\nTotal characters: " << face->num_glyphs;

	uint32_t charcode;
	uint32_t id;

	charcode = FT_Get_First_Char(face, &id);

	// for (uint16_t i = 1; i < face->num_glyphs; i++) {
	while (id != 0) {
		// cout << "\n" << charcode;
		initCharGlyph(face, charcode);
		charcode = FT_Get_Next_Char(face, charcode, &id);
	}

	glBindTexture(GL_TEXTURE_2D, 0);

	FT_Done_Face(face);
	FT_Done_FreeType(ft);
}

void initTextRendering(uint16_t width, uint16_t height) {
    int32_t pid = shaders[SHADER_TEXT].programId;
    glUseProgram(pid);

    glUniform2f(
        glGetUniformLocation(pid, "resolution"),
        static_cast<float>(width),
        static_cast<float>(height)
    );

	initFonts();

	glGenVertexArrays(1, &textVAO);
	glGenBuffers(1, &textVBO);
	glBindVertexArray(textVAO);
	glBindBuffer(GL_ARRAY_BUFFER, textVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

// render line of text
void renderText(string text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color) {
    int32_t pid = shaders[SHADER_TEXT].programId;

    glUseProgram(pid);

	glUniform3f(
        glGetUniformLocation(pid, "textColor"),
        color.r,
        color.g,
        color.b
    );
    glUniform2f(glGetUniformLocation(pid, "resolution"), res.x, res.y);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	// iterate through all characters
	for (const char c : text) {
		Character ch = characters[(int)c];

		float xpos = pos.x + ch.bearing.x;
		float ypos = pos.y - (ch.size.y - ch.bearing.y);

		float xpos_plus_w = xpos + ch.size.x;
		float ypos_plus_h = ypos - ch.size.y;
		// update textVBO for each character
		float vertices[6][4] = {
			{ xpos,         ypos_plus_h,    0.0f, 0.0f },
			{ xpos,         ypos,           0.0f, 1.0f },
			{ xpos_plus_w,  ypos,           1.0f, 1.0f },
			{ xpos,         ypos_plus_h,    0.0f, 0.0f },
			{ xpos_plus_w,  ypos,           1.0f, 1.0f },
			{ xpos_plus_w,  ypos_plus_h,    1.0f, 0.0f },
		};
		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		// update content of textVBO memory
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		pos.x += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

// render line of text
void renderText(wstring text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color) {
    int32_t pid = shaders[SHADER_TEXT].programId;

    glUseProgram(pid);

	glUniform3f(glGetUniformLocation(pid, "textColor"), color.r, color.g, color.b);
    glUniform2f(glGetUniformLocation(pid, "resolution"), res.x, res.y);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	// iterate through all characters
	for (const wchar_t c : text) {
		Character ch = characters[(int)c];

		float xpos = pos.x + ch.bearing.x;
		float ypos = pos.y - (ch.size.y - ch.bearing.y);

		float xpos_plus_w = xpos - ch.size.x; // xpos + w
		float ypos_plus_h = ypos + ch.size.y; // ypos + h
		// update textVBO for each character
		float vertices[6][4] = {
			{ xpos,         ypos_plus_h,    0.0f, 0.0f },
			{ xpos,         ypos,           0.0f, 1.0f },
			{ xpos_plus_w,  ypos,           1.0f, 1.0f },
			{ xpos,         ypos_plus_h,    0.0f, 0.0f },
			{ xpos_plus_w,  ypos,           1.0f, 1.0f },
			{ xpos_plus_w,  ypos_plus_h,    1.0f, 0.0f },
		};
		// render glyph texture over quad
		glBindTexture(GL_TEXTURE_2D, ch.textureID);
		// update content of textVBO memory
		glBindBuffer(GL_ARRAY_BUFFER, textVBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		// render quad
		glDrawArrays(GL_TRIANGLES, 0, 6);
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		pos.x += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

