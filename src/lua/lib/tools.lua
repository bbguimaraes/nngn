local TOOLS_BIN_PATH = "tools/bin"

local function dirname(p)
    local last, i = nil, 2
    while true do
        i = string.find(p, "/", i, true)
        if i == nil then
            if last ~= nil then
                p = string.sub(p, 1, last - 1)
            end
            return p
        end
        last = i
        i = i + 1
    end
end

local function path_contains(path, p)
    for p in string.gmatch(path, "[^:]+") do
        if p == path then return true end
    end
    return false
end

local function add_to_path()
    local path = Platform.argv()[1]
    if path then
        path = dirname(path)
            .. string.sub(package.config, 1, 1)
            .. TOOLS_BIN_PATH
    else
        path = TOOLS_BIN_PATH
    end
    local path_env = os.getenv("PATH")
    if path_contains(path_env, path) then return end
    if path_env then
        path_env = path_env .. ":" .. path
    else
        path_env = path
    end
    Platform.setenv("PATH", path_env)
end

local function wrap(f)
    return function(...)
        add_to_path()
        return f(...)
    end
end

return {
    add_to_path = add_to_path,
    wrap = wrap,
}
