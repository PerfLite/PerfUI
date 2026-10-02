cmake_minimum_required(VERSION 3.20)
project(PerfUI VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Optimization & warning flags
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8 /EHsc /MP)
endif()

# ---------------------------------------------------------------------------
# 1. PerfUI_Core (Static Library - Pure C++20, Zero external dependencies)
# ---------------------------------------------------------------------------
add_library(PerfUI_Core STATIC
    include/PerfUI/Types.h
    include/PerfUI/Layout.h
    include/PerfUI/LayoutEngine.h
    include/PerfUI/UIRenderBackend.h
    include/PerfUI/UIElement.h
    include/PerfUI/UIContext.h
    include/PerfUI/Panel.h
    include/PerfUI/Text.h
    include/PerfUI/Button.h
    include/PerfUI/ScrollView.h
    include/PerfUI/Checkbox.h
    include/PerfUI/ProgressBar.h
    include/PerfUI/Slider.h
    include/PerfUI/Image.h
    include/PerfUI/Theme.h
    include/PerfUI/Animation.h
    include/PerfUI/TabBar.h
    include/PerfUI/ComboBox.h
    include/PerfUI/TextInput.h
    include/PerfUI/SettingsView.h
    include/PerfUI/ConfigManager.h
    include/PerfUI/Toast.h
    include/PerfUI/ContextMenu.h
    include/PerfUI/ModalDialog.h
    include/PerfUI/JournalWindow.h
    include/PerfUI/MainMenuWindow.h
    include/PerfUI/PerfUI.h
    src/core/UIElement.cpp
    src/core/UIContext.cpp
    src/core/ConfigManager.cpp
    src/layout/LayoutEngine.cpp
    src/widgets/Panel.cpp
    src/widgets/Text.cpp
    src/widgets/Button.cpp
    src/widgets/ScrollView.cpp
    src/widgets/Checkbox.cpp
    src/widgets/ProgressBar.cpp
    src/widgets/Slider.cpp
    src/widgets/Image.cpp
    src/widgets/TabBar.cpp
    src/widgets/ComboBox.cpp
    src/widgets/TextInput.cpp
    src/widgets/SettingsView.cpp
    src/widgets/Toast.cpp
    src/widgets/ContextMenu.cpp
    src/widgets/ModalDialog.cpp
    src/widgets/JournalWindow.cpp
    src/widgets/MainMenuWindow.cpp
)

target_include_directories(PerfUI_Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ---------------------------------------------------------------------------
# 2. PerfUI_Backend_ImGui (Static Library - ImGui Renderer Bridge)
# ---------------------------------------------------------------------------
add_library(PerfUI_Backend_ImGui STATIC
    src/backends/imgui/ImGuiRenderBackend.h
    src/backends/imgui/ImGuiRenderBackend.cpp
    third_party/imgui/imgui.h
    third_party/imgui/imgui.cpp
    third_party/imgui/imgui_draw.cpp
    third_party/imgui/imgui_tables.cpp
    third_party/imgui/imgui_widgets.cpp
)

target_include_directories(PerfUI_Backend_ImGui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/backends/imgui>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui>
)

target_link_libraries(PerfUI_Backend_ImGui PUBLIC
    PerfUI_Core
)

# ---------------------------------------------------------------------------
# 3. PerfUI_Sandbox (Desktop Standalone Test Runner)
# ---------------------------------------------------------------------------
add_executable(PerfUI_Sandbox WIN32
    examples/sandbox/main.cpp
    third_party/imgui/backends/imgui_impl_win32.h
    third_party/imgui/backends/imgui_impl_win32.cpp
    third_party/imgui/backends/imgui_impl_dx11.h
    third_party/imgui/backends/imgui_impl_dx11.cpp
)

target_include_directories(PerfUI_Sandbox PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends
)

target_link_libraries(PerfUI_Sandbox PRIVATE
    PerfUI_Backend_ImGui
    d3d11
    dxgi
    d3dcompiler
)

# ---------------------------------------------------------------------------
# 4. PerfUI_Test_Independence (Headless Core & Mock Backend Test Suite)
# ---------------------------------------------------------------------------
add_executable(PerfUI_Test_Independence
    tests/BackendIndependenceTest.cpp
    src/backends/mock/MockRenderBackend.h
)

target_include_directories(PerfUI_Test_Independence PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/mock
)

target_link_libraries(PerfUI_Test_Independence PRIVATE
    PerfUI_Core
)

# ---------------------------------------------------------------------------
# 5. PerfUI_Test_Layout (Layout Engine, Flexbox & Focus Test Suite + Bench)
# ---------------------------------------------------------------------------
add_executable(PerfUI_Test_Layout
    tests/LayoutTest.cpp
    src/backends/mock/MockRenderBackend.h
)

target_include_directories(PerfUI_Test_Layout PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/mock
)

target_link_libraries(PerfUI_Test_Layout PRIVATE
    PerfUI_Core
)


