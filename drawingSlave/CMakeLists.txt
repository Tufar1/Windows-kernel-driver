cmake_minimum_required(VERSION 3.20)
project(usermode LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

add_executable(usermode
    ../main.cpp

    imgui/imgui.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_demo.cpp

    imgui/imgui_impl_win32.cpp
    imgui/imgui_impl_dx11.cpp
)

target_include_directories(usermode PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/imgui
)

target_compile_features(usermode PRIVATE cxx_std_23)
target_compile_definitions(usermode PRIVATE _AMD64_ _WIN64)

target_link_libraries(usermode PRIVATE
    d3d11 d3dcompiler dwmapi
)
