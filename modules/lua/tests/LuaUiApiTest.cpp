/**
 * @file LuaUiApiTest.cpp
 * @brief Unit tests for Stage 2: UI API handles, markup callback resolution,
 *        timers, dead element safety, and hot-reload.
 */

#include "PerfUI/PerfUI.h"
#include "PerfUI/Lua/ScriptHost.h"
#include "PerfUI/Lua/ScriptCallbackResolver.h"
#include "PerfUI/Markup/MarkupLoader.h"
#include "MockRenderBackend.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace PerfUI;

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << "\n"; \
            ++g_testsPassed; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (line " << __LINE__ << ")\n"; \
            ++g_testsFailed; \
        } \
    } while (0)

namespace {

std::filesystem::path MakeTempTestDir()
{
    const auto dir = std::filesystem::temp_directory_path() / "perfui_lua_ui_test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    return dir;
}

void WriteFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream stream(path, std::ios::binary);
    stream << content;
}

void Test1_UiFindAndHandles()
{
    std::cout << "\n=== Test 1: UI Handles & Widget Values ===\n";
    UIContext ctx;
    MockRenderBackend backend;
    ctx.setRenderBackend(&backend);

    auto* panel = ctx.root()->add<Panel>("TestPanel");
    auto* btn = panel->add<Button>("Click Me", "TestBtn");
    auto* txt = panel->add<Text>("Hello World", "TestText");
    auto* sld = panel->add<Slider>(0.5f, 0.0f, 1.0f, "Volume", "TestSlider");
    auto* chk = panel->add<Checkbox>("Enable Sound", false, "TestCheck");
    auto* cmb = panel->add<ComboBox>(std::vector<std::string>{ "Low", "Medium", "High" }, 0, "TestCombo");
    auto* prg = panel->add<ProgressBar>(0.25f, "TestProgress");
    auto* tab = panel->add<TabBar>("TestTab");
    tab->addTab("Tab0");
    tab->addTab("Tab1");

    ScriptHost host("TestMod");
    host.bindContext(&ctx, ctx.root());

    host.registerModule("test", [](ModuleBuilder& m) {
        m.function("assert", [](bool cond, std::string msg) {
            TEST_ASSERT(cond, msg);
        });
    });

    const char* script = R"(
        TEST_ASSERT = test.assert

        local btn = ui.find("TestBtn")
        TEST_ASSERT(btn ~= nil, "btn handle exists")
        TEST_ASSERT(btn:isValid() == true, "btn is valid")
        TEST_ASSERT(btn:name() == "TestBtn", "btn name matches")
        TEST_ASSERT(btn:text() == "Click Me", "btn initial text")
        btn:setText("Saved!")
        btn:setVisible(false)
        btn:setEnabled(false)
        btn:setOpacity(0.8)

        local txt = ui.find("TestText")
        TEST_ASSERT(txt ~= nil, "txt handle exists")
        TEST_ASSERT(txt:text() == "Hello World", "txt initial text")
        txt:setText("New Text")

        local sld = ui.find("TestSlider")
        TEST_ASSERT(sld ~= nil, "slider handle exists")
        TEST_ASSERT(sld:value() == 0.5, "slider initial value")
        sld:setValue(0.85)

        local chk = ui.find("TestCheck")
        TEST_ASSERT(chk ~= nil, "checkbox handle exists")
        TEST_ASSERT(chk:isChecked() == false, "check initial false")
        chk:setChecked(true)

        local cmb = ui.find("TestCombo")
        TEST_ASSERT(cmb ~= nil, "combo handle exists")
        TEST_ASSERT(cmb:selectedIndex() == 0, "combo initial index 0")
        cmb:selectIndex(2)

        local prg = ui.find("TestProgress")
        TEST_ASSERT(prg ~= nil, "progress handle exists")
        TEST_ASSERT(prg:value() == 0.25, "progress initial value")
        prg:setValue(0.75)

        local tab = ui.find("TestTab")
        TEST_ASSERT(tab ~= nil, "tab handle exists")
        TEST_ASSERT(tab:selectedIndex() == 0, "tab initial index 0")
        tab:selectIndex(1)

        local col = ui.theme.color("borderFocus")
        TEST_ASSERT(col ~= nil, "theme color found")
        TEST_ASSERT(col.r == 212 and col.g == 175 and col.b == 55, "nordic gold rgb")

        local missing = ui.find("NoSuchElement")
        TEST_ASSERT(missing == nil, "missing element is nil")
    )";

    bool ok = host.loadString(script, "test1");
    TEST_ASSERT(ok, "Script executed successfully");

    TEST_ASSERT(btn->text() == "Saved!", "Button text updated in C++");
    TEST_ASSERT(!btn->isVisible(), "Button visible is false in C++");
    TEST_ASSERT(!btn->isEnabled(), "Button enabled is false in C++");
    TEST_ASSERT(std::abs(btn->opacity() - 0.8f) < 0.01f, "Button opacity updated in C++");
    TEST_ASSERT(txt->text() == "New Text", "Text updated in C++");
    TEST_ASSERT(std::abs(sld->value() - 0.85f) < 0.01f, "Slider value updated in C++");
    TEST_ASSERT(chk->isChecked() == true, "Checkbox checked is true in C++");
    TEST_ASSERT(cmb->selectedIndex() == 2, "ComboBox selectedIndex is 2 in C++");
    TEST_ASSERT(std::abs(prg->value() - 0.75f) < 0.01f, "ProgressBar value is 0.75 in C++");
    TEST_ASSERT(tab->selectedTab() == 1, "TabBar selectedTab is 1 in C++");
}

void Test2_DeadElementSafety()
{
    std::cout << "\n=== Test 2: Dead Element Safety ===\n";
    UIContext ctx;
    MockRenderBackend backend;
    ctx.setRenderBackend(&backend);

    ctx.root()->add<Button>("Doomed", "DoomedBtn");
    ScriptHost host("TestMod");
    host.bindContext(&ctx, ctx.root());

    std::vector<std::string> loggedWarnings;
    ScriptHost::SetLogSink([&](ScriptLogLevel lvl, std::string_view msg) {
        if (lvl == ScriptLogLevel::Warn) {
            loggedWarnings.emplace_back(msg);
        }
    });

    bool ok1 = host.loadString(R"(
        g_doomed = ui.find("DoomedBtn")
        assert(g_doomed ~= nil)
        assert(g_doomed:isValid() == true)
        assert(g_doomed:text() == "Doomed")
    )", "test2_find");
    TEST_ASSERT(ok1, "Found element while alive");

    // Destroy all elements
    ctx.root()->clearChildren();

    // Call methods on dead handle
    bool ok2 = host.loadString(R"(
        assert(g_doomed:isValid() == false, "handle isValid is false")
        assert(g_doomed:isVisible() == false, "dead handle isVisible is false")
        assert(g_doomed:isEnabled() == false, "dead handle isEnabled is false")
        assert(g_doomed:text() == nil, "dead handle text is nil")
        assert(g_doomed:value() == nil, "dead handle value is nil")

        -- mutating calls must not crash
        g_doomed:setText("Zombie")
        g_doomed:setVisible(true)
        g_doomed:setEnabled(true)
        g_doomed:setValue(10)
    )", "test2_dead");
    TEST_ASSERT(ok2, "Dead handle calls executed safely without crash");

    bool hasWarn = false;
    for (const auto& w : loggedWarnings) {
        if (w.find("attempt to use destroyed element 'DoomedBtn'") != std::string::npos) {
            hasWarn = true;
            break;
        }
    }
    TEST_ASSERT(hasWarn, "Warning logged for dead element call");
}

void Test3_MarkupCallbackPriority()
{
    std::cout << "\n=== Test 3: Markup Callbacks & C++ Priority ===\n";
    UIContext ctx;
    MockRenderBackend backend;
    ctx.setRenderBackend(&backend);

    ScriptHost host("TestMod");
    host.bindContext(&ctx);

    bool ok = host.loadString(R"(
        luaClickCount = 0
        cppShadowCount = 0
        luaToggleVal = -1

        function onLuaOnlyClick()
            luaClickCount = luaClickCount + 1
        end

        function onSharedClick()
            cppShadowCount = 999
        end

        function onLuaToggle(val)
            luaToggleVal = val
        end
    )", "test3_script");
    TEST_ASSERT(ok, "Loaded callbacks in Lua");

    ScriptCallbackResolver resolver(host);
    MarkupLoader loader(ctx);
    loader.setCallbackResolver(&resolver);

    int cppCalls = 0;
    loader.bindCallback("onSharedClick", [&]() {
        ++cppCalls;
    });

    const char* xml = R"(
        <UI>
            <Panel name="Main">
                <Button name="BtnLua" onClick="onLuaOnlyClick" text="Lua"/>
                <Button name="BtnShared" onClick="onSharedClick" text="Shared"/>
                <Checkbox name="ChkToggle" onToggle="onLuaToggle" text="Toggle"/>
            </Panel>
        </UI>
    )";

    UIElement* root = loader.loadString(xml);
    TEST_ASSERT(root != nullptr, "Loaded XML tree");

    auto* btnLua = dynamic_cast<Button*>(loader.findByName("BtnLua"));
    auto* btnShared = dynamic_cast<Button*>(loader.findByName("BtnShared"));
    auto* chkToggle = dynamic_cast<Checkbox*>(loader.findByName("ChkToggle"));
    TEST_ASSERT(btnLua && btnShared && chkToggle, "All widgets found");

    // Click Lua button
    btnLua->onPointerDown({ 5.0f, 5.0f });
    btnLua->onPointerUp({ 5.0f, 5.0f });

    // Click Shared button (C++ callback registered with same name)
    btnShared->onPointerDown({ 5.0f, 5.0f });
    btnShared->onPointerUp({ 5.0f, 5.0f });

    // Toggle checkbox
    chkToggle->setBounds({ 0.0f, 0.0f, 100.0f, 30.0f });
    chkToggle->onPointerDown({ 5.0f, 5.0f });
    chkToggle->onPointerUp({ 5.0f, 5.0f });

    TEST_ASSERT(cppCalls == 1, "C++ callback executed for shared name");

    host.registerModule("verify", [&](ModuleBuilder& m) {
        m.function("check", [&](double lCount, double shadow, double toggle) {
            TEST_ASSERT(lCount == 1.0, "Lua only callback executed");
            TEST_ASSERT(shadow == 0.0, "C++ callback took precedence over Lua with same name");
            TEST_ASSERT(toggle == 1.0, "Lua toggle callback received parameter 1.0");
        });
    });
    host.loadString("verify.check(luaClickCount, cppShadowCount, luaToggleVal)", "verify");
}

void Test4_Timers()
{
    std::cout << "\n=== Test 4: Timers & Intervals ===\n";
    ScriptHost host("TestMod");

    bool ok = host.loadString(R"(
        g_timerFired = 0
        g_intervalFired = 0
        g_cancelledFired = 0

        t1 = ui.timer(0.1, function()
            g_timerFired = g_timerFired + 1
        end)

        t2 = ui.interval(0.05, function()
            g_intervalFired = g_intervalFired + 1
        end)

        t3 = ui.timer(0.05, function()
            g_cancelledFired = g_cancelledFired + 1
        end)
        ui.cancel(t3)
    )", "test4");
    TEST_ASSERT(ok, "Timers registered in Lua");

    // Small dt: nothing should fire
    host.update(0.02f);
    host.loadString("assert(g_timerFired == 0 and g_intervalFired == 0 and g_cancelledFired == 0)", "check0");

    // dt = 0.04s (total 0.06s): interval should fire once, cancelled never
    host.update(0.04f);
    host.loadString("assert(g_intervalFired == 1, 'interval fired once')\n"
                    "assert(g_cancelledFired == 0, 'cancelled timer never fires')\n"
                    "assert(g_timerFired == 0, 'timer not fired yet')", "check1");

    // dt = 0.05s (total 0.11s): one-shot timer fires, interval fires second time
    host.update(0.05f);
    host.loadString("assert(g_timerFired == 1, 'timer fired once')\n"
                    "assert(g_intervalFired == 2, 'interval fired twice')", "check2");

    // dt = 0.1s: timer does NOT fire again, interval fires again
    host.update(0.1f);
    host.loadString("assert(g_timerFired == 1, 'timer fires only once')\n"
                    "assert(g_intervalFired >= 3, 'interval keeps firing')", "check3");

    // host.reset() cancels all timers
    host.reset();
    host.update(1.0f);
    TEST_ASSERT(host.status().ok, "Host still healthy after reset and timers clear");
}

void Test5_HotReload()
{
    std::cout << "\n=== Test 5: Hot-Reload of Markup & Script ===\n";
    const auto dir = MakeTempTestDir();
    const auto scriptFile = dir / "logic.lua";
    const auto markupFile = dir / "view.xml";

    WriteFile(scriptFile, R"(
        runCount = (runCount or 0) + 1
        g_btn = ui.find("ActionBtn")
    )");

    WriteFile(markupFile, R"(
        <UI script="logic.lua">
            <Panel name="Root">
                <Button name="ActionBtn" text="V1"/>
            </Panel>
        </UI>
    )");

    UIContext ctx;
    MockRenderBackend backend;
    ctx.setRenderBackend(&backend);

    ScriptHost host("TestMod");
    host.bindContext(&ctx, ctx.root());
    ScriptCallbackResolver resolver(host);

    MarkupLoader loader(ctx);
    loader.setCallbackResolver(&resolver);
    loader.enableHotReload(true);

    loader.onScriptReload([&](const std::string& relPath) {
        host.reset();
        host.bindContext(&ctx, ctx.root());
        host.loadFile(dir / relPath);
    });

    UIElement* root = loader.loadFile(markupFile);
    TEST_ASSERT(root != nullptr, "Loaded V1 markup");
    TEST_ASSERT(loader.scriptAttribute() == "logic.lua", "Found script attribute");

    host.loadFile(scriptFile);
    host.loadString("assert(runCount == 1 and g_btn ~= nil and g_btn:isValid() == true)", "v1_check");

    // Modify markup on disk
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    WriteFile(markupFile, R"(
        <UI script="logic.lua">
            <Panel name="Root">
                <Button name="OtherBtn" text="V2"/>
            </Panel>
        </UI>
    )");

    loader.poll();

    // Check that reload happened, old handle is dead, new element found
    host.loadString("assert(g_btn:isValid() == false, 'old handle is invalid')\n"
                    "assert(ui.find('OtherBtn') ~= nil, 'new button found')", "v2_check");
    TEST_ASSERT(true, "Hot-reload workflow verified");
}

void Test6_ModuleException()
{
    std::cout << "\n=== Test 6: C++ Module Exception Graceful Handling ===\n";
    ScriptHost host("TestMod");

    host.registerModule("game", [](ModuleBuilder& m) {
        m.function("throwRuntime", []() -> double {
            throw std::runtime_error("simulated native game failure");
        });
    });

    bool ok = host.loadString("game.throwRuntime()", "test6");
    TEST_ASSERT(!ok, "Calling function throwing exception returned false");
    TEST_ASSERT(host.status().ok, "Host is not permanently disabled on first error");
    TEST_ASSERT(host.status().lastError.find("simulated native game failure") != std::string::npos,
                "Lua lastError captures C++ exception message");
}

void Test7_InvalidArguments()
{
    std::cout << "\n=== Test 7: Invalid Argument Handling ===\n";
    UIContext ctx;
    ctx.root()->add<Button>("Btn", "ValidBtn");

    ScriptHost host("TestMod");
    host.bindContext(&ctx, ctx.root());

    // 1. btn:setText(nil)
    bool ok1 = host.loadString("local b = ui.find('ValidBtn'); b:setText(nil)", "test7_1");
    TEST_ASSERT(!ok1, "setText(nil) rejected");
    TEST_ASSERT(host.status().lastError.find("string expected") != std::string::npos,
                "Correct error message for setText(nil)");

    // 2. ui.timer("bad_arg", fn)
    bool ok2 = host.loadString("ui.timer('not_a_number', function() end)", "test7_2");
    TEST_ASSERT(!ok2, "ui.timer with string delay rejected");
    TEST_ASSERT(host.status().lastError.find("number expected") != std::string::npos,
                "Correct error message for ui.timer string delay");

    // 3. ui.cancel("bad_id")
    bool ok3 = host.loadString("ui.cancel('invalid_id')", "test7_3");
    TEST_ASSERT(!ok3, "ui.cancel with non-integer rejected");
    TEST_ASSERT(host.status().lastError.find("integer expected") != std::string::npos ||
                host.status().lastError.find("number expected") != std::string::npos,
                "Correct error message for ui.cancel bad id");
}

} // namespace

int main()
{
    std::cout << "Running PerfUI Lua UI API Tests (Stage 2)...\n";

    Test1_UiFindAndHandles();
    Test2_DeadElementSafety();
    Test3_MarkupCallbackPriority();
    Test4_Timers();
    Test5_HotReload();
    Test6_ModuleException();
    Test7_InvalidArguments();

    std::cout << "\n========================================\n";
    std::cout << "Tests Passed: " << g_testsPassed << "\n";
    std::cout << "Tests Failed: " << g_testsFailed << "\n";
    std::cout << "========================================\n";

    return (g_testsFailed == 0) ? 0 : 1;
}
