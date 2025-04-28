// input_buffer_window.cpp

#include "../include/input_buffer_window.h"
#include "../include/pip_window.h"

// 创建用于接收中文输入的隐藏窗口
HWND CreateInputBufferWindow(HINSTANCE hInstance) {
    return CreateWindowExW(
            WS_EX_TOOLWINDOW, // 不显示在任务栏或 Alt+Tab
            INPUT_BUFFER_CLASS,
            L"InputBuffer",
            WS_POPUP, // 必须是弹出窗口
            0, 0, 0, 0, // 初始位置和大小
            NULL, NULL, hInstance, NULL);
}