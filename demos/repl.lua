dofile "src/lua/path.lua"
local font = require "nngn.lib.font"
local input = require "nngn.lib.input"
local textbox = require "nngn.lib.textbox"

local CHAR_BEGIN = string.byte(" ")
local CHAR_END = string.byte("~")
local LETTERS_BEGIN = string.byte("A")
local LETTERS_END = string.byte("Z")
local SHIFT_MAP = {
    ["`"] = "~",
    ["1"] = "!",
    ["2"] = "@",
    ["3"] = "#",
    ["4"] = "$",
    ["5"] = "%",
    ["6"] = "^",
    ["7"] = "&",
    ["8"] = "*",
    ["9"] = "(",
    ["0"] = ")",
    ["-"] = "_",
    ["="] = "+",
    ["["] = "{",
    ["]"] = "}",
    ["\\"] = "|",
    [";"] = ":",
    ["'"] = "\"",
    [","] = "<",
    ["."] = ">",
    ["/"] = "?",
}

local function read_key(key, mods)
    if key > 127 then return end
    if key == Input.KEY_ENTER then return key end
    if mods & Input.MOD_SHIFT == 0 then
        if LETTERS_BEGIN <= key and key <= LETTERS_END then
            return string.lower(string.char(key))
        end
        return string.char(key)
    end
    if LETTERS_BEGIN <= key and key <= LETTERS_END then
        return string.char(key)
    end
    return SHIFT_MAP[string.char(key)]
end

local function fmt_error(err)
    return string.format("%serror: %s", string.char(Textbox.TEXT_RED), err)
end

local function eval(input)
    local chunk, err = load("return " .. input)
    if err then
        chunk, err = load(input)
    end
    if err then return fmt_error(err) end
    if chunk then
        local ok, ret = pcall(chunk)
        if not ok then return fmt_error(ret) end
        return string.format("%s%s", string.char(Textbox.TEXT_GREEN), ret)
    end
    return ""
end

require("nngn.lib.graphics").init()
nngn.renderers:set_max_text(1 << 16)
require("nngn.lib.input").install()
font.load(32, "DejaVuSans.ttf")
nngn.textbox:set_title("lua")
nngn.textbox:set_text("write some lua code and press enter")
nngn.textbox:set_speed(1)
input.input:remove(string.byte(" "))
local input = ""
nngn.input:register_callback(function(key, press, mods)
    if not press then return end
    if key == Input.KEY_ENTER then
        nngn.textbox:set_text(eval(input))
        input = ""
    else
        local char = read_key(key, mods)
        if char then
            input = input .. char
            nngn.textbox:set_text(input)
        end
    end
end)
