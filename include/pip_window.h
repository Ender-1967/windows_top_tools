// pip_window.h

#pragma once

#include <windows.h>
#include <dwmapi.h>
#include "global_state.h"

HWND CreatePipWindow(HWND srcHwnd);
LRESULT CALLBACK PipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);