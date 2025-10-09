#include "CabbageUtils.h"
#include <cstdio>

namespace cabbage {

std::string File::browseForFile(const std::string& title, const std::string& initialDir, const std::string& filters)
{
    // Linux implementation using zenity
    std::string cmd = "zenity --file-selection --title=\"" + title + "\"";
    if (!initialDir.empty()) {
        cmd += " --filename=\"" + initialDir + "/\"";
    }
    if (!filters.empty() && filters != "*") {
        // zenity uses --file-filter, but it's complex, so skip for now
    }
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    
    char buffer[128];
    std::string result = "";
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        result += buffer;
    }
    pclose(pipe);
    
    // Remove trailing newline
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}

} // end of namespace