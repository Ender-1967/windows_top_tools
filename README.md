# 文件组织和说明：

* main.cpp: 程序入口点 (WinMain). 负责注册窗口类，创建主窗口，处理主消息循环。
* pip_window.h / pip_window.cpp: 处理画中画窗口的创建和消息处理 (CreatePipWindow, PipWndProc). 这包括窗口的拖动、大小调整、关闭按钮、鼠标事件传递、键盘事件传递、以及与缩略图的交互。
* main_window.h / main_window.cpp: 处理主窗口的消息 (MainWndProc). 主要负责热键处理（注册缩略图）、主窗口的状态栏更新，以及程序级别的资源清理。
* thumbnail_manager.h / thumbnail_manager.cpp: 管理缩略图的创建、更新和销毁 (RegisterThumbnail, UnregisterThumbnail). 还负责获取原始窗口句柄、绘制原始窗口边框、更新状态栏等功能。
* input_buffer_window.h / input_buffer_window.cpp: 创建用于接收中文输入的隐藏窗口 (CreateInputBufferWindow). 这个窗口用于解决画中画窗口中输入法的问题。
* global_state.h / global_state.cpp: 存放全局数据结构和变量 (ThumbnailInfo, AppState, g_thumbnails, g_appState, 全局常量). global_state.cpp 负责初始化这些全局变量。
* utils.h / utils.cpp: 存放一些辅助函数 (RegisterWindowClasses, IsInTitleBar).

## 重构要点：
* 模块化： 代码被分解成更小的、更易于管理的模块，每个模块负责特定的功能。
* 职责分离： 每个文件中的函数都有明确的职责，减少了函数之间的耦合。
* 清晰的接口： 头文件定义了模块之间的接口，使得代码更易于理解和维护。
* 全局状态管理： global_state.h 和 global_state.cpp 用于集中管理全局数据，避免全局变量的滥用。
* 代码可读性： 代码结构更加清晰，注释更加完善，提高了代码的可读性。