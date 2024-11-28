/*
 * Copyright (C) the iPlug 2 developers, Rory Walsh (c) 2024
 * 
 * Cabbage3 is licensed under the MIT License. See the LICENSE file for details.
 * This software is provided "as-is", without any express or implied warranty.
 * See the LICENSE file for more details.
 * 
 * Modifications made by Rory Walsh in 2024.
 * 
 * This file is based on the iPlug 2 library, which is licensed under the
 * [iPlug 2 License Information]. The original copyright notice and license
 * must remain intact in the portions of the code that have not been modified.
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