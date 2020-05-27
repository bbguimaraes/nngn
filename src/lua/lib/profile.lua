local utils = require "nngn.lib.utils"

local function active(c) return Stats.active(c.STATS_IDX) end
local function activate(c) Stats.set_active(c.STATS_IDX, true) end

local function write_bar_graph(names, values, max)
    if max == nil then max = math.max(table.unpack(values)) end
    if max == 0 then max = 1 end
    local max_len = 0
    for _, v in pairs(names) do max_len = math.max(max_len, string.len(v)) end
    local cols = 77 - max_len
    local max_str = utils.fmt_time(max)
    io.write(string.rep(" ", 80 - string.len(max_str)), max_str, "\n")
    for i = 1, #names do
        local name, value = names[i], values[i]
        local fill = math.tointeger(cols * value // max)
        io.write(
            name, string.rep(" ", max_len - string.len(name) + 1),
            "[", string.rep("=", fill), string.rep(" ", cols - fill), "]\n")
    end
end

local function dump_text(c)
    local names, values, n_events = c.stats_names(), c.stats(), c.STATS_N_EVENTS
    for i = 1, #names do
        local v = values[n_events * i] - values[n_events * (i - 1) + 1]
        io.write(names[i], ": ", utils.fmt_time(v), "\n")
    end
end

local function dump_tui(c, max)
    local names, values, n_events = c.stats_names(), c.stats(), c.STATS_N_EVENTS
    local n = #names
    for i = 1, n do
        values[i] = values[n_events * i] - values[n_events * (i - 1) + 1]
    end
    for i = n + 1, #values do values[i] = nil end
    write_bar_graph(names, values, max)
end

return {
    active = active,
    activate = activate,
    dump_text = dump_text,
    dump_tui = dump_tui,
}
