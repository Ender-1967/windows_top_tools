cmake_minimum_required(VERSION 3.29)
project(window_top_tools)

set(CMAKE_CXX_STANDARD 20)
# 添加 dwmapi.lib 库
#link_libraries(dwmapi)
#link_libraries(gdi32)
#link_libraries(user32)


add_executable(window_top_tools
        main.cpp
        )
target_link_libraries(window_top_tools PRIVATE
        dwmapi
        gdi32
        user32
        msimg32  # 添加这一行
        imm32
)