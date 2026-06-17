#include <chrono>
#include <iostream>

#include "include/logger.hpp"
#include "files.hpp"

LoggerOutputMode outputMode;
LogMode mode;
mutex logMutex;

ofstream loggingFile;

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

