#include "include/utils.hpp"

#ifdef _WIN32
    #include <winrt/windows.ui.viewmanagement.h>
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
    using namespace winrt::Windows::UI::ViewManagement;
#endif

using namespace std;

bool isDarkMode() {
    #ifdef _WIN32
         auto settings = UISettings();
         auto foreground = settings.GetColorValue(UIColorType::Foreground);

         return (5 * foreground.G) + (2 * foreground.R) + foreground.B > 1024;
    #endif

    // TODO: Implement for other platforms
    return false;
}

ClipboardResponse copyToClipboard(string text) {
    #ifdef _WIN32
        if (!OpenClipboard(nullptr)) {
            return ClipboardResponse {
                ClipboardResponseType::Error,
                "",
                "Error opening clipboard."
            };
        }

        if (!EmptyClipboard()) {
            return ClipboardResponse {
                ClipboardResponseType::Error,
                "",
                "Error emptying clipboard."
            };
        }

        size_t len = text.length() + 1;
        HGLOBAL copyText = GlobalAlloc(GMEM_MOVEABLE, len);
        const char* localCopy = text.c_str();
        strcpy_s((char*)GlobalLock(copyText), len, localCopy);

        if (SetClipboardData(CF_TEXT, copyText) == nullptr) {
            GlobalUnlock(copyText);
            CloseClipboard();

            return ClipboardResponse {
                ClipboardResponseType::Error,
                "",
                "Error opening clipboard."
            };
        }

        GlobalUnlock(copyText);
        CloseClipboard();

        return ClipboardResponse { ClipboardResponseType::Ok, "", "" };
    #endif

    return ClipboardResponse { ClipboardResponseType::Unsupported, "", "" };
}

ClipboardResponse pasteFromClipboard() {
    #ifdef _WIN32
        if (!OpenClipboard(nullptr)) {
            return ClipboardResponse {
                ClipboardResponseType::Error,
                "",
                "Error opening clipboard."
            };
        }

        HANDLE data = GetClipboardData(CF_TEXT);

        if (!data) {
            CloseClipboard();

            return ClipboardResponse {
                ClipboardResponseType::Error,
                "",
                "Could not get clipboard data."
            };
        }

        char* text_c = (char*) GlobalLock(data);

        if (!text_c) {
            GlobalUnlock(data);
            CloseClipboard();

            return ClipboardResponse {
                ClipboardResponseType::Error,
                "",
                "Could not get text from clipboard."
            };
        }

        string text(text_c);

        GlobalUnlock(data);
        CloseClipboard();

        return ClipboardResponse { ClipboardResponseType::Ok, text, "" };
    #endif

    return ClipboardResponse { ClipboardResponseType::Unsupported, "", "" };
}

string getClipboardError(ClipboardResponse cr) {
    switch (cr.respType) {
        case ClipboardResponseType::Ok: return "";
        case ClipboardResponseType::Unsupported:
            return "Error: Clipboard functionalities are not currently supported by this app in your current platform.";
        case ClipboardResponseType::Error: return "Error: " + cr.errorMsg;
    }
}

