// main.cpp

#include <windows.h>
#include <iostream>
#include "../include/pip_window.h"
#include "../include/main_window.h"
#include "../include/thumbnail_manager.h"
#include "../include/input_buffer_window.h"
#include "../include/global_state.h"
#include "../include/utils.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    setbuf(stdout, NULL);

    // 注册窗口类
    if (!RegisterWindowClasses(hInstance)) {
        MessageBoxW(NULL, L"Failed to register window classes!", L"Error", MB_ICONERROR);
        return 1;
    }

    // 创建主窗口
    HWND hwnd = CreateWindowW(
            MAIN_WINDOW_CLASS, L"PIP Manager", WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
            NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxW(NULL, L"Failed to create main window!", L"Error", MB_ICONERROR);
        return 1;
    }

    // 注册热键 (Alt+Q)
    if (!RegisterHotKey(hwnd, 1, MOD_ALT | MOD_NOREPEAT, 'Q')) {
        MessageBoxW(NULL, L"Failed to register hotkey!", L"Warning", MB_ICONWARNING);
    }

    // 显示主窗口
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 注销热键
    UnregisterHotKey(hwnd, 1);

    return (int) msg.wParam;
}