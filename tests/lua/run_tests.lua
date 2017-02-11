#!/usr/bin/env lua

-- TODO: remove hardcode
package.path  = package.path .. ";../libs/luaunit/?.lua;../src/lua/?.lua;../tests/lua/?.lua"
package.cpath = package.cpath .. "../../build/*.dylib;./?.so;./?.dylib"

test = require "luaunit"

function log(...)
    local msg = '@\t\t' .. os.date('%Y-%m-%d %H:%M:%S ',os.time())
    for _, val in ipairs({...}) do
        msg = msg .. tostring(val) .. ' '
    end
    io.write(msg .. '\n')
    io.flush()
end

function wait(timeInSec, condition, silent)
    if not silent then
        log("Start waiting for " .. tostring(timeInSec) .. "sec...")
    end
    local result = false
    local startTime = os.time()
    while ( (os.time() - startTime) <= timeInSec) do
        if condition ~= nil then
            if type(condition) == 'function' then
                if condition() then
                    result = true
                    break
                end
            else
                if condition then
                    result = true
                    break
                end
            end
        end
    end
    if not silent then
        log "Give up"
    end
    return result
end

function sleep(timeInSec, silent)
    if not silent then
        log("Start sleep for " .. tostring(timeInSec) .. "sec...")
    end
    wait(timeInSec, nil, true)
    if not silent then
        log "Wake up"
    end
end

do
    -- Hack input arguments to make tests verbose by default
    local found = false
    for _, v in ipairs(arg) do
        if v == '-o' or v == '--output' then
            found = true
            break
        end
    end
    if not found then
        table.insert(arg, '-o')
        table.insert(arg, 'TAP')
    end
end

-----------
-- TESTS --
-----------

require("smoke_test")

-----------
os.exit( test.LuaUnit.run() )