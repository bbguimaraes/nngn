local DEFAULT <const> = "DejaVuSans.ttf"
local DEFAULT_SIZE <const> = 32

local function find(name)
    local cmd <const> = "fc-match --format %{file} " .. name
    local ok, p <const> = pcall(io.popen, cmd)
    if not ok then
        if p ~= "'popen' not supported" then
            error(p)
        end
        return name
    end
    local ret <const> = p:read()
    local ok, v0, v1 <const> = p:close()
    if ok then
        return ret
    end
    log("lua:font.find: command \"", cmd, "\" failed: ", v0, " ", v1, "\n")
    return name
end

local function load(size, name)
    return nngn.fonts:load(size or DEFAULT_SIZE, find(name or DEFAULT))
end

return {
    DEFAULT = DEFAULT,
    DEFAULT_SIZE = DEFAULT_SIZE,
    load = load,
}
