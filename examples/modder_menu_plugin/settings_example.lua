--[[
    PerfUI Mod Settings - Lua Scripting Example
    Demonstrates reading settings, reacting to changes, and modifying values at runtime.
--]]

local modId = "MyAwesomeMod"

-- Read initial values with safe fallbacks
local hudEnabled = perfui.settings.get(modId, "enableDynamicHud", true)
local hudScale = perfui.settings.get(modId, "hudScale", 100.0)
local accentColor = perfui.settings.get(modId, "hudAccentColor", 0x3399FFCC)

print(string.format("[LuaMod] Loaded: HUD Enabled=%s, Scale=%.1f%%", tostring(hudEnabled), hudScale))

-- Subscribe to real-time changes made by the user in the Settings Window
perfui.settings.onChanged(modId, "enableDynamicHud", function(mod, key, value)
    print(string.format("[LuaMod] Setting changed: %s.%s = %s", mod, key, tostring(value)))
    if value == true then
        -- Enable widget display
    else
        -- Hide widget display
    end
end)

perfui.settings.onChanged(modId, "hudScale", function(mod, key, value)
    print(string.format("[LuaMod] HUD Scale updated to: %.1f%%", value))
end)

-- Example programmatic change (e.g. from an in-game event or console command)
function ResetHudScaleToDefault()
    perfui.settings.set(modId, "hudScale", 100.0)
end
