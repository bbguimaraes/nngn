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

local function update_task(active, task, f)
    if active then
        if not task then
            task = nngn:schedule():next(Schedule.HEARTBEAT, f)
        end
        return task
    elseif task then
        nngn:schedule():cancel(task)
    end
end

local function update(ps, task, update, f)
    for k, v in pairs(ps) do
        local p <const> = v[1]
        if Platform.ferror(p) then
            ps[k] = nil
        else
            f(k, v)
            p:flush()
        end
    end
    return update_task(next(ps), task, update)
end

local function init(ps, k, open_f, init_f)
    local p <const> = open_f(k)
    p:setvbuf("line")
    local t <const> = {p}
    init_f(k, t)
    p:flush()
    ps[k] = t
end

local function init_r(ps, task, update_f, open_f, init_f, ...)
    for _, k in ipairs{...} do
        local t = ps[k]
        if t then
            t[1]:close()
            ps[k] = nil
        else
            init(ps, k, open_f, init_f)
        end
    end
    return update_task(next(ps), task, update_f)
end

local function init_w(ps, task, update_f, open_f, init_f, ...)
    for _, k in ipairs{...} do
        local t = ps[k]
        if t then
            local p <const> = t[1]
            p:write("q\n")
            p:close()
            ps[k] = nil
        else
            init(ps, k, open_f, init_f)
        end
    end
    return update_task(next(ps), task, update_f)
end

return {
    add_to_path = add_to_path,
    wrap = wrap,
    init_r = init_r,
    init_w = init_w,
    update_task = update_task,
    update = update,
}
