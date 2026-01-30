#include "include/utils.hpp"

#ifdef _WIN32
    #include <winrt/windows.ui.viewmanagement.h>
    using namespace winrt::Windows::UI::ViewManagement;
#endif

using namespace std;

bool isDarkMode() {
    #ifdef _WIN32
         auto settings = UISettings();
         auto foreground = settings.GetColorValue(UIColorType::Foreground);

         return (5 * foreground.G) + (2 * foreground.R) + foreground.B > 1024;
    #endif

    return false;
}

