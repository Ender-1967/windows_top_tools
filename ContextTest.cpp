#include <iostream>
#include <windows.h>
#include <shellapi.h> // For ShellExecute

// 定义更改 DPI 的函数
bool SetGlobalDPI(int dpiX, int dpiY) {
    // 构建控制面板中显示设置的路径
    std::wstring displaySettingsPath = L"control.exe /name Microsoft.Display";

    // 使用 ShellExecute 打开显示设置
    HINSTANCE result = ShellExecute(NULL, L"open", displaySettingsPath.c_str(), NULL, NULL, SW_SHOWNORMAL);

    if ((INT_PTR)result > 32) {
        std::cout << "已尝试打开显示设置，请手动更改 DPI。" << std::endl;
        return true; // 成功打开显示设置，但实际更改需要用户手动操作
    } else {
        std::cerr << "打开显示设置失败，错误代码: " << (int)result << std::endl;
        return false;
    }

    // 注意：无法通过标准 API 直接以编程方式更改全局 DPI 并立即生效。
    // 通常需要修改注册表并重启，但这涉及到系统级的更改，风险较高，
    // 并且可能被操作系统策略限制。

    // 以下是一些相关的注册表键，但直接修改不推荐：
    // HKEY_CURRENT_CONFIG\Display\Settings\LogPixels
    // HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Hardware Profiles\Current\Software\Fonts\LogPixels

    // Windows 10/11 引入了更复杂的 DPI 感知机制和每监视器 DPI 设置。
    // 直接修改注册表可能不会按预期工作，并且可能导致系统不稳定。

    // 更好的做法是引导用户到显示设置中进行更改。
}

int main() {
    int targetDPI = 144; // 例如，设置为 144 DPI (150% 缩放)

    std::cout << "尝试打开显示设置以更改 DPI..." << std::endl;
    if (SetGlobalDPI(targetDPI, targetDPI)) {
        std::cout << "请在显示设置中手动调整缩放比例。" << std::endl;
    } else {
        std::cerr << "更改 DPI 失败。" << std::endl;
    }

    return 0;
}