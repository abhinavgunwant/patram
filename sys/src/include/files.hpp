/**
 * This handles *mostly* everything to do with files.
 */
#ifndef PATRAM_SYS_FILES
#define PATRAM_SYS_FILES

#include <string>
#include <vector>
#include <filesystem>
#include "GLFW/glfw3.h"

#include "character.hpp"

using namespace std;

// #define DEBUG_FILES

#define MAGIC_1                     0x03
#define MAGIC_2                     0xf0
#define MAGIC_3                     0x00
#define MAGIC_4                     0x00 // last word for cache index
#define MAGIC_4_FI                  0x01 // last word for font information
#define MAGIC_4_VINFO               0x02 // last word for vault information
#define MAGIC_4_VI                  0x03 // last word for vault index

#define ATLAS_DIMENSION             1024

#define CACHE_INDEX_FILE_VERSION        0
#define FONT_INFORMATION_FILE_VERSION   0
#define VAULT_INFORMATION_FILE_VERSION  0
#define VAULT_INDEX_FILE_VERSION        0

#define FONT_ATLAS_FILE_NAME_PREFIX     "fa"
#define FONT_ATLAS_FILE_NAME_EXT        ".png"
#define FONT_INFO_FILE_NAME_PREFIX      "fi"
#define FONT_ATLAS_MARGIN               2

#define CURRENT_FONT_INFO_VERSION       0

#define FILE_EXISTS(p) (filesystem::exists(p) && filesystem::is_regular_file(p))
#define DIR_EXISTS(p) (filesystem::exists(p) && filesystem::is_directory(p))

struct FileDialogResult {
    bool success;
    string filepath;
};

enum class OpenedFileType : uint8_t {
    None,
    TextFile,
    InvalidFile,
};

enum class FileDialogMode : uint8_t {
    Open,
    Save,
};

enum class VaultIndexEntryType: uint8_t {
    Folder,
    File,
};

struct VaultIndexEntry {
    VaultIndexEntryType type;

    string title;

    vector<VaultIndexEntry> children;
};

enum class LocationType {
    File,
    URL,
    Invalid,
};

filesystem::path getHomeDirectoryPath();
string getHomeDirectory();

filesystem::path getPatramDirectoryPath();
string getPatramDirectory();

filesystem::path getLogPath();

filesystem::path getCacheDirectoryPath();
string getCacheDirectory();

filesystem::path getCacheIndexPath();

string getDefaultVaultFile();

// Should precede `getFontInfoPath`
filesystem::path getFontAtlasPath(uint8_t index);
filesystem::path getFontInfoPath(uint8_t index);

filesystem::path getVaultsDirectoryPath();
string getVaultsDirectory();

filesystem::path getVaultPath(string selectedVault);

string getDefaultVault();

void setDefaultVault(string vaultName);

bool fileExists(filesystem::path p);
bool dirExists(filesystem::path p);
bool defaultVaultExists();

OpenedFileType checkFile(filesystem::path p);
vector<string> readFileText(filesystem::path p);

vector<string> getVaults();

vector<VaultIndexEntry> getVaultIndex(string vaultName);
void setVaultIndex(string vaultName, vector<VaultIndexEntry>);
vector<VaultIndexEntry> getVaultContent(string vaultName);

void createCacheDirectory();
void createCacheIndex(vector<string> fontFilePaths);
void createFontInfoFile(uint8_t index, float ascender, float descender, Character* characters, uint16_t len);
void createVaultInfoFile(string vaultName, string password);
void createVaultIndexFile(string vaultName, string password);

void renameFile(filesystem::path oldPath, filesystem::path newPath);
void deleteNode(filesystem::path p);
void createNote(filesystem::path p);
void createFolder(filesystem::path p);

bool processFontInfoFile(Character* characters, float& ascender, float& descender);

FileDialogResult openFileDialog(GLFWwindow* window, string title, FileDialogMode mode);

string downloadFileToTemp(string url);
LocationType getLocationType(string location);

#endif // PATRAM_SYS_FILES

