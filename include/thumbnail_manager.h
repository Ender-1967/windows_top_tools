// thumbnail_manager.h

#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <dwmapi.h>
#include "global_state.h"

bool RegisterThumbnail(HWND srcHwnd);
void UnregisterThumbnail(HWND srcHwnd);
HWND GetOriginalWindowHandle(HWND hwndPip);
void DrawOriginalWindowBorder(HWND hwnd);
void UpdateStatusBar(HWND hwnd);