// global_state.h

#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <dwmapi.h>

struct ThumbnailInfo {
    HWND hwndPip;         // 画中画窗口句柄
    HWND hwndOriginal;    // 原始窗口句柄
    HTHUMBNAIL hThumbnail; // 缩略图句柄
    float aspectRatio;    // 宽高比
    std::wstring originalTitle; // 原始窗口标题
    RECT originalRect;    // 原始窗口位置
    int originalShowCmd;  // 原始窗口显示状态
    HWND hwndInputBuffer; // 用于接收中文输入的隐藏窗口
    float scaleX;
    float scaleY;
};

struct AppState {
    bool isDragging = false;
    HWND hwndBeingDragged = nullptr;
    POINT dragStartScreenPos = {0, 0};
    RECT windowStartRect = {0, 0, 0, 0};
    HWND hwndStatusBar = nullptr;
    bool isResizing = false;
    bool showOriginalBorder = true;
};

extern std::map<HWND, ThumbnailInfo> g_thumbnails; // 源窗口 -> 画中画窗口和缩略图句柄映射
extern AppState g_appState;

const float SCALE_FACTOR = 0.3f;
extern int TITLE_BAR_HEIGHT;
const int CLOSE_BUTTON_SIZE = 30;
const int STATUS_BAR_HEIGHT = 20;

const wchar_t MAIN_WINDOW_CLASS[] = L"MainWndClass";
const wchar_t PIP_WINDOW_CLASS[] = L"PipWndClass";
const wchar_t INPUT_BUFFER_CLASS[] = L"InputBufferClass";