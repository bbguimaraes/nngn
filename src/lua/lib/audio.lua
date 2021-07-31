local utils <const> = require "nngn.lib.utils"
local class <const> = utils.class

local note_dur <const> = function(r, bpm, n, div)
    return n * r * 60 / (div * bpm)
end

local byte_0, byte_sharp, byte_flat <const> = string.byte("0")

local note_off0 <const> = {
    [string.byte("b")] = 2,
    [string.byte("c")] = 3,
    [string.byte("d")] = 5,
    [string.byte("e")] = 7,
    [string.byte("f")] = 8,
    [string.byte("g")] = 10,
}

local note_off1 <const> = {
    [string.byte("#")] = 1,
    [string.byte("b")] = -1,
}

local note_exp <const> = math.pow(2, 1/12)
local note <const> = function(s)
    local oct = string.byte(s, -1) - byte_0
    local n, o = string.byte(s, 1, -2)
    n = (note_off0[n] or 0) + (note_off1[o] or 0)
    if n > 2 then
        oct = oct - 1
    end
    return 55 * (1 << (oct - 1)) * math.pow(note_exp, n)
end

local no_len <const> = class "audio.no_len" {len = function() end}
local child_len <const> = class "audio.child_len" {}
local max_len <const> = class "audio.max_len" {}
local list_len <const> = class "audio.list_len" {}
local nop <const> = class "audio.nop" : public(no_len) {}
local seek <const> = class "audio.seek" {}
local sine <const> = class "audio.sine" : public(no_len) {}
local sine_fm <const> = class "audio.sine_fm" : public(no_len) {}
local square <const> = class "audio.square" : public(no_len) {}
local saw <const> = class "audio.saw" : public(no_len) {}
local noise <const> = class "audio.noise" : public(no_len) {}
local chord <const> = class "audio.chord" : public(child_len) {}
local slide <const> = class "audio.slide" : public(no_len) {}
local gain <const> = class "audio.gain" : public(child_len) {}
local fade <const> = class "audio.fade" : public(child_len) {}
local exp_fade <const> = class "audio.exp_fade" : public(fade) {}
local env <const> = class "audio.env" : public(child_len) {}
local dur <const> = class "audio.dur" {}
local map <const> = class "audio.map" {}
local map2 <const> = class "audio.map2" {}
local cat <const> = class "audio.cat" : public(list_len) {}
local add <const> = class "audio.add" : public(max_len) {}
local mix <const> = class "audio.mix" : public(max_len) {}
local harm <const> = class "audio.harm" {}
local seq <const> = class "audio.seq" : public(mix) {}
local fn <const> = class "audio.fn" : public(no_len) {}

function child_len:len()
    if self.c then
        return self.c:len()
    end
end

function max_len:len()
    local ret
    for _, x in ipairs(self) do
        local n <const> = x:len()
        if n then
            ret = math.max(ret or 0, n)
        end
    end
    return ret
end

function list_len:len()
    local l, ret = {}, 0
    for i, x in ipairs(self) do
        local n <const> = x:len()
        l[i] = n
        ret = ret + n
    end
    self._len = l
    return ret
end

function nop.new()
    return setmetatable({}, nop)
end

function nop:gen(_, _, b, e)
    return e - b
end

function seek.new(n, c)
    return setmetatable({n = n, c = c}, seek)
end

function seek:len()
    if self.c then
        return self.n + self.c:len()
    end
    return self.n
end

function seek:gen(a, dst, b, e)
    local ret <const> = e - b
    if self.c then
        self.c:gen(a, dst, b + self.n, e)
    end
    return ret
end

function sine.new(freq)
    return setmetatable({freq = freq}, sine)
end

function sine:gen(a, dst, b, e)
    a:sine(dst, b, e, self.freq)
end

function sine_fm.new(freq, lfo_a, lfo_freq, lfo_d)
    return setmetatable({
        freq = freq, lfo_a = lfo_a, lfo_freq = lfo_freq, lfo_d = lfo_d,
    }, sine_fm)
end

function sine_fm:gen(a, dst, b, e)
    a:sine_fm(dst, b, e, self.freq, self.lfo_a, self.lfo_freq, self.lfo_d or 0)
end

function square.new(freq)
    return setmetatable({freq = freq}, square)
end

function square:gen(a, dst, b, e)
    a:square(dst, b, e, self.freq)
end

function saw.new(freq)
    return setmetatable({freq = freq}, saw)
end

function saw:gen(a, dst, b, e)
    a:saw(dst, b, e, self.freq)
end

function noise.new()
    return setmetatable({}, noise)
end

function noise:gen(a, dst, b, e)
    a:noise(dst, b, e)
end

function chord.new(t, c)
    return setmetatable({t = t, c = c}, chord)
end

function chord:gen(a, dst, b, e)
    local c <const> = self.c
    local ret <const> = e - b
    for _, x in ipairs(self.t) do
        c(x):gen(a, dst, b, e)
    end
    return ret
end

function slide.new(freq0, freq1, steps, c)
    return setmetatable({
        freq0 = freq0, freq1 = freq1, steps = steps, c = c,
    }, slide)
end

function slide:gen(a, dst, b, e)
    local ret <const> = e - b
    local d <const> = ret / self.steps
    local i = b
    while i < e do
        local di <const> = math.min(d, e - i)
        local t <const> = (i + di / 2 - b) / ret
        local f <const> = self.freq0 + (t * (self.freq1 - self.freq0))
        self.c(f):gen(a, dst, i, i + d)
        i = i + d
    end
    return ret
end

function gain.new(g, c)
    return setmetatable({g = g, c = c}, gain)
end

function gain:gen(a, dst, b, e)
    if self.c then
        self.c:gen(a, dst, b, e)
    end
    a.gain(dst, b, e, self.g)
    return e - b
end

function fade.new(bp, bg, ep, eg, c)
    return setmetatable({bp = bp, bg = bg, ep = ep, eg = eg, c = c}, fade)
end

function fade:gen(a, dst, b, e)
    local ret <const> = e - b
    local ep, t <const> = self:gen_child(a, dst, b, e)
    local bp <const> = math.min(e, self.pos(b, e, self.bp))
    local eg <const> = self.bg + t * (self.eg - self.bg)
    a.fade(dst, bp, ep, self.bg, eg)
    return ret
end

function fade:gen_child(a, dst, b, e)
    if self.c then
        self.c:gen(a, dst, b, e)
    end
    local t = 1
    if self.ep ~= 0 then
        local ep <const> = self.pos(b, e, self.ep)
        if ep < e then
            t = 1
            e = ep
        else
            t = (e - b) / (ep - b)
        end
    end
    return e, t
end

function fade.pos(b, e, p)
    if p < 0 then
        return math.max(b, e + p)
    else
        return b + p
    end
end

function exp_fade.new(bp, bg, ep, eg, exp, c)
    return setmetatable({
        bp = bp, bg = bg, ep = ep, eg = eg, exp = exp, c = c,
    }, exp_fade)
end

function exp_fade:gen(a, dst, b, e)
    if c then
        self.c:gen(a, dst, b, e)
    end
    local ep = e
    if self.ep ~= 0 then
        ep = self.pos(b, e, self.ep)
    end
    a.exp_fade(
        dst, math.min(e, self.pos(b, e, self.bp)), math.min(e, ep), ep,
        self.bg, self.eg, self.exp)
    return e - b
end

function env.new(a, d, s, r, c)
    return setmetatable({
        attack= a, decay = d, sustain = s, release = r, c = c
    }, env)
end

function env:gen(a, dst, b, e)
    if self.c then
        self.c:gen(a, dst, b, e)
    end
    a.env(dst, b, e, self.attack, self.decay, self.sustain, self.release)
end

function dur.new(d, c)
    return setmetatable({d = d, c = c}, dur)
end

function dur:len()
    return self.d
end

function dur:gen(a, dst, b, e)
    self.c:gen(a, dst, b, b + math.min(self.d, e - b))
    return e - b
end

function map.new(f, t, dur)
    return setmetatable({f = f, t = t, dur = dur}, map)
end

function map:len()
    return #self.t * self.dur
end

function map:gen(a, dst, b, e)
    local f, dur <const> = self.f, self.dur
    for _, x in ipairs(self.t) do
        f(x):gen(a, dst, b, b + dur)
        b = b + dur
    end
    return #self.t * dur
end

function map2.new(f, t)
    return setmetatable({f = f, t = t, dur = dur}, map2)
end

function map2:len()
    local t <const> = self.t
    local ret = 0
    for i = 2, #t, 2 do
        ret = ret + t[i]
    end
    return ret
end

function map2:gen(a, dst, b, e)
    local f, t, ret <const> = self.f, self.t, e - b
    for i = 1, #self.t, 2 do
        local dur <const> = t[i + 1]
        f(t[i]):gen(a, dst, b, b + dur)
        b = b + dur
    end
    return ret
end

function cat.new(t)
    return setmetatable(t, cat)
end

function cat:gen(a, dst, b, e)
    local ret <const> = e - b
    for i, x in ipairs(self) do
        local n <const> = self._len[i]
        x:gen(a, dst, b, b + n)
        b = b + n
    end
    assert(b <= e)
    return ret
end

function add.new(t)
    return setmetatable(t, add)
end

function add:gen(a, dst, b, e)
    for _, x in ipairs(self) do
        x:gen(a, dst, b, e)
    end
    return e - b
end

function mix.new(t)
    return setmetatable(t, mix)
end

function mix:gen(a, dst, b, e)
    local n <const> = Compute.SIZEOF_FLOAT * math.ceil(e)
    local tmp <const> = Compute.create_vector(n)
    for i, x in ipairs(self) do
        if i ~= 1 then
            Compute.zero_vector(tmp, 0, n)
        end
        x:gen(a, tmp, b, e)
        a.mix(dst, tmp)
    end
    return e - b
end

function harm.new(c, d, f, t)
    local ret <const> = {}
    for i = 1, #t, 3 do
        table.insert(ret, add.new {
            c(f * t[i + 2]),
            exp_fade.new(0, 1, d, 0, t[i + 1]),
            gain.new(t[i]),
        })
    end
    return mix.new(ret)
end

function seq.new(d, t)
    local ret <const> = {}
    for i = 1, #t, 2 do
        table.insert(ret, cat.new(seq.parse(t[i], t[i + 1], d)))
    end
    local len
    if #t ~= 0 then
        len = d * #t[2]
    end
    return mix.new(ret)
end

function seq.parse(c, s, d)
    local ch_o, ch_sp <const> = string.byte("o ", 1, 2)
    local ret <const> = {}
    local rests = 0
    for i = 1, #s do
        local ic <const> = string.byte(s, i)
        if ic == ch_o then
            if rests == 0 then
                table.insert(ret, dur.new(d, c))
            else
                table.insert(ret, seek.new(d * rests, dur.new(d, c)))
                rests = 0
            end
        elseif ic == ch_sp then
            rests = rests + 1
        end
    end
    return ret
end

function fn.new(f)
    return setmetatable({f = f}, fn)
end

function fn:gen(...) return self.f(...) end

function bass(r, f)
    local d <const> = 8 * r
    return dur.new(d, add.new {
        mix.new {
            sine.new(f),
            gain.new(Audio.db(-15), sine.new(f * 2)),
            add.new {
                mix.new {
                    gain.new(Audio.db(-40), sine.new(f * 3)),
                    gain.new(Audio.db(-30), sine.new(f * 4)),
                    gain.new(Audio.db(-40), sine.new(f * 5)),
                },
            },
        },
        fade.new(0, 0, rate / 64, 1),
        fade.new(rate / -64, 1, 0, 0),
        exp_fade.new(rate / 4, 1, d, 0, 2),
        gain.new(Audio.db(-1)),
    })
end

local drums_kick_f <const> = note "a1"
local drums_kick <const> = function(rate, f)
    local dur <const> = rate / 4
    return audio.dur(dur, audio.add {
        audio.mix {
            audio.gain(0.9, audio.sine_fm(f / 1.5, 0.5, 2)),
            audio.gain(0.05, audio.sine(f * 2)),
            audio.gain(0.01, audio.sine(f * 3)),
            audio.gain(0.01, audio.sine(f * 4)),
            audio.dur(dur, audio.add {
                audio.noise(),
                audio.fade(0, 1, 0, 0),
                audio.gain(0.01)
            }),
        },
        audio.exp_fade(dur / -2, 1, 0, 0, 4),
    })
end

local drums_snare_f <const> = note "e2"
local drums_snare <const> = function(rate, f)
    if f == 0 then return audio.nop() end
    local dur <const> = rate / 4
    return audio.dur(dur, audio.add {
        audio.mix {
            audio.gain(0.10, audio.sine_fm(f / 2, 1, 0.5)),
            audio.gain(0.70, audio.sine_fm(f * 1, 1, 0.5)),
            audio.gain(0.05, audio.sine_fm(f * 2, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 3, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 4, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 5, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 6, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 7, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 8, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 9, 1, 0.5)),
            audio.gain(0.01, audio.sine_fm(f * 10, 1, 0.5)),
            audio.gain(0.30, audio.noise()),
        },
        audio.exp_fade(0, 1, 0, 0, 2),
    })
end

local drums_hihat_f <const> = note "a5"
local drums_hihat <const> = function(rate, f, open)
    if f == 0 then return audio.nop() end
    local dur <const> = rate
    return audio.mix {
        audio.dur(rate / 10, audio.add {
            audio.mix {
                audio.gain( 8, audio.square(f / 16)),
                audio.gain( 8, audio.square(f / 8)),
                audio.gain( 8, audio.square(f / 4)),
                audio.gain( 1, audio.square(f / 2)),
                audio.gain( 2, audio.square(f * 1)),
                audio.gain( 1, audio.square(f * 2)),
                audio.gain( 1, audio.square(f * 3)),
                audio.gain( 2, audio.square(f * 4.16)),
                audio.gain( 2, audio.square(f * 5.43)),
                audio.gain( 4, audio.square(f * 6.79)),
                audio.gain( 2, audio.square(f * 8.21)),
                audio.gain( 1, audio.square(f * 9)),
                audio.gain( 1, audio.square(f * 10)),
                audio.gain( 1, audio.square(f * 11)),
                audio.gain( 1, audio.square(f * 12)),
                audio.gain( 1, audio.square(f * 13)),
                audio.gain( 1, audio.square(f * 14)),
                audio.gain( 1, audio.square(f * 15)),
                audio.gain( 1, audio.square(f * 16)),
                audio.gain( 1, audio.square(f * 17)),
                audio.gain( 1, audio.square(f * 18)),
                audio.gain( 1, audio.square(f * 19)),
                audio.gain(32, audio.square(f * 20)),
                audio.gain(32, audio.square(f * 21)),
                audio.gain(32, audio.square(f * 22)),
                audio.gain(32, audio.square(f * 23)),
                audio.gain(32, audio.square(f * 24)),
                audio.gain(32, audio.square(f * 25)),
                audio.gain(32, audio.square(f * 26)),
                audio.gain(32, audio.square(f * 27)),
                audio.gain(32, audio.square(f * 28)),
                audio.gain(32, audio.square(f * 29)),
            },
            audio.exp_fade(0, 1, 0, 0, 4),
            audio.gain(1/64),
        }),
        audio.dur(rate * open, audio.add {
            audio.noise(),
            audio.exp_fade(0, 1, 0, 0, 2),
            audio.gain(0.3),
        }),
    }
end

function gen(a, c)
    local n = c:len()
    if not n then
        error("audio has no length")
    end
    n = math.ceil(n)
    local dst <const> =
        Compute.create_vector(Compute.SIZEOF_FLOAT * n)
    c:gen(a, dst, 0, n)
    return dst, n
end

return {
    db = Audio.db,
    note_dur = note_dur,
    note = note,
    nop = nop.new,
    seek = seek.new,
    sine = sine.new,
    sine_fm = sine_fm.new,
    square = square.new,
    saw = saw.new,
    noise = noise.new,
    chord = chord.new,
    slide = slide.new,
    gain = gain.new,
    fade = fade.new,
    exp_fade = exp_fade.new,
    env = env.new,
    map = map.new,
    dur = dur.new,
    map2 = map2.new,
    cat = cat.new,
    add = add.new,
    mix = mix.new,
    harm = harm.new,
    seq = seq.new,
    fn = fn.new,
    bass = bass,
    drums = {
        kick = drums_kick,
        snare = drums_snare,
        hihat = drums_hihat
    },
    gen = gen,
}
