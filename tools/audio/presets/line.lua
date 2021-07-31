local dur <const> = rate / 8
local n <const> = audio.note
local notes <const> = {n"a4", n"b4", n"c5", n"e5"}

local f <const> = function(f)
    return audio.env(
        rate / 32, rate / 32, 0.5, rate / 32,
        audio.sine(f))
end

return audio.map(f, notes, dur)
