-- ============================================================================
-- PerfUI Lua Counter Example (Pure Script Logic)
-- ============================================================================

local count = 0
local step = 1
local autoTimerId = nil

local display = ui.find("CounterDisplay")

local function updateDisplay()
    if display then
        display:setText("Count: " .. tostring(count))
    end
end

function onIncrement()
    count = count + step
    updateDisplay()
end

function onDecrement()
    count = count - step
    updateDisplay()
end

function onReset()
    count = 0
    updateDisplay()
    ui.toast("Counter reset to 0", "info")
end

function onStepChanged(newStep)
    step = math.floor(newStep)
    ui.toast("Step changed to " .. tostring(step), "info")
end

function onAutoToggle(enabled)
    if enabled == 1.0 or enabled == true then
        if not autoTimerId then
            autoTimerId = ui.interval(1.0, function()
                onIncrement()
            end)
            ui.toast("Auto-increment started", "success")
        end
    else
        if autoTimerId then
            ui.cancel(autoTimerId)
            autoTimerId = nil
            ui.toast("Auto-increment stopped", "warning")
        end
    end
end

-- Initialize display
updateDisplay()
log.info("Lua Counter initialized successfully!")
