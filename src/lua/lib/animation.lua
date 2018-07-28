local nngn_camera <const> = require "nngn.lib.camera"
local input <const> = require "nngn.lib.input"
local nngn_math <const> = require "nngn.lib.math"
local player <const> = require "nngn.lib.player"
local state <const> = require "nngn.lib.state"
local nngn_textbox <const> = require "nngn.lib.textbox"
local class <const> = require("nngn.lib.utils").class
local vec3 <const> = nngn_math.vec3

local animation = class "animation" {
    chain = function(self, next) self.next = next end,
}

local timer = class "timer" : public(animation) {}
local fn = class "fn" : public(animation) {}
local camera = class "camera" : public(animation) {}
local camera_rot = class "camera_rot" : public(animation) {}
local camera_rev = class "camera_rev" : public(animation) {}
local velocity = class "velocity" : public(animation) {}
local flock = class "Flock" : public(animation) {}
local sprite = class "Sprite" : public(animation) {}
local textbox = class "textbox" : public(animation) {}
local cycle = class "cycle" : public(animation) {}
local save_state = class "save_state" : public(fn) {}
local restore_state = class "restore_state" : public(fn) {}

function timer:new(ms)
    return setmetatable({count = ms, timer = ms}, self)
end

function timer:update(dt)
    self.timer = self.timer - dt
    if self.timer <= 0 then
        self.timer = self.count
        return self.next
    end
    return self
end

function fn:new(f)
    return setmetatable({f = f}, self)
end

function fn:update(dt)
    if self.f(dt) then return self.next end
    return self
end

function save_state:new(t)
    return fn:new(function() state.save(t); return true end)
end

function restore_state:new(t)
    return fn:new(function() state.restore(t); return true end)
end

function camera:new(dst, acc, camera)
    camera = camera or nngn:camera()
    return setmetatable({camera = camera, dst = dst, acc = acc}, self)
end

function camera:done(pos)
    return (self.dst - pos):dot(self.acc) <= 0
end

function camera:update(dt)
    local c <const> = self.camera
    local pos <const> = vec3(c:pos())
    if not self.src then
        self.src = pos
        self.cur = {c:acc()}
        c:set_acc(table.unpack(self.acc))
        return self
    end
    if self:done(pos) then
        self.src = nil
        c:set_vel(0, 0, 0)
        c:set_acc(table.unpack(self.cur))
        return self.next
    end
    return self
end

function camera_rot:new(dst, acc, camera)
    camera = camera or nngn:camera()
    return setmetatable({camera = camera, dst = dst, acc = acc}, self)
end

function camera_rot:init(c)
    self.src = {c:rot()}
    self.cur = {c:rot_acc()}
    c:set_rot_acc(table.unpack(self.acc))
    return self
end

function camera_rot:finish(c, rot)
    -- TODO add to camera
    for i = 1, 3 do
        rot[i] = math.fmod(rot[i], 2 * math.pi)
    end
    self.src = nil
    c:set_rot_acc(table.unpack(self.cur))
    c:set_rot(table.unpack(rot))
    return self.next
end

function camera_rot:remaining(rot)
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

function camera_rot:dampen(c, r)
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

function camera_rot:update(dt)
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

function camera_rev:new(center, w, camera)
    camera = camera or nngn:camera()
    center = vec3(table.unpack(center))
    return setmetatable({
        camera = camera,
        center = center,
        v = vec3(camera:pos()) - center,
        w = w,
        timer = 0,
    }, self)
end

function camera_rev:update(dt)
    local c <const> = self.camera
    local rx, ry <const> = c:rot()
    self.timer = self.timer + dt
    local a <const> = (self.timer * self.w) % (2 * math.pi)
    local v <const> = Math.rotate_z(self.v, math.sin(a), math.cos(a))
    local p <const> = self.center + vec3(table.unpack(v))
    c:set_pos(table.unpack(p))
    c:set_rot(rx, ry, a)
    return self
end

function velocity:new(entity, dst, vel)
    return setmetatable({entity = entity, dst = dst, vel = vel}, self)
end

function velocity:update(dt)
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

function flock:new(target, entities, acc, min_dist)
    return setmetatable({
        target = target,
        entities = entities,
        acc = acc,
        min_dist2 = min_dist * min_dist,
    }, self)
end

function flock:update(dt)
    local acc, min_dist2 <const> = self.acc, self.min_dist2
    local target <const> = vec3(self.target:pos())
    for _, e in ipairs(self.entities) do
        local pos <const> = vec3(e:pos())
        local v = flock.follow(target, pos, acc, min_dist2, vec3())
        for _, f in ipairs(self.entities) do
            if e ~= f then
                v = flock.avoid(f, pos, min_dist2, v)
            end
        end
        flock.apply(e, v)
    end
    return self
end

function flock.follow(target, pos, acc, min2, ret)
    local dist <const> = target - pos
    local len2 <const> = dist:len_sq()
    if len2 >= min2 then
        ret = ret + dist * vec3(acc / math.sqrt(len2))
    end
    return ret
end

function flock.avoid(f, pos, min2, ret)
    local dist <const> = pos - vec3(f:pos())
    if dist:len_sq() <= min2 then ret = ret + dist end
    return ret
end

function flock.apply(e, acc)
    e:set_acc(table.unpack(acc))
    if acc[1] == 0 and acc[2] == 0 then
        e:set_vel(0, 0, 0)
    end
end

function sprite:new(entity, track)
    return setmetatable({entity = entity, track = track}, self)
end

function sprite:update()
    self.entity:animation():sprite():set_track(self.track)
    return self.next
end

function textbox:new(title, text)
    return setmetatable({title = title, text = text, state = 0}, self)
end

function textbox:update()
    local s = self.state
    if s == 0 then
        nngn_textbox.set(self.title, self.text)
        self.state = 1
        return self
    end
    if nngn:textbox():empty() then
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

function cycle:new(t)
    local first = chain(t)
    return setmetatable({first = first, cur = first}, self)
end

function cycle:update(dt)
    local n = self.cur:update(dt)
    if not n then n = self.first end
    self.cur = n
    return self
end

local function pause_input() input.pause(); return true end
local function resume_input() input.resume(); return true end

local function stop()
    player.stop()
    nngn_camera.set_follow()
    return true
end

return {
    pause_input = function() return fn:new(pause_input) end,
    resume_input = function() return fn:new(resume_input) end,
    save_state = function(...) return save_state:new(...) end,
    restore_state = function(...) return restore_state:new(...) end,
    stop = function() return fn:new(stop) end,
    timer = function(...) return timer:new(...) end,
    fn = function(...) return fn:new(...) end,
    camera = function(...) return camera:new(...) end,
    camera_rot = function(...) return camera_rot:new(...) end,
    camera_rev = function(...) return camera_rev:new(...) end,
    velocity = function(...) return velocity:new(...) end,
    flock = function(...) return flock:new(...) end,
    sprite = function(...) return sprite:new(...) end,
    textbox = function(...) return textbox:new(...) end,
    cycle = function(t) return cycle:new(t) end,
    chain = chain,
}
