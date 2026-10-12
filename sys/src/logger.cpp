#include <chrono>
#include <iostream>
#include <thread>
#include <ctime>
#include <filesystem>
#include <algorithm>

#include "include/logger.hpp"
#include "files.hpp"

LoggerOutputMode outputMode;
LogMode mode;
mutex logMutex;

ofstream loggingFile;

struct FileTime {
    std::filesystem::file_time_type time;
    string path;
    string name;
};

bool sortFunc(const FileTime& a, const FileTime& b) { return a.time > b.time; }

void manageLogFiles() {
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    vector<FileTime> ft;
    ft.reserve(128);

    for (const auto entry: filesystem::directory_iterator(getLogPath())) {
        ft.push_back(FileTime { entry.last_write_time(), entry.path().string(), entry.path().filename().string() });
    }

    if (ft.size() > 20) {
        uint32_t files = ft.size() - 16;
        info("Cleaning up old log files (" + to_string(files) + " total).");

        std::sort(ft.begin(), ft.end(), sortFunc);

        vector<string> cleanupList;
        cleanupList.reserve(files);

        for (
            vector<FileTime>::iterator i = ft.begin() + 16;
            i != ft.end();
            ++i
        ) {
            if (std::filesystem::remove(i->path)) {
                debug("Deleted log file: " + i->name);
            } else {
                error("Could not delete log file: " + i->name);
            }
        }

        info("Finished log file cleanup");
    } else {
        debug("No log files to clean");
    }
}

void initLogging() {
    outputMode = LoggerOutputMode::File;
    mode = LogMode::Debug;

    filesystem::path loggingPath = getLogPath();

    if (!filesystem::exists(loggingPath)) {
        filesystem::create_directory(loggingPath);
    }

    loggingPath /= "patram.log";

    if (filesystem::exists(loggingPath)) {
        error_code ec;
        auto fileTime = filesystem::last_write_time(loggingPath, ec);

        if (ec) {
            cerr << "\nERROR: " << ec.message();
        } else {
            auto systemTime = chrono::clock_cast<chrono::system_clock>(fileTime);
            auto local = chrono::current_zone()->to_local(systemTime);
            string formattedTime = format("{:%Y-%m-%d-%H-%M-%S}", local);

            filesystem::path destinationPath = getLogPath()
                / format("patram.{}.log", formattedTime);

            copy(loggingPath, destinationPath);
        }
    }

    loggingFile.open(loggingPath, ios::trunc | ios::out);
    loggingFile << "patram v0.1.0 logs";

    info("Startup");

    jthread logWorker(manageLogFiles);
    logWorker.detach();
}

void destroyLogging() {
    loggingFile.close();
}

string logger_current_time() {
    auto now = chrono::system_clock::now();
    auto local = chrono::current_zone()->to_local(now);
    return format("{:%Y-%m-%d %H:%M:%S}", local);
}

void trace(string s, const source_location& location) {
    if (mode > LogMode::Trace) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nTRACE "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << s;
}

void debug(string s, const source_location& location) {
    if (mode > LogMode::Debug) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nDEBUG "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << s;
}

void info(string s, const source_location& location) {
    if (mode > LogMode::Info) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nINFO  "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << s;
}

void warn(string s, const source_location& location) {
    if (mode > LogMode::Warning) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nWARN  "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << s;
}


void error(string s, const source_location& location) {
    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nERROR "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << s << flush;
}

