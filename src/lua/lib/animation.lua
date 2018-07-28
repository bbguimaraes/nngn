local camera = require "nngn.lib.camera"
local input = require "nngn.lib.input"
local nngn_math = require "nngn.lib.math"
local player = require "nngn.lib.player"
local state = require "nngn.lib.state"
local textbox = require "nngn.lib.textbox"
local class = require("nngn.lib.utils").class

local vec3 <const> = nngn_math.vec3

local Animation = class {
    chain = function(self, next) self.next = next end,
}

local Timer = class : public(Animation) {}
local Function = class : public(Animation) {}
local Camera = class : public(Animation) {}
local CameraRot = class : public(Animation) {}
local Velocity = class : public(Animation) {}
local Flock = class : public(Animation) {}
local Sprite = class : public(Animation) {}
local Textbox = class : public(Animation) {}
local Cycle = class : public(Animation) {}
local SaveState = class : public(Function) {}
local RestoreState = class : public(Function) {}

function Timer:new(ms)
    return setmetatable({count = ms, timer = ms}, self)
end

function Timer:update(dt)
    self.timer = self.timer - dt
    if self.timer <= 0 then
        self.timer = self.count
        return self.next
    end
    return self
end

function Function:new(f)
    return setmetatable({f = f}, self)
end

function Function:update(dt)
    if self.f(dt) then return self.next end
    return self
end

function SaveState:new(t)
    return Function:new(function() state.save(t); return true end)
end

function RestoreState:new(t)
    return Function:new(function() state.restore(t); return true end)
end

function Camera:new(dst, acc, camera)
    camera = camera or nngn.camera
    return setmetatable({camera = camera, dst = dst, acc = acc}, self)
end

function Camera:update(dt)
    local c = self.camera
    local pos = vec3(c:pos())
    if not self.src then
        self.src = pos
        self.cur = {c:acc()}
        c:set_acc(table.unpack(self.acc))
        return self
    end
    if (self.dst - pos):dot(self.acc) <= 0 then
        self.src = nil
        c:set_vel(0, 0, 0)
        c:set_acc(table.unpack(self.cur))
        return self.next
    end
    return self
end

function CameraRot:new(dst, acc, camera)
    camera = camera or nngn.camera
    return setmetatable({camera = camera, dst = dst, acc = acc}, self)
end

function CameraRot:init(c)
    self.src = {c:rot()}
    self.cur = {c:rot_acc()}
    c:set_rot_acc(table.unpack(self.acc))
    return self
end

function CameraRot:finish(c, rot)
    -- TODO add to camera
    for i = 1, 3 do
        rot[i] = math.fmod(rot[i], 2 * math.pi)
    end
    self.src = nil
    c:set_rot_acc(table.unpack(self.cur))
    c:set_rot(table.unpack(rot))
    return self.next
end

function CameraRot:remaining(rot)
    local c <const> = rot - self.src
    local d <const> = vec3(table.unpack(self.dst)) - self.src
    local ret = vec3(1)
    for i = 1, 3 do
        if math.abs(d[i]) <= Math.FLOAT_EPSILON then goto continue end
        ret[i] = c[i] / d[i]
        ::continue::
    end
    return vec3(1) - ret
end

function CameraRot:dampen(c, r)
    local t <const> = 0x1p-4
    if r:len() >= t then
        if self.dampen_vel then
            self.dampen_vel = nil
        end
        return
    end
    local v = self.dampen_vel
    if not v then
        v = {c:rot_vel()}
        self.dampen_vel = v
    end
    r = r * vec3(1 / t)
    for i = 1, 3 do
        if r[i] == 0 then goto continue end
        local sub <const> = 1 - r[i]
        r[i] = 1 - sub * sub * sub
        ::continue::
    end
    c:set_rot_vel(table.unpack(r * v))
end

function CameraRot:update(dt)
    local c <const> = self.camera
    if not self.src then
        return self:init(c)
    end
    local rot <const> = vec3(c:rot())
    if (self.dst - rot):dot(self.acc) <= 0 then
        return self:finish(c, rot)
    end
    self:dampen(c, self:remaining(rot))
    return self
end

function Velocity:new(entity, dst, vel)
    return setmetatable({entity = entity, dst = dst, vel = vel}, self)
end

function Velocity:update(dt)
    local e = self.entity
    local pos = vec3(e:pos())
    if not self.src then
        self.src = pos
        self.cur = {e:vel()}
        e:set_vel(table.unpack(self.vel))
        return self
    end
    if (self.dst - pos):dot(self.vel) <= 0 then
        self.src = nil
        e:set_vel(table.unpack(self.cur))
        return self.next
    end
    return self
end

function Flock:new(target, entities, acc, min_dist)
    return setmetatable({
        target = target,
        entities = entities,
        acc = acc,
        min_dist2 = min_dist * min_dist,
    }, self)
end

function Flock:update(dt)
    local acc, min_dist2 <const> = self.acc, self.min_dist2
    local target <const> = vec3(self.target:pos())
    for _, e in ipairs(self.entities) do
        local pos <const> = vec3(e:pos())
        local v = Flock.follow(target, pos, acc, min_dist2, vec3())
        for _, f in ipairs(self.entities) do
            if e ~= f then
                v = Flock.avoid(f, pos, min_dist2, v)
            end
        end
        Flock.apply(e, v)
    end
    return self
end

function Flock.follow(target, pos, acc, min2, ret)
    local dist <const> = target - pos
    local len2 <const> = dist:len_sq()
    if len2 >= min2 then
        ret = ret + dist * vec3(acc / math.sqrt(len2))
    end
    return ret
end

function Flock.avoid(f, pos, min2, ret)
    local dist <const> = pos - vec3(f:pos())
    if dist:len_sq() <= min2 then ret = ret + dist end
    return ret
end

function Flock.apply(e, acc)
    e:set_acc(table.unpack(acc))
    if acc[1] == 0 and acc[2] == 0 then
        e:set_vel(0, 0, 0)
    end
end

function Sprite:new(entity, track)
    return setmetatable({entity = entity, track = track}, self)
end

function Sprite:update()
    self.entity:animation():sprite():set_track(self.track)
    return self.next
end

function Textbox:new(title, text)
    return setmetatable({title = title, text = text, state = 0}, self)
end

function Textbox:update()
    local s = self.state
    if s == 0 then
        textbox.update(self.title, self.text)
        self.state = 1
        return self
    end
    if nngn.textbox:empty() then
        self.state = 0
        return self.next
    end
    return self
end

local function chain(t)
    local last, _ = t[1]
    for i = 2, #t do _, last = last:chain(t[i]), t[i] end
    return t[1]
end

function Cycle:new(t)
    local first = chain(t)
    return setmetatable({first = first, cur = first}, self)
end

function Cycle:update(dt)
    local n = self.cur:update(dt)
    if not n then n = self.first end
    self.cur = n
    return self
end

local function pause_input() input.pause(); return true end
local function resume_input() input.resume(); return true end

local function stop()
    local p = nngn.players:cur()
    if p then player.stop(p) end
    camera.set_follow()
    return true
end

return {
    pause_input = function() return Function:new(pause_input) end,
    resume_input = function() return Function:new(resume_input) end,
    save_state = function(...) return SaveState:new(...) end,
    restore_state = function(...) return RestoreState:new(...) end,
    stop = function() return Function:new(stop) end,
    timer = function(...) return Timer:new(...) end,
    func = function(...) return Function:new(...) end,
    camera = function(...) return Camera:new(...) end,
    camera_rot = function(...) return CameraRot:new(...) end,
    velocity = function(...) return Velocity:new(...) end,
    flock = function(...) return Flock:new(...) end,
    sprite = function(...) return Sprite:new(...) end,
    textbox = function(...) return Textbox:new(...) end,
    cycle = function(t) return Cycle:new(t) end,
    chain = chain,
}
