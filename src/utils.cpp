// utils.cpp

#include "../include/utils.h"
#include "../include/global_state.h"
#include "../include/main_window.h"
#include "../include/pip_window.h"

// 注册窗口类
bool RegisterWindowClasses(HINSTANCE hInstance) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MAIN_WINDOW_CLASS;
    if (!RegisterClassW(&wc)) return false;

    wc = {};
    wc.lpfnWndProc = PipWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = PIP_WINDOW_CLASS;
    if (!RegisterClassW(&wc)) return false;

    // 注册用于接收中文输入的隐藏窗口类
    WNDCLASSW inputBufferClass = {};
    inputBufferClass.lpfnWndProc = DefWindowProcW;
    inputBufferClass.hInstance = hInstance;
    inputBufferClass.lpszClassName = INPUT_BUFFER_CLASS;
    if (!RegisterClassW(&inputBufferClass)) return false;

    return true;
}

// 检查鼠标是否在标题栏
bool IsInTitleBar(HWND hwnd, int x, int y) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    return y < TITLE_BAR_HEIGHT;
}