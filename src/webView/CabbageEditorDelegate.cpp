 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "CabbageEditorDelegate.h"

using namespace iplug;

CabbageEditorDelegate::CabbageEditorDelegate(int nParams)
  : IEditorDelegate(nParams)
  , IWebView()
{
}

CabbageEditorDelegate::~CabbageEditorDelegate()
{
  CloseWindow();
}

extern float GetScaleForHWND(HWND hWnd);

void* CabbageEditorDelegate::OpenWindow(void* pParent)
{
  auto scale = GetScaleForHWND((HWND) pParent);
  return OpenWebView(pParent, 0., 0., static_cast<float>((GetEditorWidth()) / scale), static_cast<float>((GetEditorHeight()) / scale), scale);
}

void CabbageEditorDelegate::Resize(int width, int height)
{
  SetWebViewBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
  EditorResizeFromUI(width, height, true);
}

void CabbageEditorDelegate::OpenFileBrowser()
{
	// Not implemented
	cabAssert(false, "Not implemented");
}