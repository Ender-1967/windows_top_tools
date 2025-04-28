#include <windows.h>
#include <dwmapi.h>
#include <map>
#include <stdio.h>
#include <windowsx.h>
#include <algorithm>
#include <wingdi.h>
#include <imm.h>
#include <vector>

#define UNICODE
#define _UNICODE

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "msimg32.lib")
#pragma comment(lib, "imm32.lib")

// 全局变量
struct ThumbnailInfo {
    HWND hwndPip;         // 画中画窗口句柄
    HWND hwndOriginal;    // 原始窗口句柄
    HTHUMBNAIL hThumbnail; // 缩略图句柄
    float aspectRatio;    // 宽高比
};

std::map<HWND, ThumbnailInfo> g_thumbnails; // 源窗口 -> 画中画窗口和缩略图句柄映射
const float SCALE_FACTOR = 0.3f;
int TITLE_BAR_HEIGHT = 40;             // 标题栏高度
const int CLOSE_BUTTON_SIZE = 30;      // 关闭按钮大小
const int STATUS_BAR_HEIGHT = 20;
HIMC g_hOriginalImc = nullptr;


// 拖动相关变量
struct AppState {
    bool isResizing = false;
    bool isDragging = false;
    POINT dragStartScreenPos;
    RECT windowStartRect;
    HWND hwndBeingDragged = nullptr;
    HWND hwndStatusBar = nullptr;
};

AppState g_appState;

// 窗口类名
const wchar_t MAIN_WINDOW_CLASS[] = L"PIPManagerMainClass";
const wchar_t PIP_WINDOW_CLASS[] = L"PIPWindowClass";

// 创建画中画窗口
HWND CreatePipWindow(HWND srcHwnd) {
    // 获取源窗口位置和大小
    RECT srcRect;
    GetWindowRect(srcHwnd, &srcRect);

    // 计算缩略图尺寸
    int width = static_cast<int>((srcRect.right - srcRect.left) * SCALE_FACTOR);
    int height = static_cast<int>((srcRect.bottom - srcRect.top) * SCALE_FACTOR) + CLOSE_BUTTON_SIZE;

    // 创建无边框窗口，并移除标题栏
    HWND hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            PIP_WINDOW_CLASS,
            L"PIP Window",
            WS_POPUP | WS_VISIBLE,
            srcRect.left, srcRect.top, width, height,
            NULL, NULL, GetModuleHandle(NULL), NULL);

    // 设置窗口圆角
    HRGN hRgn = CreateRoundRectRgn(0, 0, width, height, 8, 8);
    SetWindowRgn(hwnd, hRgn, TRUE);
    DeleteObject(hRgn);

    // 设置背景透明度
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    // 创建缩小后的关闭按钮并固定到左上角
    const int newCloseButtonSize = CLOSE_BUTTON_SIZE / 2;
    CreateWindowW(
            L"BUTTON", L"X",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_OWNERDRAW,
            5, 5, newCloseButtonSize, newCloseButtonSize,
            hwnd, (HMENU) 1, GetModuleHandle(NULL), NULL);
    TITLE_BAR_HEIGHT = newCloseButtonSize * 1.6;

    return hwnd;
}

// 注册缩略图
bool RegisterThumbnail(HWND srcHwnd) {

    // 检查是否已经注册过
    if (g_thumbnails.find(srcHwnd) != g_thumbnails.end()) {
        return false;
    }

    HWND destHwnd = CreatePipWindow(srcHwnd);

    // 创建 PIP 窗口时关联原始输入法上下文
    if (!g_hOriginalImc) {
        g_hOriginalImc = ImmGetContext(srcHwnd);
    }
    ImmAssociateContext(destHwnd, g_hOriginalImc);
    if (!destHwnd) return false;

    HTHUMBNAIL thumbnail;
    HRESULT hr = DwmRegisterThumbnail(destHwnd, srcHwnd, &thumbnail);
    if (SUCCEEDED(hr)) {
        // 计算原始窗口的宽高比
        RECT srcRect;
        GetWindowRect(srcHwnd, &srcRect);
        float aspectRatio = (float) (srcRect.right - srcRect.left) / (srcRect.bottom - srcRect.top);

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

        // 存储缩略图信息，包括原始窗口句柄
        g_thumbnails[srcHwnd] = {destHwnd, srcHwnd, thumbnail, aspectRatio};


        InvalidateRect(destHwnd, NULL, TRUE);
        UpdateWindow(destHwnd);

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
        DwmUnregisterThumbnail(it->second.hThumbnail);
        DestroyWindow(it->second.hwndPip);
        g_thumbnails.erase(it);
    }
}

// 检查鼠标坐标是否在标题栏区域
bool IsInTitleBar(HWND hwnd, int x, int y) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    return y < TITLE_BAR_HEIGHT && x < (clientRect.right - CLOSE_BUTTON_SIZE - 10);
}

// 创建辅助函数查找原始窗口
HWND FindOriginalWindow(HWND hwndPip) {
    for (const auto &pair: g_thumbnails) {
        if (pair.second.hwndPip == hwndPip) {
            return pair.second.hwndOriginal;
        }
    }
    return nullptr;
}

void SendUnicodeChar(HWND hWnd, WCHAR wch) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wScan = 0;
    input.ki.time = 0;
    input.ki.dwExtraInfo = 0;

    input.ki.wVk = 0;
    input.ki.dwFlags = KEYEVENTF_UNICODE;
    input.ki.wScan = wch;
    ::SendInput(1, &input, sizeof(INPUT));

    input.ki.dwFlags |= KEYEVENTF_KEYUP;
    ::SendInput(1, &input, sizeof(INPUT));
}

// 画中画窗口过程
LRESULT CALLBACK PipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            // 清理缩略图资源
            for (auto it = g_thumbnails.begin(); it != g_thumbnails.end(); ++it) {
                if (it->second.hwndPip == hwnd) {
                    DwmUnregisterThumbnail(it->second.hThumbnail);
                    g_thumbnails.erase(it);
                    break;
                }
            }
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // 关闭按钮
                DestroyWindow(hwnd);
            }
            break;

        case WM_DRAWITEM:
            if (wParam == 1) { // 绘制关闭按钮
                LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT) lParam;
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0));
                FillRect(dis->hDC, &dis->rcItem, hBrush);
                DeleteObject(hBrush);

                SetBkMode(dis->hDC, TRANSPARENT);
                SetTextColor(dis->hDC, RGB(255, 255, 255));
                DrawTextW(dis->hDC, L"X", -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                if (dis->itemState & ODS_HOTLIGHT) {
                    hBrush = CreateSolidBrush(RGB(255, 70, 70));
                    FillRect(dis->hDC, &dis->rcItem, hBrush);
                    DeleteObject(hBrush);
                }
            }
            break;

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            if (IsInTitleBar(hwnd, x, y)) {
                // 标题栏拖动
                SetCapture(hwnd);
                g_appState.isDragging = true;
                g_appState.hwndBeingDragged = hwnd;
                GetCursorPos(&g_appState.dragStartScreenPos);
                GetWindowRect(hwnd, &g_appState.windowStartRect);
            } else {
                // 鼠标点击事件传递到原始窗口
                HWND originalHwnd = nullptr;
                for (const auto &pair: g_thumbnails) {
                    if (pair.second.hwndPip == hwnd) {
                        originalHwnd = pair.second.hwndOriginal;
                        break;
                    }
                }

                if (originalHwnd) {
                    // 坐标转换
                    RECT pipRect;
                    GetWindowRect(hwnd, &pipRect);
                    int pipWidth = pipRect.right - pipRect.left;
                    int pipHeight = pipRect.bottom - pipRect.top - TITLE_BAR_HEIGHT;

                    RECT originalRect;
                    GetWindowRect(originalHwnd, &originalRect);
                    int originalWidth = originalRect.right - originalRect.left;
                    int originalHeight = originalRect.bottom - originalRect.top;

                    int originalX = static_cast<int>(static_cast<float>(x) / pipWidth * originalWidth);
                    int originalY = static_cast<int>(static_cast<float>(y - TITLE_BAR_HEIGHT) / pipHeight *
                                                     originalHeight);

                    // 发送鼠标消息到原始窗口
                    SendMessage(originalHwnd, WM_LBUTTONDOWN, wParam, MAKELPARAM(originalX, originalY));
                }
            }
            break;
        }

        case WM_LBUTTONUP:
            if (g_appState.isDragging && g_appState.hwndBeingDragged == hwnd) {
                // 结束拖动
                ReleaseCapture();
                g_appState.isDragging = false;
                g_appState.hwndBeingDragged = nullptr;
            } else {
                // 鼠标抬起事件传递
                HWND originalHwnd = nullptr;
                for (const auto &pair: g_thumbnails) {
                    if (pair.second.hwndPip == hwnd) {
                        originalHwnd = pair.second.hwndOriginal;
                        break;
                    }
                }
                if (originalHwnd) {
                    RECT pipRect;
                    GetWindowRect(hwnd, &pipRect);
                    int pipWidth = pipRect.right - pipRect.left;
                    int pipHeight = pipRect.bottom - pipRect.top - TITLE_BAR_HEIGHT;

                    RECT originalRect;
                    GetWindowRect(originalHwnd, &originalRect);
                    int originalWidth = originalRect.right - originalRect.left;
                    int originalHeight = originalRect.bottom - originalRect.top;

                    int originalX = static_cast<int>(static_cast<float>(GET_X_LPARAM(lParam)) / pipWidth *
                                                     originalWidth);
                    int originalY = static_cast<int>(static_cast<float>(GET_Y_LPARAM(lParam) - TITLE_BAR_HEIGHT) /
                                                     pipHeight * originalHeight);

                    SendMessage(originalHwnd, WM_LBUTTONUP, wParam, MAKELPARAM(originalX, originalY));
                }
            }
            break;

        case WM_MOUSEMOVE:
            if (!(g_appState.isDragging && g_appState.hwndBeingDragged == hwnd)) {
                // 鼠标移动事件传递
                HWND originalHwnd = nullptr;
                for (const auto &pair: g_thumbnails) {
                    if (pair.second.hwndPip == hwnd) {
                        originalHwnd = pair.second.hwndOriginal;
                        break;
                    }
                }
                if (originalHwnd) {
                    RECT pipRect;
                    GetWindowRect(hwnd, &pipRect);
                    int pipWidth = pipRect.right - pipRect.left;
                    int pipHeight = pipRect.bottom - TITLE_BAR_HEIGHT - pipRect.top;

                    RECT originalRect;
                    GetWindowRect(originalHwnd, &originalRect);
                    int originalWidth = originalRect.right - originalRect.left;
                    int originalHeight = originalRect.bottom - originalRect.top;

                    int x = GET_X_LPARAM(lParam);
                    int y = GET_Y_LPARAM(lParam);
                    int originalX = static_cast<int>(static_cast<float>(x) / pipWidth * originalWidth);
                    int originalY = static_cast<int>(static_cast<float>(y - TITLE_BAR_HEIGHT) / pipHeight *
                                                     originalHeight);

                    SendMessage(originalHwnd, WM_MOUSEMOVE, wParam, MAKELPARAM(originalX, originalY));
                }
            } else {
                // 拖动画中画窗口
                POINT currentPos;
                GetCursorPos(&currentPos);

                int deltaX = currentPos.x - g_appState.dragStartScreenPos.x;
                int deltaY = currentPos.y - g_appState.dragStartScreenPos.y;

                int newX = g_appState.windowStartRect.left + deltaX;
                int newY = g_appState.windowStartRect.top + deltaY;

                int screenWidth = GetSystemMetrics(SM_CXSCREEN);
                int screenHeight = GetSystemMetrics(SM_CYSCREEN);

                RECT windowRect;
                GetWindowRect(hwnd, &windowRect);
                int windowWidth = windowRect.right - windowRect.left;
                int windowHeight = windowRect.bottom - windowRect.top;

                newX = std::max(0, std::min(newX, screenWidth - windowWidth));
                newY = std::max(0, std::min(newY, screenHeight - windowHeight));

                SetWindowPos(hwnd, NULL, newX, newY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }
            break;


// 在你的消息处理中
        case WM_IME_COMPOSITION: {
            HIMC himc = ImmGetContext(hwnd);
            if (himc) {
                LONG_PTR resultLen = ImmGetCompositionString(himc, GCS_RESULTSTR, nullptr, 0);
                if (resultLen > 0) {
                    std::vector<WCHAR> result(resultLen / sizeof(WCHAR) + 1);
                    ImmGetCompositionString(himc, GCS_RESULTSTR, result.data(), resultLen);
                    result[resultLen / sizeof(WCHAR)] = 0;
                    HWND originalHwnd = FindOriginalWindow(hwnd);
                    if (originalHwnd) {
                        for (auto wch: result) {
                            SendUnicodeChar(originalHwnd, wch);
                        }
                    }
                }
                ImmReleaseContext(hwnd, himc);
            }
            break;
        }
        case WM_CHAR: {
            HWND originalHwnd = FindOriginalWindow(hwnd);
            if (originalHwnd) {
                SendMessage(originalHwnd, msg, wParam, lParam);
            }
            break;
        }


        case WM_NCHITTEST: {
            // 允许拖动标题栏
            LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);
            if (hit == HTCLIENT) {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ScreenToClient(hwnd, &pt);

                if (pt.y < TITLE_BAR_HEIGHT && pt.x < (CLOSE_BUTTON_SIZE + 10)) {
                    return HTCAPTION;
                }
            }
            return hit;
        }

        case WM_PAINT: {
            // 绘制标题栏
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            HFONT hFont = CreateFontW(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS,
                                      CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
            SelectObject(hdc, hFont);

            RECT titleRect;
            GetClientRect(hwnd, &titleRect);
            titleRect.bottom = TITLE_BAR_HEIGHT;

            TRIVERTEX vertex[2] = {
                    {titleRect.left,  titleRect.top,    240, 240, 240, 240},
                    {titleRect.right, titleRect.bottom, 200, 200, 200, 255}};
            GRADIENT_RECT gRect = {0, 1};
            GradientFill(hdc, vertex, 2, &gRect, 1, GRADIENT_FILL_RECT_V);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(0, 0, 0));
            DrawTextW(hdc, L"PIP Window", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            EndPaint(hwnd, &ps);
            DeleteObject(hFont);
            break;
        }

        case WM_CREATE: {
            // 创建状态栏
            g_appState.hwndStatusBar = CreateWindowExW(
                    0, STATUSCLASSNAMEW, L"Status Bar",
                    WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                    0, 0, 100, STATUS_BAR_HEIGHT,
                    hwnd, NULL, GetModuleHandle(NULL), NULL);
            break;
        }

        case WM_SIZE: {
            if (g_appState.isResizing) break;
            // 调整缩略图和状态栏大小
            for (auto &pair: g_thumbnails) {
                if (pair.second.hwndPip == hwnd) {
                    RECT destRect;
                    GetClientRect(hwnd, &destRect);
                    destRect.top += TITLE_BAR_HEIGHT;

                    DWM_THUMBNAIL_PROPERTIES props = {};
                    props.dwFlags = DWM_TNP_RECTDESTINATION;
                    props.rcDestination = destRect;

                    DwmUpdateThumbnailProperties(pair.second.hThumbnail, &props);
                    break;
                }
            }
            RECT windowRect;
            GetClientRect(hwnd, &windowRect);
            int width = windowRect.right - windowRect.left;
            int height = windowRect.bottom - windowRect.top;

            RECT statusRect = {0, height - STATUS_BAR_HEIGHT, width, height};
            MoveWindow(g_appState.hwndStatusBar, statusRect.left, statusRect.top, statusRect.right - statusRect.left,
                       STATUS_BAR_HEIGHT, TRUE);
            break;
        }

        case WM_SIZING: {
            if (g_appState.isResizing) return TRUE;
            g_appState.isResizing = true;

            ThumbnailInfo *info = nullptr;
            for (auto &pair: g_thumbnails) {
                if (pair.second.hwndPip == hwnd) {
                    info = &pair.second;
                    break;
                }
            }

            if (info) {
                RECT *pRect = (RECT *) lParam;
                const int titleHeight = TITLE_BAR_HEIGHT;

                int width = pRect->right - pRect->left;
                int height = pRect->bottom - pRect->top - titleHeight;

                switch (wParam) {
                    case WMSZ_LEFT:
                    case WMSZ_RIGHT:
                        height = static_cast<int>(width / info->aspectRatio);
                        pRect->bottom = pRect->top + titleHeight + height;
                        break;

                    case WMSZ_TOP:
                    case WMSZ_BOTTOM:
                        width = static_cast<int>(height * info->aspectRatio);
                        pRect->right = pRect->left + width;
                        break;

                    default: {
                        bool widthDominant = (static_cast<float>(width) / height) > info->aspectRatio;
                        if (widthDominant) {
                            height = static_cast<int>(width / info->aspectRatio);
                        } else {
                            width = static_cast<int>(height * info->aspectRatio);
                        }

                        if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_TOPRIGHT) {
                            pRect->top = pRect->bottom - titleHeight - height;
                        } else {
                            pRect->bottom = pRect->top + titleHeight + height;
                        }

                        if (wParam == WMSZ_TOPLEFT || wParam == WMSZ_BOTTOMLEFT) {
                            pRect->left = pRect->right - width;
                        } else {
                            pRect->right = pRect->left + width;
                        }
                    }
                }
            }
            RECT windowRect;
            GetClientRect(hwnd, &windowRect);
            int width = windowRect.right - windowRect.left;
            int height = windowRect.bottom - windowRect.top;

            RECT statusRect = {0, height - STATUS_BAR_HEIGHT, width, height};
            MoveWindow(g_appState.hwndStatusBar, statusRect.left, statusRect.top, statusRect.right - statusRect.left,
                       STATUS_BAR_HEIGHT, TRUE);

            g_appState.isResizing = false;
            return TRUE;
        }

        case WM_GETMINMAXINFO: {
            // 设置最小尺寸
            MINMAXINFO *mmi = (MINMAXINFO *) lParam;
            mmi->ptMinTrackSize.x = 200;
            mmi->ptMinTrackSize.y = (int) (200 / 16.0f * 9.0f) + TITLE_BAR_HEIGHT;
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 主窗口过程
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            // 清理所有缩略图
            for (auto &pair: g_thumbnails) {
                DwmUnregisterThumbnail(pair.second.hThumbnail);
                DestroyWindow(pair.second.hwndPip);
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
                }
            }
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 注册窗口类
bool RegisterWindowClasses(HINSTANCE hInstance) {
    WNDCLASSW wc = {};

    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MAIN_WINDOW_CLASS;
    if (!RegisterClassW(&wc)) return false;

    wc.lpfnWndProc = PipWndProc;
    wc.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszClassName = PIP_WINDOW_CLASS;
    return RegisterClassW(&wc) != 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
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

