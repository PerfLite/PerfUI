-- PerfUI PSS Demo Script
-- Demonstrates runtime CSS variable switching and pseudo-class interactions

local currentTheme = 0
local themes = {
    { name = "Nordic Gold", accent = "#c8a45a", hover = "#e0be75" },
    { name = "Cyber Cyan",  accent = "#00d2ff", hover = "#52e5ff" },
    { name = "Crimson Ruby",accent = "#ff4757", hover = "#ff6b81" },
    { name = "Emerald Glow",accent = "#2ed573", hover = "#7bed9f" }
}

function onSwitchTheme()
    currentTheme = (currentTheme % #themes) + 1
    local th = themes[currentTheme]

    -- Mutate CSS variables at runtime without rebuilding the UI tree
    ui.setVar("--accent", th.accent)
    ui.setVar("--accent-hover", th.hover)
    ui.setVar("--card-border-hover", th.accent)
    ui.setVar("--btn-active-bg", th.accent)

    local status = ui.find("StatusText")
    if status then
        status:setText("Theme: " .. th.name .. " (" .. th.accent .. ") | Styles dynamically updated")
    end
    ui.toast("Theme Switched", th.name .. " (" .. th.accent .. ")", "success", 2.0)
end

function onToggleBlur(checked)
    local isChecked = (checked == 1 or checked == true)
    local card = ui.find("MainCard")
    if card then
        if isChecked then
            card:addClass("blur-active")
        else
            card:removeClass("blur-active")
        end
    end
end

function onToggleDpi(checked)
    local isChecked = (checked == 1 or checked == true)
    local status = ui.find("StatusText")
    if status then
        status:setText("High-DPI Scaling: " .. (isChecked and "Enabled" or "Disabled"))
    end
end

function onToggleSound(checked)
    local isChecked = (checked == 1 or checked == true)
    local status = ui.find("StatusText")
    if status then
        status:setText("Sound Feedback: " .. (isChecked and "Active" or "Muted"))
    end
end

function onApplySettings()
    local btn = ui.find("BtnApply")
    if btn then
        btn:setText("Settings Applied!")
        ui.timer(1.5, function()
            btn:setText("Accept & Save")
        end)
    end
end
