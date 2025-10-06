/**
 * Inspired by: https://learnopengl.com/In-Practice/Text-Rendering
 **/

#ifndef PATRAM_TEXT
#define PATRAM_TEXT

#include <string>

#include <glm/glm.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define MAX_CHARACTERS 65536

using namespace std;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
	glm::ivec2   size;      // Size of glyph
	glm::ivec2   bearing;   // Offset from baseline to left/top of glyph
	unsigned int textureID; // ID handle of the glyph texture
	unsigned int advance;   // Horizontal offset to advance to next glyph
};

extern Character characters[MAX_CHARACTERS];
extern unsigned int textVAO, textVBO;

void initCharGlyph(FT_Face& face, uint16_t c);
void initFonts();
void initTextRendering(uint16_t width, uint16_t height);
void renderText(string text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color = glm::fvec3(1.0, 1.0, 1.0));
void renderText(wstring text, glm::vec2 pos, glm::vec2& res, glm::fvec3 color = glm::fvec3(1.0, 1.0, 1.0));

#endif // PATRAM_TEXT

