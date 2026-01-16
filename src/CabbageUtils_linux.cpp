/*
 * Copyright (c) 2024 Rory Walsh
 *
 * This file is part of Cabbage3
 *
 * Cabbage3 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cabbage3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cabbage3.  If not, see <https://www.gnu.org/licenses/>.
 */

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