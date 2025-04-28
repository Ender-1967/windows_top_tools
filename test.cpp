#include <windows.h>
#include <dwmapi.h>
#include <map>
#include <string>
#define UNICODE
#define _UNICODE

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")

// 全局变量
std::map<HWND, HWND> g_thumbnails; // 源窗口->画中画窗口映射
const float SCALE_FACTOR = 0.3f;
bool g_isDragging = false; // 标记是否正在拖动
POINT g_dragStart; // 鼠标按下时的位置
RECT g_windowStartRect; // 窗口初始位置
// 记录上次鼠标位置
POINT g_lastMovePos = {0, 0};  // 上次鼠标位置
POINT g_dragStartScreenPos = {}; // 鼠标开始拖动时的屏幕坐标


// 创建画中画窗口
HWND CreatePipWindow(HWND srcHwnd) {
    // 获取源窗口位置和大小
    RECT srcRect;
    GetWindowRect(srcHwnd, &srcRect);

    // 计算缩略图尺寸
    int width = static_cast<int>((srcRect.right - srcRect.left) * SCALE_FACTOR);
    int height = static_cast<int>((srcRect.bottom - srcRect.top) * SCALE_FACTOR);

    // 创建无边框窗口
    HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
            L"PIPManagerClass",
            L"PIP Window",
            WS_POPUP | WS_VISIBLE,
            srcRect.left, srcRect.top, width, height,
            NULL, NULL, NULL, NULL
    );

    // 添加关闭按钮
    CreateWindowW(
            L"BUTTON", L"X",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            5, 5, 30, 30,
            hwnd, (HMENU)1, NULL, NULL
    );

    return hwnd;
}

// 注册缩略图
bool RegisterThumbnail(HWND srcHwnd) {
    if (g_thumbnails.find(srcHwnd) != g_thumbnails.end()) {
        return false; // 已存在
    }

    HWND destHwnd = CreatePipWindow(srcHwnd);
    if (!destHwnd) return false;

    HTHUMBNAIL thumbnail;
    HRESULT hr = DwmRegisterThumbnail(destHwnd, srcHwnd, &thumbnail);
    if (SUCCEEDED(hr)) {
        // 设置缩略图属性
        DWM_THUMBNAIL_PROPERTIES props;
        props.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE | DWM_TNP_OPACITY;
        props.opacity = 255;
        props.fVisible = TRUE;

        RECT destRect;
        GetClientRect(destHwnd, &destRect);
        destRect.top += 40; // 留出标题栏空间
        props.rcDestination = destRect;

        DwmUpdateThumbnailProperties(thumbnail, &props);

        g_thumbnails[srcHwnd] = destHwnd;
        return true;
    }

    DestroyWindow(destHwnd);
    return false;
}

// 移除缩略图
void UnregisterThumbnail(HWND srcHwnd) {
    auto it = g_thumbnails.find(srcHwnd);
    if (it != g_thumbnails.end()) {
        DestroyWindow(it->second);
        g_thumbnails.erase(it);
    }
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // 关闭按钮
                for (auto it = g_thumbnails.begin(); it != g_thumbnails.end(); ++it) {
                    if (it->second == hwnd) {
                        UnregisterThumbnail(it->first);
                        break;
                    }
                }
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_LBUTTONDOWN: {  // 鼠标左键按下，开始拖动
            SetCapture(hwnd);  // 捕获鼠标
            g_isDragging = true;

            // 获取当前鼠标位置，作为拖动的起点
            g_dragStart.x = LOWORD(lParam);
            g_dragStart.y = HIWORD(lParam);

            // 获取窗口当前位置
            GetWindowRect(hwnd, &g_windowStartRect);

            // 更新最后的鼠标位置
            g_lastMovePos.x = g_dragStart.x;
            g_lastMovePos.y = g_dragStart.y;

            break;
        }

        case WM_LBUTTONUP: {  // 鼠标左键释放，停止拖动
            ReleaseCapture();
            g_isDragging = false;
            break;
        }

        case WM_MOUSEMOVE: {  // 鼠标移动时更新窗口位置
            if (g_isDragging) {
                int deltaX = LOWORD(lParam) - g_dragStart.x;
                int deltaY = HIWORD(lParam) - g_dragStart.y;

                // 优化：只有鼠标移动超过一定的阈值时才更新窗口位置
                if (abs(LOWORD(lParam) - g_lastMovePos.x) > 5 || abs(HIWORD(lParam) - g_lastMovePos.y) > 5) {
                    int newX = g_windowStartRect.left + deltaX;
                    int newY = g_windowStartRect.top + deltaY;

                    // 移动窗口
                    SetWindowPos(hwnd, NULL, newX, newY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

                    // 更新最后的鼠标位置
                    g_lastMovePos.x = LOWORD(lParam);
                    g_lastMovePos.y = HIWORD(lParam);
                }
            }
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 热键处理
void HandleHotkey() {
    HWND fgWindow = GetForegroundWindow();
    if (fgWindow && g_thumbnails.find(fgWindow) == g_thumbnails.end()) {
        RegisterThumbnail(fgWindow);
    }
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    // 注册窗口类
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"PIPManagerClass";  // 确保使用正确的类名
    RegisterClassW(&wc);  // 使用 RegisterClassW 注册类

    // 创建消息窗口
    HWND hwnd = CreateWindowW(
            L"PIPManagerClass", L"PIP Manager", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL
    );

    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // 注册热键
    RegisterHotKey(hwnd, 1, MOD_ALT, 'Q');  // 去掉 MOD_NOREPEAT

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == WM_HOTKEY) {
            HandleHotkey();
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 清理
    for (auto& pair : g_thumbnails) {
        DestroyWindow(pair.second);
    }

    return 0;
}
