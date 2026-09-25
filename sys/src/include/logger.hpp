#ifndef PATRAM_CORE_LOGGER
#define PATRAM_CORE_LOGGER

#include <cstdint>
#include <string>
#include <format>
#include <source_location>
#include <filesystem>
#include <fstream>
#include <mutex>

using namespace std;

#define tracev(fmt, ...) tracev__(source_location::current(), fmt, __VA_ARGS__)
#define debugv(fmt, ...) debugv__(source_location::current(), fmt, __VA_ARGS__)
#define infov(fmt, ...) infov__(source_location::current(), fmt, __VA_ARGS__)
#define warnv(fmt, ...) warnv__(source_location::current(), fmt, __VA_ARGS__)
#define errorv(fmt, ...) errorv__(source_location::current(), fmt, __VA_ARGS__)

enum class LoggerOutputMode : uint8_t {
    File,
    StdOut
};

enum class LogMode : uint8_t {
    Trace,
    Debug,
    Info,
    Warning,
    Error
};

extern LoggerOutputMode outputMode;
extern LogMode mode;
extern mutex logMutex;
extern ofstream loggingFile;

void initLogging();
void destroyLogging();
string logger_current_time();

void trace(string s, const source_location& location = source_location::current());
template <typename... Args>
void tracev__(const source_location& location, format_string<Args...> fmt, Args&& ...args) {
    if (mode > LogMode::Trace) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nTRACE "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << format(fmt, std::forward<Args>(args)...);
}

void debug(string s, const source_location& location = source_location::current());
template <typename... Args>
void debugv__(const source_location& location, format_string<Args...> fmt, Args&& ...args) {
    if (mode > LogMode::Debug) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nDEBUG "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << format(fmt, std::forward<Args>(args)...);
}

void info(string s, const source_location& location = source_location::current());
template <typename... Args>
void infov__(const source_location& location, format_string<Args...> fmt, Args&& ...args) {
    if (mode > LogMode::Info) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nINFO  "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << format(fmt, std::forward<Args>(args)...);
}

void warn(string s, const source_location& location = source_location::current());
template <typename... Args>
void warnv__(const source_location& location, format_string<Args...> fmt, Args&& ...args) {
    if (mode > LogMode::Warning) { return; }

    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nWARN  "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << format(fmt, std::forward<Args>(args)...);
}

void error(string s, const source_location& location = source_location::current());
template <typename... Args>
void errorv__(const source_location& location, format_string<Args...> fmt, Args&& ...args) {
    lock_guard<mutex> lock(logMutex);

    filesystem::path p(location.file_name());

    loggingFile << "\nERROR "
        << logger_current_time() << " - "
        << p.filename().string() << ":" << location.line() << ": "
        << format(fmt, std::forward<Args>(args)...);
}

#endif // PATRAM_CORE_LOGGER

