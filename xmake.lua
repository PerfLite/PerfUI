set_xmakever("2.8.2")

set_project("PerfUI")
set_version("0.1.0")

set_languages("c++23")
set_warnings("allextra")

add_rules("mode.release", "mode.debug")
add_rules("plugin.vsxmake.autoupdate")

add_requires("commonlibsse-ng", {optional = true})

target("PerfUI")
    set_default(false)
    set_kind("shared")
    add_packages("commonlibsse-ng")

    add_includedirs("include")
    add_includedirs("third_party/imgui")
    add_includedirs("third_party/imgui/backends")
    add_includedirs("src")

    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "src/backends/imgui/*.cpp",
        "src/skyrim/*.cpp",
        "third_party/imgui/imgui.cpp",
        "third_party/imgui/imgui_draw.cpp",
        "third_party/imgui/imgui_tables.cpp",
        "third_party/imgui/imgui_widgets.cpp",
        "third_party/imgui/backends/imgui_impl_win32.cpp",
        "third_party/imgui/backends/imgui_impl_dx11.cpp"
    )

    add_syslinks("d3d11", "dxgi", "d3dcompiler", "user32", "gdi32")
    set_pcxxheader("src/skyrim/Pch.h")

target("PerfUI_Test_Independence")
    set_kind("binary")
    set_languages("c++20")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/BackendIndependenceTest.cpp"
    )

target("PerfUI_Test_Layout")
    set_kind("binary")
    set_languages("c++20")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/LayoutTest.cpp"
    )

