// pip_window.cpp

#include "../include/pip_window.h"
#include "../include/thumbnail_manager.h"
#include "../include/global_state.h"
#include "../include/utils.h"
#include "input_buffer_window.h"
#include <iostream>
#include <windowsx.h>

// 声明全局变量 g_originalWindowRects
extern std::map<HWND, RECT> g_originalWindowRects;

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
            WS_POPUP | WS_VISIBLE | WS_VISIBLE,
            srcRect.left, srcRect.top, width, height,
            NULL, NULL, GetModuleHandle(NULL), NULL);

    // 设置窗口圆角
    HRGN hRgn = CreateRoundRectRgn(0, 0, width, height, 8, 8);
    SetWindowRgn(hwnd, hRgn, TRUE);
    DeleteObject(hRgn);

    // 设置背景透明度,主窗口透明
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


// 画中画窗口过程
LRESULT CALLBACK PipWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // 关闭按钮
                HWND originalHwnd = GetOriginalWindowHandle(hwnd);
                UnregisterThumbnail(originalHwnd);
                //DestroyWindow(hwnd); //  在UnregisterThumbnail已经调用
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

            if (g_appState.isResizing) break; // 调整期间不传递

            if (IsInTitleBar(hwnd, x, y)) {
                // 标题栏拖动
                SetCapture(hwnd);
                g_appState.isDragging = true;
                g_appState.hwndBeingDragged = hwnd;
                GetCursorPos(&g_appState.dragStartScreenPos);
                GetWindowRect(hwnd, &g_appState.windowStartRect);
            }
            else {
                // 鼠标点击事件传递到原始窗口
                HWND originalHwnd = GetOriginalWindowHandle(hwnd);
                if (originalHwnd) {
                    // 坐标转换
                    ThumbnailInfo* info = nullptr;
                    for (auto& pair : g_thumbnails) {
                        if (pair.second.hwndPip == hwnd) {
                            info = &pair.second;
                            break;
                        }
                    }
                    if (!info) break; // 如果找不到 ThumbnailInfo，则退出

                    RECT pipRect;
                    GetWindowRect(hwnd, &pipRect);
                    int pipWidth = pipRect.right - pipRect.left;
                    int pipHeight = pipRect.bottom - pipRect.top - TITLE_BAR_HEIGHT;

                    RECT originalRect;
                    GetWindowRect(originalHwnd, &originalRect);
                    int originalWidth = originalRect.right - originalRect.left;
                    int originalHeight = originalRect.bottom - originalRect.top;

                    // 使用保存的缩放比例
                    int originalX = static_cast<int>(static_cast<float>(x) / pipWidth * originalWidth);
                    int originalY = static_cast<int>(static_cast<float>(y - TITLE_BAR_HEIGHT) / pipHeight * originalHeight);

                    // 激活输入法，并将焦点设置到隐藏窗口
//                    HIMC hIMC = ImmGetContext(originalHwnd);
//                    if (hIMC) {
//                        HWND hwndInputBuffer = g_thumbnails[originalHwnd].hwndInputBuffer; // 获取隐藏窗口句柄
//                        ImmAssociateContext(originalHwnd, reinterpret_cast<HIMC>(hwndInputBuffer)); // 将输入法上下文与隐藏窗口关联
//                        SetFocus(hwndInputBuffer); // 设置焦点到隐藏窗口
//                        ImmReleaseContext(originalHwnd, hIMC);
//                    }

                    // 发送鼠标消息到原始窗口
                    PostMessage(originalHwnd, WM_LBUTTONDOWN, wParam, MAKELPARAM(originalX, originalY));
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
                HWND originalHwnd = GetOriginalWindowHandle(hwnd);
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

                    // 发送鼠标消息到原始窗口
                    PostMessage(originalHwnd, WM_LBUTTONUP, wParam, MAKELPARAM(originalX, originalY));
                }
            }
            break;

//        case WM_MOUSEWHEEL:
        case WM_MOUSEMOVE:
            if (g_appState.isResizing) break; // 调整期间不传递
            if (!(g_appState.isDragging && g_appState.hwndBeingDragged == hwnd)) {
                // 鼠标移动事件传递
                HWND originalHwnd = GetOriginalWindowHandle(hwnd);
                if (originalHwnd) {
                    POINT currentPos;
                    GetCursorPos(&currentPos);
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
                    PostMessage(originalHwnd, msg, wParam, MAKELPARAM(originalX, originalY));
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
        case WM_MOUSEWHEEL: {
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd && IsWindow(originalHwnd)) {
                WORD keyFlags = LOWORD(wParam);               // 低位
                if(keyFlags == MK_CONTROL){
                    printf("zoom\n");

                    break;
                }
                // 提取滚轮增量
                int delta = GET_WHEEL_DELTA_WPARAM(wParam);

                // 跨线程输入处理
                DWORD srcTID = GetWindowThreadProcessId(originalHwnd, NULL);
                DWORD currTID = GetCurrentThreadId();
                bool attached = (srcTID != currTID) && AttachThreadInput(currTID, srcTID, TRUE);

                if(delta > 0)
                    PostMessage(originalHwnd, WM_VSCROLL, SB_LINEUP,0);
                else
                    PostMessage(originalHwnd, WM_VSCROLL, SB_LINEDOWN,0);

                // 恢复线程状态
                if (attached) AttachThreadInput(currTID, srcTID, FALSE);
            }
            break;
        }
        //添加 WM_CAPTURECHANGED 处理 (处理调整大小)
        case WM_CAPTURECHANGED: {
            g_appState.isDragging = false;
            g_appState.hwndBeingDragged = nullptr;
            break;
        }


        case WM_KEYDOWN:
        case WM_KEYUP: {
            // 键盘事件传递到原始窗口，检查是否为功能键
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd) {
                //有关于输入法的更改
                // 检查是否为功能键 (例如，方向键，Enter)
                if (wParam == VK_LEFT || wParam == VK_RIGHT ||
                    wParam == VK_UP || wParam == VK_DOWN ||
                    wParam == VK_RETURN || wParam == VK_ESCAPE ||
                    wParam == VK_TAB || wParam == VK_CONTROL ||
                    wParam == VK_SHIFT || wParam == VK_MENU ||
                    wParam == VK_SPACE || wParam == VK_BACK ||
                    wParam == VK_DELETE) {
                    PostMessage(originalHwnd, msg, wParam, lParam);
                } else {
                    // 获取输入法上下文
                    HIMC hIMC = ImmGetContext(hwnd);
                    if (hIMC) {
                        // 检查输入法是否处于中文模式
                        if (ImmGetOpenStatus(hIMC)) {
                            //  如果输入法开启，不直接传递，通过 WM_IME_COMPOSITION 传递
                        } else {
                            // 如果输入法关闭，直接传递
                            PostMessage(originalHwnd, msg, wParam, lParam);
                        }
                        ImmReleaseContext(hwnd, hIMC);
                    } else {
                        PostMessage(originalHwnd, msg, wParam, lParam);
                    }
                }
            }
            break;
        }
        case WM_CHAR: {
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd) {
                PostMessage(originalHwnd, msg, wParam, lParam);
            }
            break;
        }

        case WM_NCHITTEST: {
            // 先检查是否在可拖动区域
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(hwnd, &pt);

            // 检查关闭按钮区域
            if (pt.y < TITLE_BAR_HEIGHT && pt.x < (CLOSE_BUTTON_SIZE + 10)) {
                return HTCLIENT; // 让按钮处理点击
            }

            // 检查标题栏区域
            if (pt.y < TITLE_BAR_HEIGHT) {
                return HTCAPTION;
            }

            // 模拟窗口边框
            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);
            int borderWidth = 5; // 边框宽度，可以根据需要调整
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);

            bool originalWindowResizable = false;
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd) {
                LONG style = GetWindowLong(originalHwnd, GWL_STYLE);
                if (style & WS_THICKFRAME) {
                    originalWindowResizable = true;
                }
            }
            if (originalWindowResizable) {
                if (pt.y <= borderWidth && pt.x <= borderWidth)
                    return HTTOPLEFT;
                if (pt.y <= borderWidth && pt.x >= windowRect.right - windowRect.left - borderWidth)
                    return HTTOPRIGHT;
                if (pt.y >= windowRect.bottom - windowRect.top - borderWidth && pt.x <= borderWidth)
                    return HTBOTTOMLEFT;
                if (pt.y >= windowRect.bottom - windowRect.top - borderWidth &&
                    pt.x >= windowRect.right - windowRect.left - borderWidth)
                    return HTBOTTOMRIGHT;
                if (pt.y <= borderWidth)
                    return HTTOP;
                if (pt.y >= windowRect.bottom - windowRect.top - borderWidth)
                    return HTBOTTOM;
                if (pt.x <= borderWidth)
                    return HTLEFT;
                if (pt.x >= windowRect.right - windowRect.left - borderWidth)
                    return HTRIGHT;
            }

            // 默认处理边框区域前，先检查原始窗口是否可调整大小

            // 默认处理边框区域
            LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);

            // 如果是边框区域，直接返回系统检测结果
            if (hit != HTCLIENT) {
                return hit;
            }

            return HTCLIENT;
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
            // 获取原始窗口标题并显示
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd) {
                auto it = g_thumbnails.find(originalHwnd);
                if (it != g_thumbnails.end()) {
                    DrawTextW(hdc, it->second.originalTitle.c_str(), -1, &titleRect,
                              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                } else {
                    DrawTextW(hdc, L"PIP Window", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                }
            } else {
                DrawTextW(hdc, L"PIP Window", -1, &titleRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

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

                    // 计算新的缩放比例
                    RECT originalRect;
                    GetWindowRect(pair.second.hwndOriginal, &originalRect);
                    float scaleX = (float)(destRect.right - destRect.left) / (originalRect.right - originalRect.left);
                    float scaleY = (float)(destRect.bottom - destRect.top) / (originalRect.bottom - originalRect.top);

                    // 更新缩略图属性
                    DWM_THUMBNAIL_PROPERTIES props = {};
                    props.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
                    props.rcDestination = destRect;
                    props.fVisible = TRUE;

                    DwmUpdateThumbnailProperties(pair.second.hThumbnail, &props);

                    // 保存新的缩放比例
                    pair.second.scaleX = scaleX;
                    pair.second.scaleY = scaleY;
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

//        case WM_SIZING: {
//            if (g_appState.isResizing) return TRUE;
//            g_appState.isResizing = true;
//
//            // 获取原始宽高比信息
//            ThumbnailInfo* info = nullptr;
//            for (auto& pair : g_thumbnails) {
//                if (pair.second.hwndPip == hwnd) {
//                    info = &pair.second;
//                    break;
//                }
//            }
//
//            if (!info) {
//                g_appState.isResizing = false;
//                return DefWindowProc(hwnd, msg, wParam, lParam);
//            }
//
//            RECT* pRect = (RECT*)lParam;
//            const float aspectRatio = info->aspectRatio;
//
//            // 计算可用区域（排除标题栏）
//            int titleHeight = TITLE_BAR_HEIGHT;
//            int contentHeight = (pRect->bottom - pRect->top) - titleHeight;
//
//            // 根据拖动方向强制保持宽高比
//            switch (wParam) {
//                case WMSZ_LEFT:      // 左侧
//                case WMSZ_RIGHT:     // 右侧
//                    contentHeight = static_cast<int>((pRect->right - pRect->left) / aspectRatio);
//                    pRect->bottom = pRect->top + titleHeight + contentHeight;
//                    break;
//
//                case WMSZ_TOP:       // 顶部
//                case WMSZ_BOTTOM:    // 底部
//                    pRect->right = pRect->left + static_cast<int>(contentHeight * aspectRatio);
//                    break;
//
//                case WMSZ_TOPLEFT:   // 左上角
//                case WMSZ_TOPRIGHT:  // 右上角
//                case WMSZ_BOTTOMLEFT:// 左下角
//                case WMSZ_BOTTOMRIGHT:// 右下角
//                {
//                    // 计算鼠标移动方向的主导轴
//                    bool widthDominant = (abs(pRect->right - pRect->left) > abs(pRect->bottom - pRect->top) * aspectRatio);
//                    if (widthDominant) {
//                        contentHeight = static_cast<int>((pRect->right - pRect->left) / aspectRatio);
//                        pRect->bottom = pRect->top + titleHeight + contentHeight;
//                    } else {
//                        pRect->right = pRect->left + static_cast<int>(contentHeight * aspectRatio);
//                    }
//                    break;
//                }
//            }
//
//            // 确保最小尺寸
//            MINMAXINFO mmi = {};
//            SendMessage(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
//            if ((pRect->right - pRect->left) < mmi.ptMinTrackSize.x) {
//                pRect->right = pRect->left + mmi.ptMinTrackSize.x;
//            }
//            if ((pRect->bottom - pRect->top) < mmi.ptMinTrackSize.y) {
//                pRect->bottom = pRect->top + mmi.ptMinTrackSize.y;
//            }
//
//            // 更新状态栏位置
//            RECT windowRect = *pRect;
//            int width = windowRect.right - windowRect.left;
//            int height = windowRect.bottom - windowRect.top;
//            RECT statusRect = {0, height - STATUS_BAR_HEIGHT, width, height};
//            MoveWindow(g_appState.hwndStatusBar, statusRect.left, statusRect.top,
//                       statusRect.right - statusRect.left, STATUS_BAR_HEIGHT, TRUE);
//
//            // 调用默认处理并重置状态
//            LRESULT result = DefWindowProc(hwnd, msg, wParam, lParam);
//            g_appState.isResizing = false;
//            return result;
//        }

        case WM_SIZING: {
            if (g_appState.isResizing) return TRUE;
            g_appState.isResizing = true;

            //  只调整窗口大小，不再限制宽高比
            LRESULT result = DefWindowProc(hwnd, msg, wParam, lParam);
            return result;
        }

        case WM_ENTERSIZEMOVE:
            g_appState.isResizing = true; // 开始调整
            break;


        case WM_EXITSIZEMOVE: {
            g_appState.isResizing = false; // 结束调整

            // 获取调整后的小窗口大小
            RECT newWindowRect;
            GetWindowRect(hwnd, &newWindowRect);
            int newWidth = newWindowRect.right - newWindowRect.left;
            int newHeight = newWindowRect.bottom - newWindowRect.top;

            // 获取原始窗口宽高比
            ThumbnailInfo* info = nullptr;
            for (auto& pair : g_thumbnails) {
                if (pair.second.hwndPip == hwnd) {
                    info = &pair.second;
                    break;
                }
            }
            if (info) {
                float aspectRatio = info->aspectRatio;

                // 计算画中画窗口的目标大小 (保持宽高比)
                int targetWidth = newWidth;
                int targetHeight = static_cast<int>(newWidth / aspectRatio) + TITLE_BAR_HEIGHT;

                // 同时调整小窗口和画中画窗口的大小
                SetWindowPos(hwnd, NULL, 0, 0, newWidth, newHeight,
                             SWP_NOZORDER | SWP_NOMOVE); // 调整小窗口
                SetWindowPos(hwnd, NULL, 0, 0, targetWidth, targetHeight,
                             SWP_NOZORDER | SWP_NOMOVE); // 调整画中画窗口

                // 更新缩略图目标区域
                RECT destRect;
                GetClientRect(hwnd, &destRect);
                destRect.top += TITLE_BAR_HEIGHT;

                DWM_THUMBNAIL_PROPERTIES props = {};
                props.dwFlags = DWM_TNP_RECTDESTINATION | DWM_TNP_VISIBLE;
                props.rcDestination = destRect;
                props.fVisible = TRUE;

                DwmUpdateThumbnailProperties(info->hThumbnail, &props);
            }

            break;
        }
        case WM_GETMINMAXINFO: {
            // 设置最小尺寸
            MINMAXINFO *mmi = (MINMAXINFO *) lParam;
            mmi->ptMinTrackSize.x = 200;
            mmi->ptMinTrackSize.y = (int) (200 / 16.0f * 9.0f) + TITLE_BAR_HEIGHT;
            break;
        }
        case WM_IME_STARTCOMPOSITION: {
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd) {
                HIMC hIMC = ImmGetContext(hwnd);
                if (hIMC) {
                    ImmAssociateContext(originalHwnd,
                                        reinterpret_cast<HIMC>(g_thumbnails[originalHwnd].hwndInputBuffer));
                    ImmReleaseContext(hwnd, hIMC);
                }
            }
            break;
        }
        case WM_IME_ENDCOMPOSITION: {
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd) {
                HIMC hIMC = ImmGetContext(hwnd);
                if (hIMC) {
                    ImmAssociateContext(originalHwnd,
                                        reinterpret_cast<HIMC>(g_thumbnails[originalHwnd].hwndInputBuffer));
                    ImmReleaseContext(hwnd, hIMC);
                }
            }
            break;
        }

        case WM_IME_COMPOSITION: {
            HWND originalHwnd = GetOriginalWindowHandle(hwnd);
            if (originalHwnd) {
                HIMC hIMC = ImmGetContext(hwnd);
                if (hIMC) {
                    // 检查是否有结果字符串
                    if (lParam & GCS_RESULTSTR) {
                        // 获取结果字符串的长度（包含终止符）
                        LONG resultStrLen = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
                        if (resultStrLen > 0) {
                            // 分配足够的内存来存储结果字符串（包括终止符）
                            std::wstring resultStr((resultStrLen / sizeof(wchar_t)) + 1, L'\0');

                            // 获取结果字符串（包括终止符）
                            ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, &resultStr[0],
                                                     resultStrLen + sizeof(wchar_t));

                            // 逐个发送字符到原始窗口
                            for (size_t i = 0; i < wcslen(resultStr.c_str()); i++) {
                                PostMessageW(originalHwnd, WM_IME_CHAR, (WPARAM) resultStr[i], 0);
                            }
                        }
                    }
                    ImmReleaseContext(hwnd, hIMC);
                }
            }
            break;
        }


        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}