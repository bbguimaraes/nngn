dofile "src/lua/path.lua"
local common = require "demos/cl/common"

nngn:set_compute(Compute.OPENCL_BACKEND, Compute.opencl_params{debug = true})

local prog <const> = assert(nngn.compute:create_program(
    io.open("demos/cl/vector.cl"):read("a"), "-Werror"))

local function exec(
    out_type, sizeof_out, out_size,
    prog, func, gsize, lsize, args
)
    local out = assert(nngn.compute:create_buffer(
        Compute.WRITE_ONLY, out_type, out_size))
    args = {table.unpack(args)}
    table.insert(args, Compute.BUFFER)
    table.insert(args, out)
    common.print_prof(common.avg_prof(5, function(e)
        assert(nngn.compute:execute(
            prog, func, Compute.BLOCKING, gsize, lsize, args, {}, e))
    end))
    local v = Compute.create_vector(sizeof_out * out_size)
    assert(nngn.compute:read_buffer(out, out_type, out_size, v))
    local ret = Compute.read_vector(v, 0, out_size, out_type)
    assert(nngn.compute:release_buffer(out))
    return ret
end

local function test_add_numbers()
    print("add_numbers:")
    local out = exec(
        Compute.FLOATV, Compute.SIZEOF_FLOAT, 2, prog, "add_numbers",
        {8}, {4}, {
            Compute.FLOATV, {
                 0,  1,  2,  3,  4,  5,  6,  7,
                 8,  9, 10, 11, 12, 13, 14, 15,
                16, 17, 18, 19, 20, 21, 22, 23,
                24, 25, 26, 27, 28, 29, 30, 31,
                32, 33, 34, 35, 36, 37, 38, 39,
                40, 41, 42, 43, 44, 45, 46, 47,
                48, 49, 50, 51, 52, 53, 54, 55,
                56, 57, 58, 59, 60, 61, 62, 63},
            Compute.LOCAL, 4 * Compute.SIZEOF_FLOAT})
    local sum = out[1] + out[2]
    print(sum)
    if not common.err_check(sum, 64 / 2 * 63, .01) then
        error("check failed")
    end
end

local function test_vector()
    local v = Compute.create_vector(Compute.SIZEOF_FLOAT * 8)
    assert(Compute.write_vector(
        v, 0, {Compute.FLOATV, {0, 1, 2, 3, 4, 5, 6, 7}}))
    local b0 = assert(nngn.compute:create_buffer(Compute.READ_ONLY, 0, 0, v))
    assert(Compute.write_vector(
        v, 0, {Compute.FLOATV, {8, 9, 10, 11, 12, 13, 14, 15}}))
    local b1 = assert(nngn.compute:create_buffer(Compute.READ_ONLY, 0, 0, v))
    for _, t in ipairs({
        {"add_vector", {8, 10, 12, 14, 16, 18, 20,  22}},
        {"mul_vector", {0,  9, 20, 33, 48, 65, 84, 105}},
    }) do
        local f, cmp = table.unpack(t)
        io.write(f, ":\n")
        local out = exec(
            Compute.FLOATV, Compute.SIZEOF_FLOAT, #cmp, prog, f,
            {8}, {8}, {
                Compute.BUFFER, b0,
                Compute.BUFFER, b1})
        if not common.vec_eq(out, cmp) then error("check failed") end
        common.print_vec(out)
        print()
    end
    assert(nngn.compute:release_buffer(b1))
    assert(nngn.compute:release_buffer(b0))
end

local function test_mul_mat_cpu(n)
    local m0 = nngn.math:rand_mat(n)
    local m2 = Math.vecf(n * n)
    io.write(string.format("mul_mat_cpu(%d):", n))
    io.flush()
    print(string.format(
        " %.3fs",
        nngn.timing.time_ms(function() nngn.math.mat_mul(n, m0, m0, m2) end)))
end

local function test_mul_mat(n)
    local m0, m1, cmp
    if n == 3 then
        m0 = {
            0, 1, 2,
            3, 4, 5,
            6, 7, 8}
        m1 = {
             9, 10, 11,
            12, 13, 14,
            15, 16, 17}
        cmp = {
             42,  45,  48,
            150, 162, 174,
            258, 279, 300}
    end
    local n2 = n * n
    local ls = math.max(n // 16, 1)
    local ls2 = math.min(ls, 16)
    local prog = assert(
        nngn.compute:create_program(
            io.open("demos/cl/vector.cl"):read("a"),
            "-Werror -DBLOCK_SIZE=" .. ls2))
    if not m0 and not m1 then
        m0, m1 = {}, {}
        for i = 1, n2 do
            m0[i] = nngn.math:rand()
            m1[i] = nngn.math:rand()
        end
    end
    local v = Compute.create_vector(Compute.SIZEOF_FLOAT * n2)
    assert(Compute.write_vector(v, 0, {Compute.FLOATV, m0}))
    local b0 = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, 0, v))
    assert(Compute.write_vector(v, 0, {Compute.FLOATV, m1}))
    local b1 = assert(nngn.compute:create_buffer(
        Compute.READ_ONLY, Compute.FLOATV, 0, v))
    local args = {{
        Compute.FLOATV, {n},
        Compute.BUFFER, b0,
        Compute.BUFFER, b1,
    }, {
        Compute.UINT, n,
        Compute.BUFFER, b0,
        Compute.BUFFER, b1,
    }, {
        Compute.UINT, n,
        Compute.BUFFER, b0,
        Compute.BUFFER, b1,
        Compute.LOCAL, n * Compute.SIZEOF_FLOAT,
    }, {
        Compute.UINT, n,
        Compute.BUFFER, b0,
        Compute.BUFFER, b1,
        Compute.LOCAL, ls * ls * Compute.SIZEOF_FLOAT,
        Compute.LOCAL, ls * ls * Compute.SIZEOF_FLOAT}}
    local ts = {
        {"mul_mat0", {n, n}, {ls2, ls2}, args[1]},
        {"mul_mat1", {n, n}, {ls2, ls2}, args[2]},
        {"mul_mat2", {n, n}, {ls2, ls2}, args[2]},
        {"mul_mat3", {n}, {ls}, args[2]},
        {"mul_mat4", {n}, {ls}, args[2]},
        {"mul_mat5", {n}, {ls}, args[3]},
        {"mul_mat6", {n, n}, {ls2, ls2}, args[4]},
        {"mul_mat7", {n, n}, {ls2, ls2}, args[4]},
        {"mul_mat8", {n, n}, {ls2, ls2}, args[4]},
        {"mul_mat_nvidia", {n, n}, {ls2, ls2}, args[4]}}
    if not cmp then
        local out = assert(
            nngn.compute:create_buffer(
                Compute.WRITE_ONLY, Compute.FLOATV, n2))
        local args = {table.unpack(args[2])}
        table.insert(args, Compute.BUFFER)
        table.insert(args, out)
        assert(nngn.compute:execute(
            prog, "mul_mat1", Compute.BLOCKING,
            {n, n}, {ls2, ls2}, args))
        local v = Compute.create_vector(Compute.SIZEOF_FLOAT * n2)
        assert(nngn.compute:read_buffer(out, Compute.FLOATV, n2, v))
        cmp = assert(Compute.read_vector(v, 0, n2, Compute.FLOATV))
        assert(nngn.compute:release_buffer(out))
    end
    for _, t in ipairs(ts) do
        local f, g, l, a = table.unpack(t)
        io.write(f, ": ", n, "\n")
        local out = exec(
            Compute.FLOATV, Compute.SIZEOF_FLOAT, n2, prog, f, g, l, a)
        if not common.vec_eq(out, cmp) then error("check failed") end
    end
    assert(nngn.compute:release_buffer(b1))
    assert(nngn.compute:release_buffer(b0))
end

local function test_pi(n)
    local ls = 256
    local it = 512
    local args = {{
        Compute.UINT, n,
        Compute.FLOAT, 1 / n,
    }, {
        Compute.UINT, it,
        Compute.FLOAT, 1 / n,
        Compute.LOCAL, ls * Compute.SIZEOF_FLOAT}}
    for _, t in ipairs({
        {"pi_reduce", {n // ls}, {1}, args[1]},
        {"pi_iter", {n // it}, {ls}, args[2]},
        {"pi_vec4", {n // it}, {ls}, args[2]},
        {"pi_vec8", {n // it}, {ls}, args[2]},
    }) do
        local f, g, l, a = table.unpack(t)
        print(string.format("%s %d:", f, n))
        local out = exec(
            Compute.FLOATV, Compute.SIZEOF_FLOAT, g[1] // l[1], prog, f,
            g, l, a)
        local sum = 0
        for _, v in pairs(out) do sum = sum + v end
        sum = sum / n
        if not common.err_check(sum, math.pi, .01) then
            error("check failed")
        end
        print(sum, (sum - math.pi) / math.pi)
    end
end

test_add_numbers()
test_vector()
test_mul_mat_cpu(3)
test_mul_mat(3)
test_mul_mat_cpu(1024)
test_mul_mat(1024)
test_pi(1024 * 1024)
test_pi(1024 * 1024 * 1024)
nngn:exit()
