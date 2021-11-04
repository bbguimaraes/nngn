local w <const> = rate / 2
local n <const> = audio.note

local notes <const> = {
    n"bb4", w/1, n"f4", w/1,
    0, w/2, n"bb4", w/2, n"bb4", w/4, n"c5", w/4, n"d5", w/4, n"eb5", w/4,
    n"f5", w/1, 0, w/1, 0, w/2, n"f5", w/2,
    n"f5", w/4, n"gb5", w/2, n"ab5", w/4,
    n"bb5", w/1, 0, w/1, 0, w/2, n"bb5", w/2,
    n"bb5", w/4, n"ab5", w/2, n"gb5", w/4, n"ab5", 3*w/4, n"gb5", w/4,
    n"f5", w/1,
}

function f(f)
    return audio.add {
        audio.saw(f),
        audio.gain(audio.db(-10)),
        audio.sine(f),
        audio.env(w/16, w/16, 0.5, w/16),
    }
end

return audio.map2(f, notes)

-- return
-- <components window> audio.map2 <tool tip text>
-- audio.map2(audio.sine, notes)
-- <generate> <no rewind>
-- audio.square
-- audio.saw
-- function f(f) return
-- <from components> audio.env <tool tip text>
-- audio.env(w/8, w/8, 0.5, w/8)
