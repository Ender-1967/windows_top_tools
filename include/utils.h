// utils.h

#pragma once

#include <windows.h>

bool RegisterWindowClasses(HINSTANCE hInstance);
bool IsInTitleBar(HWND hwnd, int x, int y);