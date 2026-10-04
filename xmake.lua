set_xmakever("2.8.2")

set_project("PerfUI")
set_version("0.1.0")

set_warnings("allextra")

add_rules("mode.release", "mode.debug")
add_rules("plugin.vsxmake.autoupdate")

add_requires("commonlibsse-ng", {optional = true})

target("PerfUI")
    set_default(false)
    set_kind("shared")
    set_languages("c++20")
    add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", "PERFUI_BUILD_DLL")
    add_packages("commonlibsse-ng")

    add_includedirs("include")
    add_includedirs("third_party/imgui")
    add_includedirs("third_party/imgui/backends")
    add_includedirs("third_party/pugixml")
    add_includedirs("src/markup")
    add_includedirs("src")

    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "src/markup/*.cpp",
        "src/backends/imgui/*.cpp",
        "src/skyrim/*.cpp",
        "third_party/pugixml/pugixml.cpp",
        "third_party/imgui/imgui.cpp",
        "third_party/imgui/imgui_draw.cpp",
        "third_party/imgui/imgui_tables.cpp",
        "third_party/imgui/imgui_widgets.cpp",
        "third_party/imgui/backends/imgui_impl_win32.cpp",
        "third_party/imgui/backends/imgui_impl_dx11.cpp"
    )

    add_syslinks("d3d11", "dxgi", "d3dcompiler", "user32", "gdi32")
    set_pcxxheader("src/skyrim/Pch.h")

    after_build(function (target)
        local projectdir = os.projectdir()
        local sdk_include = path.join(projectdir, "sdk", "include", "PerfUI")
        local sdk_lib = path.join(projectdir, "sdk", "lib")
        os.mkdir(sdk_include)
        os.mkdir(sdk_lib)
        os.cp(path.join(projectdir, "include", "PerfUI", "*"), sdk_include)
        local target_dir = target:targetdir()
        local implib = path.join(target_dir, "PerfUI.lib")
        if os.isfile(implib) then
            os.cp(implib, path.join(sdk_lib, "PerfUI.lib"))
        end
    end)

target("PerfUI_Test_Independence")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
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
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/LayoutTest.cpp"
    )

target("PerfUI_Bench_ImGui")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/imgui", "third_party/imgui", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "src/backends/imgui/ImGuiRenderBackend.cpp",
        "third_party/imgui/imgui.cpp",
        "third_party/imgui/imgui_draw.cpp",
        "third_party/imgui/imgui_tables.cpp",
        "third_party/imgui/imgui_widgets.cpp",
        "tests/ImGuiBenchmark.cpp"
    )

target("PerfUI_Markup")
    set_kind("static")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_includedirs("include", "third_party/pugixml", "src/markup", "src")
    add_files(
        "src/markup/*.cpp",
        "third_party/pugixml/pugixml.cpp"
    )

target("PerfUI_Test_Markup")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src/markup", "third_party/pugixml", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "src/markup/*.cpp",
        "third_party/pugixml/pugixml.cpp",
        "tests/MarkupTest.cpp"
    )

