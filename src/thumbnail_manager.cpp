// thumbnail_manager.cpp

#include "../include/thumbnail_manager.h"
#include "../include/pip_window.h"
#include "../include/input_buffer_window.h"
#include "../include/global_state.h"
#include "../include/utils.h"

// 声明全局变量 g_originalWindowRects
extern std::map<HWND, RECT> g_originalWindowRects;

// 注册缩略图
bool RegisterThumbnail(HWND srcHwnd) {

    // 检查是否已经注册过
    if (g_thumbnails.find(srcHwnd) != g_thumbnails.end()) {
        return false;
    }
    HWND destHwnd = CreatePipWindow(srcHwnd);

    // 保存原始窗口信息
    RECT originalRect;
    GetWindowRect(srcHwnd, &originalRect);
    g_originalWindowRects[destHwnd] = originalRect;  // 保存原始窗口大小

    if (!destHwnd) return false;

    HTHUMBNAIL thumbnail;
    HRESULT hr = DwmRegisterThumbnail(destHwnd, srcHwnd, &thumbnail);
    if (SUCCEEDED(hr)) {
        // 计算原始窗口的宽高比
        RECT srcRect;
        GetWindowRect(srcHwnd, &srcRect);
        float aspectRatio = (float) (srcRect.right - srcRect.left) / (srcRect.bottom - srcRect.top);

        // 获取原始窗口标题
        int titleLength = GetWindowTextLengthW(srcHwnd) + 1;
        std::wstring originalTitle(titleLength, L'\0');
        GetWindowTextW(srcHwnd, &originalTitle[0], titleLength);
        originalTitle.resize(titleLength - 1);

        // 保存原始窗口信息
        RECT originalRect;
        GetWindowRect(srcHwnd, &originalRect);
        int originalShowCmd = GetWindowLong(srcHwnd, GWL_STYLE) & WS_VISIBLE ? SW_SHOWNORMAL : SW_HIDE;

        // 创建输入缓冲区窗口
        HWND hwndInputBuffer = CreateInputBufferWindow(GetModuleHandle(NULL));

        // 设置缩略图属性
        DWM_THUMBNAIL_PROPERTIES props = {};
        props.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE |
                        DWM_TNP_OPACITY | DWM_TNP_SOURCECLIENTAREAONLY;
        props.opacity = 255;
        props.fVisible = TRUE;
        props.fSourceClientAreaOnly = TRUE;

        RECT destRect;
        GetClientRect(destHwnd, &destRect);
        destRect.top += TITLE_BAR_HEIGHT; // 考虑标题栏高度
        props.rcDestination = destRect;

        DwmUpdateThumbnailProperties(thumbnail, &props);

        // 存储缩略图信息，包括原始窗口句柄和标题
        g_thumbnails[srcHwnd] = {destHwnd, srcHwnd, thumbnail, aspectRatio, originalTitle};

        InvalidateRect(destHwnd, NULL, TRUE);
        UpdateWindow(destHwnd);

        UpdateStatusBar(destHwnd); // 更新状态栏

        // 设置原始窗口属性，使其保持活动状态但不显示
        SetWindowLongPtr(srcHwnd, GWL_EXSTYLE,
                         GetWindowLongPtr(srcHwnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
        SetLayeredWindowAttributes(srcHwnd, 0, 0, LWA_ALPHA); // 完全透明,设置窗口隐藏
        ShowWindow(srcHwnd, SW_SHOWNORMAL); // 确保窗口处于显示状态
        SetWindowPos(srcHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // 始终置顶

        return true;
    }

    // 注册失败，销毁画中画窗口
    DestroyWindow(destHwnd);
    return false;
}

// 移除缩略图
void UnregisterThumbnail(HWND srcHwnd) {
    auto it = g_thumbnails.find(srcHwnd);
    if (it != g_thumbnails.end()) {

        // 恢复原始窗口的属性
        SetWindowLongPtr(srcHwnd, GWL_EXSTYLE,
                         GetWindowLongPtr(srcHwnd, GWL_EXSTYLE) & ~(WS_EX_LAYERED | WS_EX_TRANSPARENT));
        SetLayeredWindowAttributes(srcHwnd, 0, 255, LWA_ALPHA); // 恢复不透明度
//        SetWindowPos(srcHwnd, NULL, it->second.originalRect.left, it->second.originalRect.top,
//                     it->second.originalRect.right - it->second.originalRect.left,
//                     it->second.originalRect.bottom - it->second.originalRect.top,
//                     SWP_NOZORDER);
//        ShowWindow(srcHwnd, it->second.originalShowCmd); // 恢复显示状态
        SetWindowPos(srcHwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); // 取消置顶

        DwmUnregisterThumbnail(it->second.hThumbnail);
//        DestroyWindow(it->second.hwndInputBuffer); // 销毁输入缓冲区窗口
        if(IsWindow(it->second.hwndPip))
            DestroyWindow(it->second.hwndPip);
        g_thumbnails.erase(it);
    }
}


// 获取原始窗口句柄
HWND GetOriginalWindowHandle(HWND hwndPip) {
    for (const auto &pair: g_thumbnails) {
        if (pair.second.hwndPip == hwndPip) {
            return pair.first;
        }
    }
    return nullptr;
}

// 绘制原始窗口边框
void DrawOriginalWindowBorder(HWND hwnd) {
    HDC hdc = GetDC(hwnd);
    if (hdc) {
        RECT rect;
        GetWindowRect(hwnd, &rect);
        // 转换为客户区坐标
        MapWindowPoints(NULL, hwnd, (LPPOINT) &rect, 2);
        int borderWidth = 3; // 边框宽度
        HPEN hPen = CreatePen(PS_SOLID, borderWidth, RGB(255, 0, 0)); // 红色边框
        HBRUSH hBrush = (HBRUSH) GetStockObject(NULL_BRUSH); // 透明背景

        SelectObject(hdc, hPen);
        SelectObject(hdc, hBrush);

        Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

        DeleteObject(hPen);
        ReleaseDC(hwnd, hdc);
    }
}

// 更新状态栏
void UpdateStatusBar(HWND hwnd) {
    if (g_appState.hwndStatusBar) {
        int parts[] = {100, 200, -1};
        SendMessageW(g_appState.hwndStatusBar, SB_SETPARTS, sizeof(parts) / sizeof(int), (LPARAM) parts);

        std::wstring statusText1 = L"Total: " + std::to_wstring(g_thumbnails.size());
        SendMessageW(g_appState.hwndStatusBar, SB_SETTEXTW, 0, (LPARAM) statusText1.c_str());

        HWND originalHwnd = GetOriginalWindowHandle(hwnd);
        if (originalHwnd) {
            std::wstring statusText2 = L"HWND: " + std::to_wstring((UINT_PTR) originalHwnd);
            SendMessageW(g_appState.hwndStatusBar, SB_SETTEXTW, 1, (LPARAM) statusText2.c_str());
        } else {
            SendMessageW(g_appState.hwndStatusBar, SB_SETTEXTW, 1, (LPARAM) L"");
        }
    }
}