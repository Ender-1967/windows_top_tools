#include <windows.h>
#include <dwmapi.h>
#include <map>
#include <string>
#include <stdio.h>
#include <windowsx.h>
#include <algorithm>
#include <wingdi.h>
#include <iostream>


#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
const wchar_t MAIN_WINDOW_CLASS[] = L"PIPManagerMainClass";


#include <Windows.h>

// 宽字符版本窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CHAR: {

            WCHAR wc = (WCHAR) wParam;

            // 判断是否为中文字符（CJK统一汉字区间）
            if (wc >= 0x4E00 && wc <= 0x9FFF) {
                // 处理中文输入
                std::wcout << L"输入中文字符: " << wc << std::endl;
            } else {
                // 处理英文/符号
                std::cout << "输入英文字符: " << (char) wc << std::endl;
            }
            break;
        }
            // 处理Unicode字符输入
        default:
            return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}


//// 注册窗口类
bool RegisterWindowClasses(HINSTANCE hInstance) {
    WNDCLASSW wc = {};

//    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MAIN_WINDOW_CLASS;
    if (!RegisterClassW(&wc)) return false;

    wc.lpfnWndProc = WndProc;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszClassName = PIP_WINDOW_CLASS;
    return RegisterClassW(&wc) != 0;
}

// 注册Unicode窗口类
//ATOM MyRegisterClass(HINSTANCE hInstance) {
//    WNDCLASSEXW wcex;  // 使用宽字符结构体
//    wcex.cbSize = sizeof(WNDCLASSEXW);
//    wcex.style = CS_HREDRAW | CS_VREDRAW;
//    wcex.lpfnWndProc = WndProc;  // 宽字符窗口过程
//    wcex.cbClsExtra = 0;
//    wcex.cbWndExtra = 0;
//    wcex.hInstance = hInstance;
//    wcex.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APPLICATION));  // 宽字符资源加载
//    wcex.hCursor = LoadCursorW(NULL, reinterpret_cast<LPCWSTR>(IDC_ARROW));
//    wcex.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
//    wcex.lpszMenuName = NULL;
//    wcex.lpszClassName = L"MyUnicodeWindowClass";  // 宽字符类名
////    wcex.hIconSm = LoadIconW(wcex.hInstance, MAKEINTRESOURCEW(IDI_APPLICATION));
//
//    return RegisterClassExW(&wcex);  // 宽字符注册函数
//}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    // 注册窗口类
    if (!MyRegisterClass(hInstance)) {
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

