local function pprint(x, pre, write)
    write = write or io.write
    local t = type(x)
    if t ~= "table" then
        if pre then write(" ") end
        if t ~= "string" then return write(tostring(x), "\n") end
        return write(string.format("%q\n", x))
    end
    if pre then write("\n") else pre = "" end
    if next(x) == nil then return write("{}\n") end
    local npre = pre .. "  "
    for k, v in pairs(x) do
        write(pre, tostring(k), ":")
        pprint(v, npre, write)
    end
end

local function pformat(x)
    local ret = ""
    pprint(x, nil, function(...)
        for _, x in ipairs{...} do ret = ret .. tostring(x) end
    end)
    return ret
end

local function map(t, f, iter)
    local ret = {}
    for k, v in (iter or pairs)(t) do ret[k] = f(v) end
    return ret
end

return {
    pprint = pprint,
    pformat = pformat,
    map = map,
}
