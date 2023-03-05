local f <const> = audio.note "c3"
local dur <const> = 1.25 * rate
local n <const> = audio.note

local notes0 <const> = {
    n"eb4", dur,
    n"g3", 3 * dur / 4,
    n"d4", dur / 4,
    n"eb4", dur / 4,
    n"d4", dur / 4,
    n"g4", dur / 4,
    n"f4", dur / 4,
    n"eb4", dur / 4,
    n"d4", dur / 4,
    n"g3", dur / 4,
}

local v0 <const> = function(f)
    return audio.add {
        audio.square(f),
        audio.gain(0.25),
        audio.sine(f),
        audio.gain(0.25),
--        audio.env(rate / 8, rate / 8, 0.5, rate / 8),
        audio.env(rate / 128, rate / 4, 0.3, 0),
        audio.exp_fade(rate / 4, 1, 0, 0, 1.5),
        audio.fade(rate / -64, 1, 0, 0),
    }
end

return audio.mix {
    audio.dur(16 * rate, audio.add {
        v0(audio.note "eb5"),
        audio.sine(audio.note "c3"),
        audio.sine(audio.note "c4"),
        audio.sine(audio.note "eb4"),
        audio.sine(audio.note "g4"),
        audio.trem(0.5, 0.25, 0.25),
        audio.gain(0.01),
        audio.exp_fade(0, 0, rate / 4, 1, 4),
    }),
    audio.add {
        audio.map2(v0, notes0),
        audio.gain(1/32),
        audio.trem(0.5, 0.25, 0.25),
    },
}
