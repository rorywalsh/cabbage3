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
#include <sstream>
#import <Cocoa/Cocoa.h>

namespace cabbage {

std::string File::browseForFile(const std::string& title, const std::string& initialDir, const std::string& filters)
{
    // macOS implementation using NSOpenPanel
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];
        [panel setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:NO];
        [panel setAllowsMultipleSelection:NO];
        
        if (!initialDir.empty()) {
            [panel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:initialDir.c_str()]]];
        }
        
        // Set allowed file types
        if (!filters.empty() && filters != "*") {
            NSMutableArray* fileTypes = [NSMutableArray array];
            std::istringstream iss(filters);
            std::string token;
            while (std::getline(iss, token, ';')) {
                if (!token.empty()) {
                    // Remove leading * if present
                    if (token[0] == '*') token = token.substr(1);
                    [fileTypes addObject:[NSString stringWithUTF8String:token.c_str()]];
                }
            }
            [panel setAllowedFileTypes:fileTypes];
        }
        
        if ([panel runModal] == NSModalResponseOK) {
            NSURL* url = [[panel URLs] objectAtIndex:0];
            return std::string([[url path] UTF8String]);
        }
    }
    return "";
}

} // end of namespace