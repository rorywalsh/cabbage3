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