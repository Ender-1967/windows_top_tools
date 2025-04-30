// global_state.cpp

#include "../include/global_state.h"

std::map<HWND, ThumbnailInfo> g_thumbnails;
AppState g_appState;
    int TITLE_BAR_HEIGHT = 40;

// 添加原始窗口大小
std::map<HWND, RECT> g_originalWindowRects;