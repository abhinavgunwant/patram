#include "include/text.hpp"

#include <cctype>
#include <iostream>
#include <cstdlib>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "include/shader.hpp"
#include "include/ui.hpp"

using namespace std;

#ifdef DEBUG_TEXT
bool debugTextFirstRenderPrint = true;
#endif

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

    #ifdef DEBUG_TEXT
        if (debugTextFirstRenderPrint) {
            if (c > 64 && c < 90 || c > 96 && c < 123) {
                cout << "\nchar: " << (char)c << "; width: " << face->glyph->bitmap.width
                << " height: " << face->glyph->bitmap.rows
                << " bearingLeft: " << face->glyph->bitmap_left
                << " bearingTop: " << face->glyph->bitmap_top
                << " textureID: " << texture
                << " advance: " << face->glyph->advance.x;
            }
        }
    #endif

	characters[c] = {
        static_cast<uint8_t>(face->glyph->bitmap.width),
		static_cast<uint8_t>(face->glyph->bitmap.rows),
		static_cast<int8_t>(face->glyph->bitmap_left),
        static_cast<uint8_t>(face->glyph->bitmap_top),
		static_cast<uint16_t>(texture),
		static_cast<uint16_t>(face->glyph->advance.x)
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
	FT_Set_Pixel_Sizes(face, 0, DEFAULT_FONT_SIZE);

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
glm::vec4 renderText(
    string text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color,
    uint8_t fontSize, UIElement* parent
) {
    glm::vec4 dimensions = glm::vec4(pos.x, pos.y, 0, 0); // x, y, height, width

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

    #ifdef DEBUG_TEXT
    if (debugTextFirstRenderPrint) {
        cout << "\nRendering text: " << text;
    }
    #endif

    float xpos = pos.x;
    float yNet = pos.y;

	// iterate through all characters
	for (const char c : text) {
		Character ch = characters[(int)c];

        #ifdef DEBUG_TEXT
        if (debugTextFirstRenderPrint) {
            cout << "\n--> Rendering character: '" << (char) c << "'";
        }
        #endif

        float aspectRatio = static_cast<float>(ch.width) / static_cast<float>(ch.height);

        float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;

        float glyphHeight = static_cast<float>(ch.height) * fontHeightRatio;
        float glyphWidth = glyphHeight * aspectRatio;

        float widthRatio = glyphWidth / ch.width;
        float heightRatio = glyphHeight / ch.height;

        float bearingTop = ch.bearingTop * heightRatio;
        float bearingLeft = ch.bearingLeft * widthRatio;

        if (bearingLeft != bearingLeft) {
            bearingLeft = 0.0f;
        }

        if (bearingTop != bearingTop) {
            bearingTop = 0.0f;
        }

		xpos += bearingLeft;

        if (
            parent != nullptr
            && (
                parent->width < 800.0f
                && xpos + glyphWidth > parent->width - CONTENT_HORIZONTAL_MARGIN
                || xpos + glyphWidth > contentRight
            )
        ) {
            yNet += fontSize + 4.0f;
            xpos = pos.x;
        }

        float ypos = yNet + glyphHeight - bearingTop + fontSize;

        dimensions.z = ypos - pos.y;

		float xpos_plus_w = xpos + glyphWidth;
		float ypos_plus_h = ypos - glyphHeight; // ypos + h

        #ifdef DEBUG_TEXT
        if (debugTextFirstRenderPrint) {
            cout << "; x: " << xpos << " y: " << ypos << " x + w: "
                << xpos_plus_w << " y + h: " << ypos_plus_h
                << " bearingLeft: " << bearingLeft
                << " bearingTop: " << bearingTop
                << " widthRatio: " << widthRatio
                << " heightRatio: " << heightRatio;
        }
        #endif

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
		xpos += (static_cast<uint32_t>(ch.advance * fontHeightRatio) >> 6) - bearingLeft; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
	}

	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);

    dimensions.w = pos.x;

    return dimensions;
}

// render line of text
// TODO: update to be consistent with above function.
glm::vec4 renderText(
    wstring text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color,
    uint8_t fontSize, UIElement* parent
) {
    glm::vec4 dimensions = glm::vec4(pos.x, pos.y, 0, 0);

    int32_t pid = shaders[SHADER_TEXT].programId;

    glUseProgram(pid);

	glUniform3f(glGetUniformLocation(pid, "textColor"), color.r, color.g, color.b);
    glUniform2f(glGetUniformLocation(pid, "resolution"), res.x, res.y);
	glActiveTexture(GL_TEXTURE0);
	glBindVertexArray(textVAO);

	// iterate through all characters
	for (const wchar_t c : text) {
		Character ch = characters[(int)c];

		float xpos = pos.x + ch.bearingLeft;
		float ypos = pos.y - (ch.height - ch.bearingTop);

		float xpos_plus_w = xpos - ch.width; // xpos + w
		float ypos_plus_h = ypos + ch.height; // ypos + h
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

    return dimensions;
}

GlyphDimensions getGlyphDimensions(char c, float x, float y, glm::vec2& res, uint8_t fontSize) {
    Character ch = characters[(int)c];

    float aspectRatio = static_cast<float>(ch.width) / static_cast<float>(ch.height);

    float fontHeightRatio = static_cast<float>(fontSize) / DEFAULT_FONT_SIZE;

    float glyphHeight = static_cast<float>(ch.height) * fontHeightRatio;
    float glyphWidth = glyphHeight * aspectRatio;

    float widthRatio = glyphWidth / ch.width;
    float heightRatio = glyphHeight / ch.height;

    float bearingTop = ch.bearingTop * heightRatio;
    float bearingLeft = ch.bearingLeft * widthRatio;

    if (isnan(bearingLeft)) {
        bearingLeft = 0;
    }

    float xpos = x + bearingLeft;
    // float ypos = y + (glyphHeight - bearingTop) + fontSize;
    float ypos = y + glyphHeight - bearingTop + 0.75*fontSize;

    // float xpos_plus_w = xpos + glyphWidth;
    // float ypos_plus_h = ypos - glyphHeight; // ypos + h

    #ifdef DEBUG_TEXT
    if (debugTextFirstRenderPrint) {
     cout << "\nRendering glyph: " << c
        << "\n--> Rendering character: '" << (char) c
        << "'; x: " << xpos << " y: " << ypos << " x + w: "
        << xpos + glyphWidth << " y + h: " << ypos - glyphHeight
        << " bearingLeft: " << bearingLeft << "(" << ch.bearingLeft << ", " <<  widthRatio << ")"
        << " bearingTop: " << bearingTop
        << " widthRatio: " << widthRatio
        << " heightRatio: " << heightRatio;
    }
    #endif


    return GlyphDimensions {
        .leftX = xpos,
        .rightX = xpos + glyphWidth,
        .bottomY = ypos,
        .topY = ypos - glyphHeight,
        .scale = fontHeightRatio,
        .advance = ch.advance,
        .textureID = ch.textureID,
    };
}

uint32_t renderGlyph(char c, float x, float y, glm::vec2& res, uint8_t fontSize) {
    GlyphDimensions gd = getGlyphDimensions(c, x, y, res, fontSize);

    // update textVBO for each character
    // float vertices[4][4] = {
    //     { xpos,         ypos,           0.0f, 1.0f },
    //     { xpos_plus_w,  ypos,           1.0f, 1.0f },
    //     { xpos,         ypos_plus_h,    0.0f, 0.0f },
    //     { xpos_plus_w,  ypos_plus_h,    1.0f, 0.0f },
    // };
    float vertices[4][4] = {
        { gd.leftX,   gd.bottomY, 0.0f, 1.0f },
        { gd.rightX,  gd.bottomY, 1.0f, 1.0f },
        { gd.leftX,   gd.topY,    0.0f, 0.0f },
        { gd.rightX,  gd.topY,    1.0f, 0.0f },
    };

    // render glyph texture over quad
    glBindTexture(GL_TEXTURE_2D, gd.textureID);
    // update content of textVBO memory
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

    // render quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
    return (static_cast<uint32_t>(gd.advance * gd.scale) >> 6); // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
}

