local dur <const> = rate / 10
local notes <const> = {audio.note "g4", audio.note "c5"}

local f <const> = function(f)
    return audio.add {
        audio.sine(f),
        audio.sine(f * 1.5),
        audio.env(rate / 32, rate / 32, 0.5, rate / 32),
        audio.gain(0.5),
    }
end

return audio.map(f, notes, dur)
