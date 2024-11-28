/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#if __has_feature(objc_arc)
#error This file must be compiled without Arc. Don't use -fobjc-arc flag!
#endif



#include "CabbageEditorDelegate.h"

#ifdef OS_MAC
#import <AppKit/AppKit.h>
#elif defined(OS_IOS)
#import <UIKit/UIKit.h>
#endif



extern "C" const char* OpenFileBrowser() {
    @autoreleasepool {
        NSOpenPanel* panel = [NSOpenPanel openPanel];

        [panel setCanChooseFiles:YES];
        [panel setCanChooseDirectories:YES];
        [panel setAllowsMultipleSelection:NO];
        [panel setAllowedFileTypes:nil]; // Set file types if needed

        // runModal blocks execution until the user makes a choice
        NSInteger result = [panel runModal];
        if (result == NSModalResponseOK) {
            NSURL* selectedFileURL = [[panel URLs] objectAtIndex:0];
            NSString* filePath = [selectedFileURL path];
            const char* cFilePath = [filePath UTF8String];
            char* result = strdup(cFilePath); // Duplicate the string to return it
            return result;
        } else {
            return nullptr;
        }
    }
}

using namespace iplug;

@interface HELPER_VIEW : PLATFORM_VIEW
{
    CabbageEditorDelegate* mDelegate;
}
- (void) removeFromSuperview;
- (id) initWithEditorDelegate: (CabbageEditorDelegate*) pDelegate;
@end

@implementation HELPER_VIEW
{
}

- (id) initWithEditorDelegate: (CabbageEditorDelegate*) pDelegate;
{
    mDelegate = pDelegate;
    CGFloat w = pDelegate->GetEditorWidth();
    CGFloat h = pDelegate->GetEditorHeight();
    CGRect r = CGRectMake(0, 0, w, h);
    self = [super initWithFrame:r];
    
    void* pWebView = pDelegate->OpenWebView(self, 0, 0, w, h, 1.0f, pDelegate->GetEnableDevTools());
    
    [self addSubview: (PLATFORM_VIEW*) pWebView];
    
    return self;
}

- (void) removeFromSuperview
{
#ifdef AU_API
    //For AUv2 this is where we know about the window being closed, close via delegate
    mDelegate->CloseWindow();
#endif
    [super removeFromSuperview];
}

@end

CabbageEditorDelegate::CabbageEditorDelegate(int nParams)
: IEditorDelegate(nParams)
, IWebView()
{
    
}

CabbageEditorDelegate::~CabbageEditorDelegate()
{
    CloseWindow();
    
    if (editorDeleteFuncCallback)
        editorDeleteFuncCallback();
    
    PLATFORM_VIEW* pHelperView = (PLATFORM_VIEW*) mHelperView;
    [pHelperView release];
    mHelperView = nullptr;
}

void* CabbageEditorDelegate::OpenWindow(void* pParent)
{
    PLATFORM_VIEW* pParentView = (PLATFORM_VIEW*) pParent;
    
    HELPER_VIEW* pHelperView = [[HELPER_VIEW alloc] initWithEditorDelegate: this];
    mHelperView = (void*) pHelperView;
    
    if (pParentView) {
        [pParentView addSubview: pHelperView];
    }
    
    if (editorInitFuncCallback)
        editorInitFuncCallback();
    
    
    return mHelperView;
    
}

/*
 Opens a native file browser dialogue
 */
void CabbageEditorDelegate::OpenFileBrowser() {
    const char* selectedPath = ::OpenFileBrowser();  // Call the global function
    if (selectedPath) {
        selectedFilePath = std::string(selectedPath);  // Store the selected path in a member variable
        free((void*)selectedPath);  // Free the duplicated string
    } else {
        selectedFilePath.clear();
    }
    
}

void CabbageEditorDelegate::Resize(int width, int height)
{
    CGFloat w = static_cast<float>(width);
    CGFloat h = static_cast<float>(height);
    HELPER_VIEW* pHelperView = (HELPER_VIEW*) mHelperView;
    [pHelperView setFrame:CGRectMake(0, 0, w, h)];
    SetWebViewBounds(0, 0, w, h);
    EditorResizeFromUI(width, height, true);
}


