#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <algorithm>

#include <curl/curl.h>

#include "include/files.hpp"
#include "include/http.hpp"
#include "../lib/stb_image_write.h"

#ifdef _WIN32
    #define GLFW_EXPOSE_NATIVE_WIN32
    #define GLFW_EXPOSE_NATIVE_WGL
    #define byte byte_whatever

    #include "GLFW/glfw3native.h"
    #include <windows.h>

    #undef byte

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

string getPatramDirectory() {
    string homeDir = getHomeDirectory();
    string separator = string(1, filesystem::path::preferred_separator);

    if (!homeDir.empty()) {
        #ifdef _WIN32
            return homeDir + separator + "AppData" + separator + "Roaming" + separator + "patram";
        #endif

        #ifdef __linux__
            return homeDir + "/.local/share/patram";
        #endif
    }

    return "patram";
}

inline filesystem::path getPatramDirectoryPath() {
    return filesystem::path(getPatramDirectory());
}

filesystem::path getLogPath() { return getPatramDirectoryPath() / "logs"; }

string getCacheDirectory() {
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

string getDefaultVaultFile() {
    string separator = string(1, filesystem::path::preferred_separator);
    return getPatramDirectory() + separator + ".default";
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

inline filesystem::path getVaultsDirectoryPath() {
    return getPatramDirectoryPath() / "vaults";
}

inline string getVaultsDirectory() {
    return getPatramDirectory() + string(1, filesystem::path::preferred_separator) + "vaults";
}

filesystem::path getVaultPath(string selectedVault) {
    return getVaultsDirectoryPath() / selectedVault;
}

string getDefaultVault() {
    vector<string> dvf = readFileText(getDefaultVaultFile());

    if (!dvf.empty()) { return dvf[0]; }

    return "";
}

void setDefaultVault(string vaultName) {
    string p = getDefaultVaultFile();

    if (filesystem::exists(p) && filesystem::is_regular_file(p)) {
        ofstream file(p, ios::out | ios::trunc);
        file << vaultName;
        file.close();

        return;
    }

    ofstream file(p, ios::out);
    file << vaultName;
    file.close();
}

bool checkMagic(ifstream& f, uint8_t m1, uint8_t m2, uint8_t m3, uint8_t m4) {
    uint8_t magic[4];

    f.read((char *)&magic[0], sizeof(uint8_t));
    f.read((char *)&magic[1], sizeof(uint8_t));
    f.read((char *)&magic[2], sizeof(uint8_t));
    f.read((char *)&magic[3], sizeof(uint8_t));

    return magic[0] == m1 && magic[1] == m2 && magic[2] == m3 && magic[3] == m4;
}

void writeMagic(ofstream& f, uint8_t m1, uint8_t m2, uint8_t m3, uint8_t m4) {
    f.write((const char*)&m1, sizeof(uint8_t));
    f.write((const char*)&m2, sizeof(uint8_t));
    f.write((const char*)&m3, sizeof(uint8_t));
    f.write((const char*)&m4, sizeof(uint8_t));
}

inline bool fileExists(filesystem::path p) {
    return filesystem::exists(p) && filesystem::is_regular_file(p);
}

inline bool dirExists(filesystem::path p) {
    return filesystem::exists(p) && filesystem::is_directory(p);
}

bool defaultVaultExists() {
    string separator = string(1, filesystem::path::preferred_separator);
    filesystem::path defaultVaultFilePath = getDefaultVaultFile();

    if (filesystem::exists(defaultVaultFilePath) && filesystem::is_regular_file(defaultVaultFilePath)) {
        vector<string> fileLines = readFileText(defaultVaultFilePath);

        if (fileLines.size() == 1) {
            string vaultName = fileLines[0];

            filesystem::path defaultVaultPath = getVaultsDirectory() + separator + vaultName;

            return filesystem::exists(defaultVaultPath) && filesystem::is_directory(defaultVaultPath);
        }
    }

    return false;
}

OpenedFileType checkFile(filesystem::path p) {
    if (
        filesystem::exists(p) && (filesystem::is_regular_file(p)
        || filesystem::is_symlink(p))
    ) {
        // TODO: Check if the file is supported (extension and contents)

        return OpenedFileType::TextFile;
    }

    return OpenedFileType::InvalidFile;
}

vector<string> readFileText(filesystem::path p) {
    ifstream f(p.string());
    string line;
    vector<string> lines;
    lines.reserve(128);

    while (getline(f, line)) {
        lines.emplace_back(line);
    }

    f.close();
    return lines;
}

vector<string> getVaults() {
    vector<string> vaults;

    try {
        for (const auto& entry: filesystem::directory_iterator(getVaultsDirectory())) {
            if (entry.is_directory()) {
                vaults.push_back(entry.path().filename().string());
            }
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "\nError: " << e.what();
    }

    return vaults;
}

vector<VaultIndexEntry> readVaultIndexEntry(ifstream& f, uint32_t totalChildren, char* titleBuffer, uint32_t& titleBufferLen) {
    vector<VaultIndexEntry> entries;
    entries.reserve(totalChildren);

    for (uint32_t i=0; i<totalChildren; ++i) {
        uint16_t titleLen;
        uint8_t type;
        uint32_t childrenLen;

        f.read((char *)&titleLen, sizeof(uint16_t));

        while (titleLen > titleBufferLen) {
            titleBufferLen <<= 1;
            delete [] titleBuffer;
            titleBuffer = new char[titleBufferLen];
        }

        f.read(titleBuffer, sizeof(char) * titleLen);

        titleBuffer[titleLen] = '\0';

        f.read((char *)&type, sizeof(uint8_t));
        f.read((char *)&childrenLen, sizeof(uint32_t));

        vector<VaultIndexEntry> children;

        if (childrenLen > 0) {
            children = readVaultIndexEntry(f, childrenLen, titleBuffer, titleBufferLen);
        }

        entries.push_back({
            (VaultIndexEntryType) type,
            string(titleBuffer),
            children,
        });
    }

    return entries;
}

vector<VaultIndexEntry> getVaultIndex(string vaultName) {
    vector<VaultIndexEntry> index;

    filesystem::path vaultIndexPath = getVaultsDirectoryPath() / vaultName / ".index";

    if (!filesystem::exists(vaultIndexPath)) {
        // TODO: handle error if vault does not exist.
        return index;
    }

    ifstream f;

    f.open(vaultIndexPath.string(), ios::in | ios::binary);

    if (!checkMagic(f, MAGIC_1, MAGIC_2, MAGIC_3, MAGIC_4_VI)) {
        f.close();

        cerr << "\nMagic does not match.";
        return index;
    }

    uint16_t version = 0;
    uint32_t numRootEntries;

    f.read((char *)&version, sizeof(uint16_t));

    if (version != 0) {
        f.close();
        cerr << "\nInvalid version";
        return index;
    }

    f.read((char *)&numRootEntries, sizeof(uint32_t));

    if (numRootEntries > 0) {
        uint32_t titleBufferLen = 2048;
        char* titleBuffer = new char[titleBufferLen];
        index = readVaultIndexEntry(f, numRootEntries, titleBuffer, titleBufferLen);
        delete [] titleBuffer;
    }

    f.close();

    return index;
}

void writeVaultIndexEntries(ofstream& f, vector<VaultIndexEntry> entries) {
    for (VaultIndexEntry entry: entries) {
        uint16_t titleLen = entry.title.length();
        uint8_t type = (uint8_t) entry.type;
        char* title = (char *) entry.title.c_str();
        uint32_t childrenLen = entry.children.size();

        f.write((const char*)&titleLen, sizeof(uint16_t));
        f.write((const char*)title, sizeof(char) * titleLen);
        f.write((const char*)&type, sizeof(uint8_t));
        f.write((const char*)&childrenLen, sizeof(uint32_t));

        writeVaultIndexEntries(f, entry.children);
    }
}

void setVaultIndex(string vaultName, vector<VaultIndexEntry> index) {
    string separator = string(1, filesystem::path::preferred_separator);
    string vaultDir = getVaultsDirectory() + separator + vaultName;

    ofstream f;
    f.open(vaultDir + separator + ".index", ios::out | ios::binary);

    writeMagic(f, MAGIC_1, MAGIC_2, MAGIC_3, MAGIC_4_VI);

    uint16_t ver = VAULT_INDEX_FILE_VERSION;

    f.write((const char*)&ver, sizeof(uint16_t));

    uint32_t numRootEntries = index.size();

    f.write((const char*)&numRootEntries, sizeof(uint32_t));

    if (numRootEntries > 0) {
        writeVaultIndexEntries(f, index);
    }

    f.close();
}

vector<VaultIndexEntry> getDirectoryContent(filesystem::path dirPath) {
    vector<VaultIndexEntry> index;

    try {
        for (const auto& entry: filesystem::directory_iterator(dirPath)) {
            VaultIndexEntryType type;
            vector<VaultIndexEntry> children;

            if (entry.is_directory()) {
                type = VaultIndexEntryType::Folder;
                children = getDirectoryContent(dirPath / entry.path().filename().string());
            } else if (entry.is_regular_file()) {
                type = VaultIndexEntryType::File;
            } else {
                continue;
            }

            string filename = entry.path().filename().string();

            if (filename[0] == '.') {
                continue;
            }

            sort(children.begin(), children.end(), [](VaultIndexEntry& a, VaultIndexEntry& b) {
                if (
                    a.type == VaultIndexEntryType::Folder
                    && b.type == VaultIndexEntryType::Folder
                ) {
                    return lexicographical_compare(
                        a.title.begin(), a.title.end(),
                        b.title.begin(), b.title.end(),
                        [] (char a, char b) {
                            return tolower(a) < tolower(b);
                        }
                    );
                }

                if (a.type == VaultIndexEntryType::Folder) {
                    return true;
                }

                return false;
            });

            index.push_back({ type, filename, children });
        }
    } catch (const filesystem::filesystem_error& e) {
        cerr << "\nError: " << e.what();
    }

    sort(index.begin(), index.end(), [](VaultIndexEntry& a, VaultIndexEntry& b) {
        if (
            a.type == VaultIndexEntryType::Folder
            && b.type == VaultIndexEntryType::Folder
        ) {
            return lexicographical_compare(
                a.title.begin(), a.title.end(),
                b.title.begin(), b.title.end(),
                [] (char a, char b) {
                    return tolower(a) < tolower(b);
                }
            );
        }

        if (a.type == VaultIndexEntryType::Folder) {
            return true;
        }

        return false;
    });

    return index;
}

vector<VaultIndexEntry> getVaultContent(string vaultName) {
    filesystem::path vaultPath = getVaultsDirectoryPath() / vaultName;

    if (!filesystem::exists(vaultPath)) {
        // TODO: some way of showing this error!
        return vector<VaultIndexEntry>();
    }

    vector<VaultIndexEntry> index = getDirectoryContent(vaultPath);

    // TODO: handle error if vault does not exist.

    return index;
}

void createCacheDirectory() {
    filesystem::create_directories(getCacheDirectory());
}

void createCacheIndex(vector<string> fontFilePaths) {
    #ifdef DEBUG_FILES
        cout << "\nSTARTUP CHECK: Writing cache index";
    #endif

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
        #ifdef DEBUG_FILES
            cout << "\nSTARTUP CHECK: Font file path: " << fontFilePath << "(" << (*itr).length() << ")";
        #endif
    }

    f.close();

    #ifdef DEBUG_FILES
        cout << "\nSTARTUP CHECK: Cache Index file written in " << cacheIndex.string();
    #endif
}

void createFontInfoFile(uint8_t index, float ascender, float descender, Character* characters, uint16_t len) {
    #ifdef DEBUG_FILES
        cout << "\nSTARTUP CHECK: Writing font information";
    #endif

    filesystem::path fontInfoPath = getFontInfoPath(index);

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

    // write glyphAscender
    f.write((const char*)&ascender, sizeof(ascender));

    // write glyphDescender
    f.write((const char*)&descender, sizeof(descender));

    // write length
    f.write((const char*)&len, sizeof(len));

    for (uint16_t i=0; i<len; ++i) {
        f.write((const char*)&characters[i], sizeof(Character));
    }

    f.close();

    #ifdef DEBUG_FILES
        cout << "\nSTARTUP CHECK: Font information file written in " << fontInfoPath.string();
    #endif
}

void createFontAtlasFile(uint8_t index, Character* characters) {
    #ifdef DEBUG_FILES
        cout << "\nSTARTUP CHECK: Writing font atlas";
    #endif

    sort(chvec.begin(), chvec.end(), [](Char& a, Char& b) {
        if (a.height == b.height) {
            return a.width < b.width;
        }

        return a.height < b.height;
    });

    uint8_t* png_data = new uint8_t[ATLAS_DIMENSION * ATLAS_DIMENSION];

    memset(png_data, 0, ATLAS_DIMENSION * ATLAS_DIMENSION);

    uint16_t max_y = 0;
    uint16_t x = FONT_ATLAS_MARGIN;
    uint16_t y = FONT_ATLAS_MARGIN;

    for (vector<Char>::iterator itr = chvec.begin(); itr != chvec.end(); ++itr) {
        if ((*itr).height == 0 || (*itr).width == 0) {
            continue;
        }

        if ((*itr).height > max_y) {
            max_y = (*itr).height;
        }

        if (x + (*itr).width > ATLAS_DIMENSION) {
            y += max_y + FONT_ATLAS_MARGIN;
            x = FONT_ATLAS_MARGIN;
        }

        characters[(*itr).charcode].textureX1 = static_cast<float>(x)/static_cast<float>(ATLAS_DIMENSION);
        characters[(*itr).charcode].textureY1 = 1.0f - static_cast<float>(y)/static_cast<float>(ATLAS_DIMENSION);
        characters[(*itr).charcode].textureX2 = static_cast<float>(x + (*itr).width)/static_cast<float>(ATLAS_DIMENSION);
        characters[(*itr).charcode].textureY2 = 1.0f - static_cast<float>(y + (*itr).height)/static_cast<float>(ATLAS_DIMENSION);

        for (int i=y; i<y + (*itr).height && i<ATLAS_DIMENSION; ++i) {
            for (int j=x; j<x + (*itr).width && j<ATLAS_DIMENSION; ++j) {
                png_data[i * ATLAS_DIMENSION + j] = (*itr).bitmap[(i - y) * (*itr).width + (j - x)];
            }
        }

        x += (*itr).width + FONT_ATLAS_MARGIN;

        delete [] (*itr).bitmap;
    }

    filesystem::path fontAtlasPath = getFontAtlasPath(index);

    stbi_write_png(fontAtlasPath.string().c_str(), ATLAS_DIMENSION, ATLAS_DIMENSION, 1, png_data, ATLAS_DIMENSION);

    delete [] png_data;

    #ifdef DEBUG_FILES
        cout << "\nSTARTUP CHECK: Character atlas written in " << fontAtlasPath;
    #endif
}

void createVaultInfoFile(string vaultName, string password) {
    string separator = string(1, filesystem::path::preferred_separator);
    string vaultDir = getVaultsDirectory() + separator + vaultName;

    if (filesystem::create_directories(vaultDir)) {
        cout << "\nVault directory created!";
    } else {
        cout << "\nVault directory already exists or some error encountered!";
    }

    ofstream f;
    f.open(vaultDir + separator + ".info", ios::out | ios::binary);

    uint8_t m1 = MAGIC_1;
    uint8_t m2 = MAGIC_2;
    uint8_t m3 = MAGIC_3;
    uint8_t m4 = MAGIC_4_VINFO;

    uint16_t ver = VAULT_INFORMATION_FILE_VERSION;

    // magic bytes
    f.write((const char*)&m1, sizeof(uint8_t));
    f.write((const char*)&m2, sizeof(uint8_t));
    f.write((const char*)&m3, sizeof(uint8_t));
    f.write((const char*)&m4, sizeof(uint8_t));

    // write version
    f.write((const char*)&ver, sizeof(uint16_t));

    uint16_t algo = 0;
    uint16_t hashLen = 0;
    uint8_t* hash = nullptr;

    if (!password.empty()) {
        algo = 1;
        hashLen = 1; // TODO: update it with the actual length of hashLen
        hash = new uint8_t[16]; // TODO: update it with the actual hash

        memset(hash, 0, sizeof(hash));
    }

    // write algorithm
    f.write((const char*)&algo, sizeof(uint16_t));

    // write hashLen
    f.write((const char*)&hashLen, sizeof(uint16_t));

    if (!password.empty()) {
        // write hash
        f.write((const char*)hash, sizeof(hash));
    }

    f.close();
}

void createVaultIndexFile(string vaultName, string password) {
    string separator = string(1, filesystem::path::preferred_separator);
    string vaultDir = getVaultsDirectory() + separator + vaultName;

    ofstream f;
    f.open(vaultDir + separator + ".index", ios::out | ios::binary);

    uint8_t m1 = MAGIC_1;
    uint8_t m2 = MAGIC_2;
    uint8_t m3 = MAGIC_3;
    uint8_t m4 = MAGIC_4_VI;

    uint16_t ver = VAULT_INDEX_FILE_VERSION;

    // magic bytes
    f.write((const char*)&m1, sizeof(uint8_t));
    f.write((const char*)&m2, sizeof(uint8_t));
    f.write((const char*)&m3, sizeof(uint8_t));
    f.write((const char*)&m4, sizeof(uint8_t));

    // write version
    f.write((const char*)&ver, sizeof(uint16_t));

    uint32_t num = 0;

    f.write((const char*)&num, sizeof(uint32_t));
    f.write((const char*)&num, sizeof(uint32_t));

    f.close();
}

void renameFile(filesystem::path oldPath, filesystem::path newPath) {
    filesystem::rename(oldPath, newPath);
}

void deleteNode(filesystem::path p) {
    try {
        if (filesystem::is_directory(p)) {
            filesystem::remove_all(p);
        } else {
            filesystem::remove(p);
        }
    } catch (const filesystem::filesystem_error e) {
        cout << "\nERROR: " << e.what();
    }
}

void createNote(filesystem::path p) {
    ofstream fout(p);
    fout.close();
}

void createFolder(filesystem::path p) {
    try {
        filesystem::create_directory(p);
    } catch (const filesystem::filesystem_error e) {
        cout << "\nERROR: " << e.what();
    }
}

bool processFontInfoFile(Character* characters, float& ascender, float& descender) {
    uint8_t magic[4];
    uint16_t ver;
    uint16_t len;
    filesystem::path fontInfoPath = getFontInfoPath(0);

    ifstream f;

    f.open(fontInfoPath.string(), ios::in | ios::binary);

    // read magic words
    f.read((char *)&magic[0], sizeof(uint8_t));
    f.read((char *)&magic[1], sizeof(uint8_t));
    f.read((char *)&magic[2], sizeof(uint8_t));
    f.read((char *)&magic[3], sizeof(uint8_t));

    if (magic[0] != MAGIC_1 || magic[1] != MAGIC_2 || magic[2] != MAGIC_3 || magic[3] != MAGIC_4_FI) {
        cerr << "\nError: Invalid font info file.";
        return false;
    }

    // read version
    f.read((char *)&ver, sizeof(ver));

    if (ver != CURRENT_FONT_INFO_VERSION) {
        cerr << "\nError: Unsupported font info versoin.";
        return false;
    }

    // read ascender
    f.read((char *)&ascender, sizeof(ascender));

    // read descender
    f.read((char *)&descender, sizeof(descender));

    // read length
    f.read((char *)&len, sizeof(len));

    for (uint16_t i=0; i<len; ++i) {
        Character c;

        f.read((char *)&c, sizeof(Character));
        characters[i] = c;
    }

    f.close();

    return true;
}

FileDialogResult openFileDialog(GLFWwindow* window, string title, FileDialogMode mode) {
    bool success = false;
    string path;

    #ifdef _WIN32
        OPENFILENAME ofn = { 0 };
        char fileTitle[MAX_PATH] = { 0 };
        char selectedFile[MAX_PATH] = { 0 };

        ofn.lStructSize = sizeof(OPENFILENAME);
        ofn.hwndOwner = GetFocus();
        ofn.lpstrFile = selectedFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrFileTitle = fileTitle;
        ofn.nMaxFileTitle = MAX_PATH;
        ofn.lpstrTitle = (char*) title.c_str();

        if (mode == FileDialogMode::Open) {
            ofn.lpstrFilter = "All Files(.)\0*.*\0";
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
            success = GetOpenFileNameA(&ofn) == TRUE;

            path = ofn.lpstrFile;
        } else {
            ofn.lpstrDefExt = NULL;
            ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0";
            ofn.Flags = OFN_PATHMUSTEXIST;
            success = GetSaveFileNameA(&ofn) == TRUE;

            path = ofn.lpstrFile;

            if (success && ofn.nFileExtension == 0) {
                path += ".txt";
            }
        }

        if (!success) {
            cout << "\nError: ";
            switch (CommDlgExtendedError()) {
                case CDERR_DIALOGFAILURE: cout << "CDERR_DIALOGFAILURE"; break;
                case CDERR_FINDRESFAILURE: cout << "CDERR_FINDRESFAILURE"; break;
                case CDERR_INITIALIZATION: cout << "CDERR_INITIALIZATION"; break;
                case CDERR_LOADRESFAILURE: cout << "CDERR_LOADRESFAILURE"; break;
                case CDERR_LOADSTRFAILURE: cout << "CDERR_LOADSTRFAILURE"; break;
                case CDERR_LOCKRESFAILURE: cout << "CDERR_LOCKRESFAILURE"; break;
                case CDERR_MEMALLOCFAILURE: cout << "CDERR_MEMALLOCFAILURE"; break;
                case CDERR_MEMLOCKFAILURE: cout << "CDERR_MEMLOCKFAILURE"; break;
                case CDERR_NOHINSTANCE: cout << "CDERR_NOHINSTANCE"; break;
                case CDERR_NOHOOK: cout << "CDERR_NOHOOK"; break;
                case CDERR_NOTEMPLATE: cout << "CDERR_NOTEMPLATE"; break;
                case CDERR_STRUCTSIZE: cout << "CDERR_STRUCTSIZE"; break;
                case FNERR_BUFFERTOOSMALL: cout << "FNERR_BUFFERTOOSMALL"; break;
                case FNERR_INVALIDFILENAME: cout << "FNERR_INVALIDFILENAME"; break;
                case FNERR_SUBCLASSFAILURE: cout << "FNERR_SUBCLASSFAILURE"; break;
                default: cout << "You cancelled.";
            }
        }

        return { success, path };
    #endif

    cerr << "\nError: Opening file is not supported for this platform!";
    return { false, "" };
}

string downloadFileToTemp(string url) {
    string location = filesystem::temp_directory_path().string();

    downloadToFile(url, location);

    return location;
}

LocationType getLocationType(string location) {
    if (isValidURL(location)) {
        return LocationType::URL;
    }

    filesystem::path p(location);

    if (filesystem::exists(p)) {
        return LocationType::File;
    }

    return LocationType::Invalid;
}

