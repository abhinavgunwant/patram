#include <atomic>
#include <fstream>

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    #include <shellapi.h>
#endif

#ifdef __linux__
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
#endif

#include <curl/curl.h>

#include "include/http.hpp"
#include "include/logger.hpp"

CURL* curl;
std::atomic<bool> isCurlInitialized = false;

void initCURL() {
    if (!isCurlInitialized) {
        info("Initializing curl");

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();

        if (curl == nullptr) {
            error("Some error in initializing curl");
        }

        isCurlInitialized = true;
    }
}

void cleanupCURL() {
    if (isCurlInitialized) {
        curl_easy_cleanup(curl);
        curl_global_cleanup();
    }
}

void writeToBuffer(void* contents, size_t size, size_t nmemb, void* ptr) {
    ((vector<uint8_t>*)ptr)->assign((uint8_t*)contents, ((uint8_t*)contents) + size * nmemb);
}

void downloadToU8Vector(std::string url, std::vector<uint8_t>& output) {
    initCURL();

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeToBuffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

    CURLcode response = curl_easy_perform(curl);

    if (response != CURLE_OK) {
        error(curl_easy_strerror(response));
        return;
    }
}

void downloadToFile(std::string url, std::string location) {
    std::vector<uint8_t> byteBuffer;
    byteBuffer.reserve(4096);

    downloadToU8Vector(url, byteBuffer);

    std::fstream fileOut(location, std::ios::binary);
    fileOut.write((const char*)byteBuffer.data(), byteBuffer.size());
}

bool isValidURL(std::string location) {
    CURLU* u = curl_url();

    if (!u) {
        return false;
    }

    CURLUcode rc = curl_url_set(u, CURLUPART_URL, location.c_str(), 0);

    bool isUrl = rc == CURLUE_OK;

    curl_url_cleanup(u);

    return isUrl;
}

void openLink(std::string url) {
    if (isValidURL(url)) {

    #ifdef _WIN32
        ShellExecute(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    #endif

    #ifdef __linux__
        pid_t pid = fork();

        if (pid == 0) {
            execlp("xdg-open", "xdg-open", url.c_str(), nullptr);
            _exit(1);
        }
    #endif
    }
}

