local function float_eq(l, r, tol)
    return math.abs(l - r) < (tol or (2 * Math.FLOAT_EPSILON))
end

local function clamp(v, min, max) return math.max(min, math.min(max, v)) end

local vec2_meta, vec3_meta = {}, {}

local function vec2(x, y)
    x = x or 0
    return setmetatable({x, y or x}, vec2_meta)
end

local function vec3(x, y, z)
    x = x or 0
    return setmetatable({x, y or x, z or x}, vec3_meta)
end

local function vec2_rand(f) return vec2(f(), f()) end
local function vec2_unm(v) return vec2(-v[1], -v[2]) end
local function vec2_add(v, u) return vec2(v[1] + u[1], v[2] + u[2]) end
local function vec2_sub(v, u) return vec2(v[1] - u[1], v[2] - u[2]) end
local function vec2_mul(v, u) return vec2(v[1] * u[1], v[2] * u[2]) end
local function vec2_div(v, u) return vec2(v[1] / u[1], v[2] / u[2]) end
local function vec2_idiv(v, u) return vec2(v[1] // u[1], v[2] // u[2]) end
local function vec2_dot(v, u) return v[1] * u[1] + v[2] * u[2] end
local function vec2_sqrt(v) return vec2(math.sqrt(v[1]), math.sqrt(v[2])) end
local function vec2_str(v) return string.format("{%f, %f}", v[1], v[2]) end
local function vec2_map(v, f) return vec2(f(v[1]), f(v[2])) end

local function vec3_rand(f) return vec3(f(), f(), f()) end
local function vec3_unm(v) return vec3(-v[1], -v[2], -v[3]) end
local function vec3_add(v, u)
    return vec3(v[1] + u[1], v[2] + u[2], v[3] + u[3])
end
local function vec3_sub(v, u)
    return vec3(v[1] - u[1], v[2] - u[2], v[3] - u[3])
end
local function vec3_mul(v, u)
    return vec3(v[1] * u[1], v[2] * u[2], v[3] * u[3])
end
local function vec3_div(v, u)
    return vec3(v[1] / u[1], v[2] / u[2], v[3] / u[3])
end
local function vec3_idiv(v, u)
    return vec3(v[1] // u[1], v[2] // u[2], v[3] // u[3])
end
local function vec3_dot(v, u)
    return v[1] * u[1] + v[2] * u[2] + v[3] * u[3]
end

local function vec3_cross(v, u)
    return vec3(
        v[2] * u[3] - u[2] * v[3],
        v[3] * u[1] - u[3] * v[1],
        v[1] * u[2] - u[1] * v[2])
end

local function vec3_sqrt(v)
    return vec3(math.sqrt(v[1]), math.sqrt(v[2]), math.sqrt(v[3]))
end

local function vec3_str(v)
    return string.format("{%f, %f, %f}", v[1], v[2], v[3])
end

local function vec3_map(v, f) return vec3(f(v[1]), f(v[2]), f(v[3])) end

local function vec_norm(v) return v / v.new(v:len()) end
local function vec_len_sq(v) return v:dot(v) end
local function vec_len(v) return math.sqrt(vec_len_sq(v)) end

local function vec_reflect(v, n)
    local vec = v.new
    return vec(2) * n * vec(v:dot(n)) - v
end

local function vec_refract(v, n, n0, n1)
    v = v:norm()
    local div = n1 / n0
    local dot = v:dot(n)
    local disc = 1 - div * div * (1 - dot * dot)
    if disc <= 0 then return end
    return v.new(div) * (v - (n * v.new(dot))) - n * v.new(math.sqrt(disc))
end

local function vec_interp(v, u, t) return v.new(1 - t) * v + v.new(t) * u end

vec2_meta.__unm = vec2_unm
vec2_meta.__add = vec2_add
vec2_meta.__sub = vec2_sub
vec2_meta.__mul = vec2_mul
vec2_meta.__div = vec2_div
vec2_meta.__idiv = vec2_idiv
vec2_meta.__index = {
    new = vec2,
    dot = vec2_dot,
    len_sq = vec_len_sq,
    len = vec_len,
    norm = vec_norm,
    sqrt = vec2_sqrt,
    reflect = vec_reflect,
    refract = vec_refract,
    interp = vec_interp,
    str = vec2_str,
    map = vec2_map,
}

vec3_meta.__unm = vec3_unm
vec3_meta.__add = vec3_add
vec3_meta.__sub = vec3_sub
vec3_meta.__mul = vec3_mul
vec3_meta.__div = vec3_div
vec3_meta.__idiv = vec3_idiv
vec3_meta.__index = {
    new = vec3,
    dot = vec3_dot,
    len_sq = vec_len_sq,
    len = vec_len,
    norm = vec_norm,
    cross = vec3_cross,
    sqrt = vec3_sqrt,
    reflect = vec_reflect,
    refract = vec_refract,
    interp = vec_interp,
    str = vec3_str,
    map = vec3_map,
}

return {
    float_eq = float_eq,
    clamp = clamp,
    vec2 = vec2,
    vec3 = vec3,
    vec2_rand = vec2_rand,
    vec3_rand = vec3_rand,
}
