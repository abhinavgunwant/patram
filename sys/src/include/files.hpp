/**
 * This handles *mostly* everything to do with files.
 */
#ifndef PATRAM_SYS_FILES
#define PATRAM_SYS_FILES

#include <string>
#include <vector>
#include <filesystem>
#include "character.hpp"

using namespace std;

#define MAGIC_1                     0x03
#define MAGIC_2                     0xf0
#define MAGIC_3                     0x00
#define MAGIC_4                     0x00 // last word for cache index
#define MAGIC_4_FI                  0x01 // last word for font information

#define ATLAS_DIMENSION             1024

#define CACHE_INDEX_FILE_VERSION        0
#define FONT_INFORMATION_FILE_VERSION   0

#define FONT_ATLAS_FILE_NAME_PREFIX     "fa"
#define FONT_ATLAS_FILE_NAME_EXT        ".png"
#define FONT_INFO_FILE_NAME_PREFIX      "fi"
#define FONT_ATLAS_MARGIN               2

#define CURRENT_FONT_INFO_VERSION       0

#define FILE_EXISTS(p) (filesystem::exists(p) && filesystem::is_regular_file(p))
#define DIR_EXISTS(p) (filesystem::exists(p) && filesystem::is_directory(p))

filesystem::path getHomeDirectoryPath();
string getHomeDirectory();

filesystem::path getCacheDirectoryPath();
string getCacheDirectory();

filesystem::path getCacheIndexPath();

// Should precede `getFontInfoPath`
filesystem::path getFontAtlasPath(uint8_t index);
filesystem::path getFontInfoPath(uint8_t index);

bool fileExists(filesystem::path p);
bool dirExists(filesystem::path p);

void createCacheDirectory();
void createCacheIndex(vector<string> fontFilePaths);
void createFontInfoFile(uint8_t index, Character* characters, uint16_t len);
void createFontAtlasFile(uint8_t index, Character* characters);

bool processFontInfoFile(Character* characters);

#endif // PATRAM_SYS_FILES

