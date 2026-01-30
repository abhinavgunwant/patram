#include <stdlib.h>
#include <string>
#include <iostream>
#include <filesystem>

#include "include/files.hpp"

#ifdef _WIN32
    #include <winrt/windows.ui.viewmanagement.h>
    using namespace winrt::Windows::UI::ViewManagement;
#endif

using namespace std;

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

std::string getCacheDirectory() {
    string home = getHomeDirectory();

    if (!home.empty()) {
        #ifdef _WIN32
            return home + "\\AppData\\Local\\patram\\cache";
        #endif

        #ifdef __linux__
            return home + "/.local/share/patram/cache";
        #endif
    }

    return ".cache";
}

bool cacheDirectoryExists() {
    filesystem::path cachePath(getCacheDirectory());

    return filesystem::exists(cachePath) && filesystem::is_directory(cachePath);
}

void createCacheDirectory() {
    filesystem::create_directories(getCacheDirectory());
}


