#ifndef PATRAM_SYS_UTILS
#define PATRAM_SYS_UTILS

#include <string>

using namespace std;

enum class ClipboardResponseType {
    Ok,
    Unsupported,
    Error,
};

struct ClipboardResponse {
    ClipboardResponseType respType;
    string str;
    string errorMsg;
};

/**
 * Detects system dark mode.
 */
bool isDarkMode();
ClipboardResponse copyToClipboard(string text);
ClipboardResponse pasteFromClipboard();
string getClipboardError(ClipboardResponse cr);

#endif // PATRAM_SYS_UTILS

