-- init.lua: Initialize functions and environment for Lua executor

-- Print function (already defined in Roblox Lua, just in case it's missing)
function print(...)
    local args = {...}
    local output = ""
    for i, v in ipairs(args) do
        output = output .. tostring(v)
        if i < #args then
            output = output .. "\t"
        end
    end
    -- This outputs to Roblox's console or any log you want
    game:GetService("ReplicatedStorage").DefaultChatSystemChatEvents.SayMessageRequest:FireServer(output, "All")
end

-- Task functions (e.g., task.spawn for deferred execution)
function task_spawn(func)
    spawn(func)
end

-- Function to test the Lua executor
function test_executor()
    print("Test executor initialized successfully!")
end

-- Additional utility functions can be added here

-- Set up any environment or configurations here if needed
test_executor()  -- Call the test function to confirm the setup works
