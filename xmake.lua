set_xmakever("2.8.2")

set_project("PerfUI")
set_version("0.1.0")

set_warnings("allextra")

add_rules("mode.release", "mode.debug")
add_rules("plugin.vsxmake.autoupdate")
add_includedirs("third_party/nanosvg")
add_includedirs("modules/postfx/include")

add_requires("commonlibsse-ng", {optional = true})

option("perfui_lua")
    set_default(true)
    set_showmenu(true)
    set_description("Build the PerfUI_Lua module (Lua 5.4 scripting)")
option_end()

option("perfui_postfx")
    set_default(true)
    set_showmenu(true)
    set_description("Build the PerfUI_PostFx module (D3D11 Backdrop Blur & Glassmorphism)")
option_end()

target("PerfUI")
    set_default(true)
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN", "PERFUI_BUILD_DLL")
    add_packages("commonlibsse-ng")

    if has_config("perfui_lua") then
        add_defines("PERFUI_ENABLE_LUA")
        add_includedirs("modules/lua/include", "third_party/lua")
        add_files("modules/lua/src/*.cpp")
        if is_plat("windows") then
            add_files("third_party/lua/*.c", { cflags = "/w" })
        else
            add_files("third_party/lua/*.c", { cflags = "-w" })
        end
    end

    if has_config("perfui_postfx") then
        add_defines("PERFUI_ENABLE_POSTFX")
        add_includedirs("modules/postfx/include")
        add_files("modules/postfx/src/*.cpp")
    end

    add_includedirs("include")
    add_includedirs("third_party/imgui")
    add_includedirs("third_party/imgui/backends")
    add_includedirs("third_party/pugixml")
    add_includedirs("third_party/nanosvg")
add_includedirs("modules/postfx/include")
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
        os.cp(path.join(projectdir, "modules", "lua", "include", "PerfUI", "*"), sdk_include)
        os.cp(path.join(projectdir, "modules", "postfx", "include", "PerfUI", "*"), sdk_include)
        local target_dir = target:targetdir()
        local implib = path.join(target_dir, "PerfUI.lib")
        if os.isfile(implib) then
            os.cp(implib, path.join(sdk_lib, "PerfUI.lib"))
        end
        local pdb = path.join(target_dir, "PerfUI.pdb")
        if os.isfile(pdb) then
            os.cp(pdb, path.join(sdk_lib, "PerfUI.pdb"))
        end
        local dll = path.join(target_dir, "PerfUI.dll")
        if os.isfile(dll) then
            os.cp(dll, path.join(sdk_lib, "PerfUI.dll"))
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

target("PerfUI_Test_Animation")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/AnimationTest.cpp"
    )

target("PerfUI_Test_WorldAnchor")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/WorldAnchorTest.cpp"
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

target("PerfUI_Test_Overlay")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/OverlayTest.cpp"
    )

target("PerfUI_Test_Diagnostics")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/DiagnosticsTest.cpp"
    )

target("PerfUI_Test_Texture")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/TextureTest.cpp"
    )

target("PerfUI_Test_MapCanvas")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    if is_plat("windows") then
        add_cxflags("/utf-8", { force = true })
    end
    add_includedirs("include", "src/backends/mock", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/MapCanvasTest.cpp"
    )

target("PerfUI_Test_Style")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    if is_plat("windows") then
        add_cxflags("/utf-8", { force = true })
    end
    add_includedirs("include", "src/backends/mock", "third_party/pugixml", "src/markup", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "src/markup/*.cpp",
        "third_party/pugixml/pugixml.cpp",
        "tests/StyleSystemTest.cpp"
    )

target("PerfUI_Test_Svg")
    set_kind("binary")
    set_languages("c++20")
    add_defines("PERFUI_STATIC")
    add_syslinks("user32")
    add_includedirs("include", "src/backends/mock", "third_party/nanosvg", "src")
    add_files(
        "src/core/*.cpp",
        "src/layout/*.cpp",
        "src/widgets/*.cpp",
        "tests/SvgTest.cpp"
    )



if has_config("perfui_lua") then
    target("PerfUI_Lua")
        set_kind("static")
        set_languages("c++20", "c11")
        add_defines("PERFUI_STATIC")
        add_includedirs("modules/lua/include", "include", "third_party/lua")
        add_files("modules/lua/src/*.cpp")
        if is_plat("windows") then
            add_files("third_party/lua/*.c", { cflags = "/w" })
        else
            add_files("third_party/lua/*.c", { cflags = "-w" })
        end

    target("PerfUI_Test_Lua")
        set_kind("binary")
        set_languages("c++20")
        add_defines("PERFUI_STATIC")
        add_syslinks("user32")
        add_deps("PerfUI_Lua")
        add_includedirs("include", "modules/lua/include", "src/backends/mock", "src")
        add_files(
            "src/core/*.cpp",
            "src/layout/*.cpp",
            "src/widgets/*.cpp",
            "modules/lua/tests/ScriptHostTest.cpp"
        )
        add_tests("script_host")

    target("PerfUI_Test_LuaUi")
        set_kind("binary")
        set_languages("c++20")
        add_defines("PERFUI_STATIC")
        add_syslinks("user32")
        add_deps("PerfUI_Lua")
        add_includedirs("include", "modules/lua/include", "src/backends/mock", "src/markup", "third_party/pugixml", "src")
        add_files(
            "src/core/*.cpp",
            "src/layout/*.cpp",
            "src/widgets/*.cpp",
            "src/markup/*.cpp",
            "third_party/pugixml/pugixml.cpp",
            "modules/lua/tests/LuaUiApiTest.cpp"
        )
        add_tests("lua_ui_api")
end
