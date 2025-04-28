// main_window.cpp

#include "../include/main_window.h"
#include "../include/thumbnail_manager.h"
#include "../include/global_state.h"
#include "../include/utils.h"

// 主窗口过程
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            // 清理所有缩略图
            for (auto &pair: g_thumbnails) {
                const auto &pipInfo = pair.second;
                DwmUnregisterThumbnail(pair.second.hThumbnail);
                DestroyWindow(pair.second.hwndPip);
                DestroyWindow(pair.second.hwndInputBuffer); // 销毁输入缓冲区窗口
            }
            g_thumbnails.clear();
            PostQuitMessage(0);

            break;

        case WM_HOTKEY:
            if (wParam == 1) {
                // 注册当前前台窗口为缩略图
                HWND fgWindow = GetForegroundWindow();
                if (fgWindow && g_thumbnails.find(fgWindow) == g_thumbnails.end()) {
                    RegisterThumbnail(fgWindow);
                    // 绘制原始窗口边框
                    if (g_appState.showOriginalBorder) {
                        InvalidateRect(fgWindow, NULL, TRUE);
                        UpdateWindow(fgWindow);
                    }
                }
            }
            break;
        case WM_CREATE:
            // 初始化状态栏
            g_appState.hwndStatusBar = CreateWindowExW(
                    0, STATUSCLASSNAMEW, L"Status Bar",
                    WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                    0, 0, 100, STATUS_BAR_HEIGHT,
                    hwnd, NULL, GetModuleHandle(NULL), NULL);
            break;

        case WM_SIZE:
            // 调整状态栏大小
        {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            int width = rcClient.right - rcClient.left;
            int height = rcClient.bottom - rcClient.top;
            MoveWindow(g_appState.hwndStatusBar, 0, height - STATUS_BAR_HEIGHT, width, STATUS_BAR_HEIGHT, TRUE);
            break;
        }
        case WM_PAINT:
            // 绘制原始窗口边框
            if (g_appState.showOriginalBorder) {
                for (auto const &[hwnd, thumbnailInfo]: g_thumbnails) {
                    DrawOriginalWindowBorder(hwnd);
                }
            }
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}