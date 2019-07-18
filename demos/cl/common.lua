require "src/lua/path"
local utils = require "nngn.lib.utils"

local function vec_eq(v0, v1)
    local n = #v0
    if n ~= #v1 then return false end
    for i = 1, n do
        if v0[i] ~= v1[i] then return false end
    end
    return true
end

local function print_vec(v)
    local n = #v
    if n == 0 then return end
    io.write(v[1])
    for i = 2, n do io.write(" ", v[i]) end
end

local function print_mat(m, w, h)
    for i = 1, w * h do
        if i % w == 0 then
            io.write(m[i], "\n")
        else
            io.write(m[i], " ")
        end
    end
end

local function print_prof(t)
    local n = #t / 4
    print("queue\t\tsubmit\t\tstart\t\tend")
    for i = 0, n - 1 do
        local last = 0
        for j = 0, 3 do
            local v = t[4 * i + j + 1]
            local dt = v - last
            last = v
            if j == 0 then io.write(string.format("%u", dt))
            else io.write("\t", utils.fmt_time(dt)) end
        end
        print()
    end
    io.write("total: ", utils.fmt_time(t[#t] - t[1]), "\n")
end

local function avg_prof(n, f)
    local ret = {}
    for _ = 1, n do
        local events = {}
        f(events)
        assert(nngn:compute():wait(events))
        local prof = nngn:compute():prof_info(Compute.PROF_INFO_ALL, events)
        assert(nngn:compute():release_events(events))
        for i = 1, #prof do
            ret[i] = (ret[i] or 0) + prof[i]
        end
    end
    for i, v in ipairs(ret) do ret[i] = v // n end
    return ret
end

local function round_up(x, n)
    local r = x % n
    if r == 0 then return x end
    return x + n - r
end

local function round_down_pow2(n)
    local pow = math.log(n, 2) // 1
    return math.tointeger(math.pow(2, pow) // 1)
end

local function local_size(global, desired)
    if global < desired then return global end
    if global % desired == 0 then return desired end
    for i = 2, desired do
        if global % i == 0 then
            local n = global // i
            if n < desired then return n end
        end
    end
    return 1
end

local function err_check(v0, v1, epsilon)
    return math.abs(v1 - v0) <= epsilon
end

return {
    vec_eq = vec_eq,
    print_vec = print_vec,
    print_mat = print_mat,
    print_prof = print_prof,
    avg_prof = avg_prof,
    round_up = round_up,
    round_down_pow2 = round_down_pow2,
    local_size = local_size,
    err_check = err_check,
}
