local serialize_number, serialize_table

--- Returns a string which can be `load`ed to procude \p o again.
--- Based on chapter 15 of _Programming in Lua_.
local function serialize(o, indent, level)
    local t <const> = type(o)
    if t == "nil" or t == "boolean" or t == "string" then
        return string.format("%q", o)
    elseif t == "number" then
        return serialize_number(o)
    elseif t == "table" then
        return serialize_table(o, indent, level)
    else
        error("cannot serialize a " .. t)
    end
end

function serialize_number(o)
    local ret <const> = tostring(o)
    if assert(load("return " .. ret))() == o then
        return ret
    else
        return string.format("%q", o)
    end
end

local serialize_seq, serialize_map
function serialize_table(o, indent, level)
    if not next(o) then
        return "{}"
    end
    local ret <const> = {}
    local pre0 <const> = string.rep(" ", indent * level)
    local pre1 <const> = string.rep(" ", indent * (level + 1))
    table.insert(ret, "{\n")
    serialize_seq(ret, indent, level, pre1, o)
    serialize_map(ret, indent, level, pre1, o)
    table.insert(ret, pre0)
    table.insert(ret, "}")
    return table.concat(ret, "")
end

function serialize_seq(ret, indent, level, pre, o)
    local sep
    for _, v in ipairs(o) do
        table.insert(ret, pre)
        table.insert(ret, serialize(v, indent, level + 1))
        table.insert(ret, ",\n")
    end
end

local serialize_key
function serialize_map(ret, indent, level, pre, o)
    local n <const> = #o
    for k, v in pairs(o) do
        if math.type(k) == "integer" and k <= n then
            goto continue
        end
        table.insert(ret, pre)
        table.insert(ret, serialize_key(k))
        table.insert(ret, " = ")
        table.insert(ret, serialize(v, indent, level + 1))
        table.insert(ret, ",\n")
        ::continue::
    end
end

function serialize_key(o)
    local t = type(o)
    if t == "string" then
        if string.match(o, "^%a%w*$") then
            return o
        end
    end
    return string.format("[%s]", serialize(o))
end

return {
    serialize = function(o, indent) return serialize(o, indent or 4, 0) end,
}
