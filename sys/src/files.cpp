#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include "include/files.hpp"
#include "../lib/stb_image_write.h"

#ifdef _WIN32
    #include <winrt/windows.ui.viewmanagement.h>
    using namespace winrt::Windows::UI::ViewManagement;
#endif

using namespace std;

vector<Char> chvec;

string getHomeDirectory() {
    char* profile;
    size_t len;

    #ifdef _WIN32
        errno_t err = _dupenv_s(&profile, &len, "USERPROFILE");

        if (err == 0 && profile != nullptr) {
            return string(profile);
        } else {
            cerr << "\nErro getting the USERPROFILE";
        }
    #endif

    #ifdef __linux__
        errno_t err = _dupenv_s(&profile, &len, "HOME");

        if (err == 0 && profile != nullptr) {
            return string(profile);
        } else {
            cerr << "\nErro getting the USERPROFILE";
        }
    #endif

    return "";
}

inline filesystem::path getHomeDirectoryPath() {
    return filesystem::path(getHomeDirectory());
}

std::string getCacheDirectory() {
    string home = getHomeDirectory();

    if (!home.empty()) {
        #ifdef _WIN32
            return home + "\\AppData\\Roaming\\patram\\cache";
        #endif

        #ifdef __linux__
            return home + "/.local/share/patram/cache";
        #endif
    }

    return ".cache";
}

inline filesystem::path getCacheIndexPath() {
    return getCacheDirectoryPath() / "index";
}

inline filesystem::path getCacheDirectoryPath() {
    return filesystem::path(getCacheDirectory());
}

inline filesystem::path getFontAtlasPath(uint8_t index) {
    return getCacheDirectoryPath() / (FONT_ATLAS_FILE_NAME_PREFIX + to_string(static_cast<int>(index)) + FONT_ATLAS_FILE_NAME_EXT);
}

inline filesystem::path getFontInfoPath(uint8_t index) {
    return getCacheDirectoryPath() / (FONT_INFO_FILE_NAME_PREFIX + to_string(static_cast<int>(index)));
}

inline bool fileExists(filesystem::path p) {
    return filesystem::exists(p) && filesystem::is_regular_file(p);
}

inline bool dirExists(filesystem::path p) {
    return filesystem::exists(p) && filesystem::is_directory(p);
}

void createCacheDirectory() {
    filesystem::create_directories(getCacheDirectory());
}

void createCacheIndex(vector<string> fontFilePaths) {
    cout << "\nSTARTUP CHECK: Writing cache index";

    filesystem::path cacheIndex = getCacheIndexPath();

    ofstream f;
    f.open(cacheIndex.string().c_str(), ios::out | ios::binary);

    uint8_t m1 = MAGIC_1;
    uint8_t m2 = MAGIC_2;
    uint8_t m3 = MAGIC_3;
    uint8_t m4 = MAGIC_4;
    uint16_t ver = CACHE_INDEX_FILE_VERSION;
    uint32_t len = fontFilePaths.size();

    // magic bytes
    f.write((const char*)&m1, sizeof(uint8_t));
    f.write((const char*)&m2, sizeof(uint8_t));
    f.write((const char*)&m3, sizeof(uint8_t));
    f.write((const char*)&m4, sizeof(uint8_t));

    // write version
    f.write((const char*)&ver, sizeof(uint16_t));

    // write length
    f.write((const char*)&len, sizeof(len));

    // write data
    uint8_t index = 0;

    for (vector<string>::iterator itr=fontFilePaths.begin(); itr != fontFilePaths.end(); ++itr, ++index) {
        uint16_t len = (*itr).length();

        f.write((const char*)&index, sizeof(index));
        f.write((const char*)&len, sizeof(len));
        const char * fontFilePath = (*itr).c_str();

        f.write(fontFilePath, (*itr).length());
        cout << "\nSTARTUP CHECK: Font file path: " << fontFilePath << "(" << (*itr).length() << ")";
    }

    f.close();

    cout << "\nSTARTUP CHECK: Cache Index file written in " << cacheIndex.string();
}

void createFontInfoFile(uint8_t index, Character* characters, uint16_t len) {
    cout << "\nSTARTUP CHECK: Writing font information";

    filesystem::path fontInfoPath = getFontInfoPath(index);
    // fontInfoPath /= "fi" + to_string((int)index);

    ofstream f;
    f.open(fontInfoPath.string().c_str(), ios::out | ios::binary);

    uint8_t m1 = MAGIC_1;
    uint8_t m2 = MAGIC_2;
    uint8_t m3 = MAGIC_3;
    uint8_t m4 = MAGIC_4_FI;
    uint16_t ver = FONT_INFORMATION_FILE_VERSION;

    // magic bytes
    f.write((const char*)&m1, sizeof(uint8_t));
    f.write((const char*)&m2, sizeof(uint8_t));
    f.write((const char*)&m3, sizeof(uint8_t));
    f.write((const char*)&m4, sizeof(uint8_t));

    // write version
    f.write((const char*)&ver, sizeof(uint16_t));

    // write length
    f.write((const char*)&len, sizeof(len));

    for (uint16_t i=0; i<len; ++i) {
        f.write((const char*)&characters[i], sizeof(Character));
    }

    f.close();

    cout << "\nSTARTUP CHECK: Font information file written in " << fontInfoPath.string();
}

void createFontAtlasFile(uint8_t index, Character* characters) {
    cout << "\nSTARTUP CHECK: Writing font atlas";

    std::sort(chvec.begin(), chvec.end(), [](Char& a, Char& b) {
        if (a.height == b.height) {
            return a.width < b.width;
        }

        return a.height < b.height;
    });

    uint8_t* png_data = new uint8_t[ATLAS_DIMENSION * ATLAS_DIMENSION];

    for (uint32_t i=0; i<ATLAS_DIMENSION * ATLAS_DIMENSION; ++i) {
        png_data[i] = 0;
    }

    uint16_t max_y = 0;
    uint16_t x = 0;
    uint16_t y = 0;

    for (vector<Char>::iterator itr = chvec.begin(); itr != chvec.end(); ++itr) {
        if ((*itr).height == 0 || (*itr).width == 0) {
            continue;
        }

        if ((*itr).height > max_y) {
            max_y = (*itr).height;
        }

        if (x + (*itr).width > ATLAS_DIMENSION) {
            y += max_y + 1;
            x = 0;
        }

        // cout << "\nCharcode: " << (*itr).charcode << " x: " << x << " y: " << y;
        characters[(*itr).charcode].textureX1 = static_cast<float>(x)/static_cast<float>(ATLAS_DIMENSION);
        characters[(*itr).charcode].textureY1 = 1.0f - static_cast<float>(y)/static_cast<float>(ATLAS_DIMENSION);
        characters[(*itr).charcode].textureX2 = static_cast<float>(x + (*itr).width)/static_cast<float>(ATLAS_DIMENSION);
        characters[(*itr).charcode].textureY2 = 1.0f - static_cast<float>(y + (*itr).height)/static_cast<float>(ATLAS_DIMENSION);

        for (int i=y; i<y + (*itr).height && i<ATLAS_DIMENSION; ++i) {
            for (int j=x; j<x + (*itr).width && j<ATLAS_DIMENSION; ++j) {
                png_data[i * ATLAS_DIMENSION + j] = (*itr).bitmap[(i - y) * (*itr).width + (j - x)];
            }
        }

        x += (*itr).width + 1;

        delete [] (*itr).bitmap;
    }

    filesystem::path fontAtlasPath = getFontAtlasPath(index);

    stbi_write_png(fontAtlasPath.string().c_str(), ATLAS_DIMENSION, ATLAS_DIMENSION, 1, png_data, ATLAS_DIMENSION);

    delete [] png_data;

    cout << "\nSTARTUP CHECK: Character atlas written in " << fontAtlasPath;
}

bool processFontInfoFile(Character* characters) {
    uint8_t magic[4];
    uint16_t ver;
    uint16_t len;
    filesystem::path fontInfoPath = getFontInfoPath(0);

    ifstream f;

    f.open(fontInfoPath.string(), ios::in | ios::binary);

    f.read((char *)&magic[0], sizeof(uint8_t));
    f.read((char *)&magic[1], sizeof(uint8_t));
    f.read((char *)&magic[2], sizeof(uint8_t));
    f.read((char *)&magic[3], sizeof(uint8_t));

    if (magic[0] != MAGIC_1 || magic[1] != MAGIC_2 || magic[2] != MAGIC_3 || magic[3] != MAGIC_4_FI) {
        cerr << "\nSTARTUP CHECK: Invalid font info file.";
        return false;
    }

    f.read((char *)&ver, sizeof(ver));

    if (ver != CURRENT_FONT_INFO_VERSION) {
        cerr << "\nSTARTUP CHECK: Unsupported font info versoin.";
        return false;
    }

    f.read((char *)&len, sizeof(len));

    for (uint16_t i=0; i<len; ++i) {
        Character c;

        f.read((char *)&c, sizeof(Character));
        characters[i] = c;
    }

    f.close();

    return true;
}

