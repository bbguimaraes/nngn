local w <const> = rate * 0.9
local n <const> = audio.note

local notes0 <const> = {
    n"f3", w/1,
    n"ab3", w/1,
    n"g3", 3*w/1,
    n"f3", w/2+w/1,
    n"ab3", w/1,
    n"c4", 3*w/1,
}

local notes1 <const> = {
    n"db2", w/1,
    n"f2", w/1,
    n"eb2", 3*w/1,
    0, w/2,
    n"db2", w/1,
    n"f2", w/1,
    n"ab2", 3*w/1,
}

function ep(f)
    return audio.add {
        audio.harm(audio.sine, 4 * rate, f, {
            audio.db(  0), 0.5, 1,
            audio.db( -2), 4, 2,
            audio.db( -4), 4, 3,
            audio.db(-20), 4, 4,
            audio.db(-30), 4, 5,
            audio.db(-40), 4, 6,
        }),
        audio.fade(0, 0, rate / 256, 1),
        audio.fade(rate / -128, 1, 0, 0),
    }
end

function f0(f)
    return audio.add {
        audio.mix {
            audio.add { ep(f), audio.over(0.5, 0.3) },
            audio.add { ep(f), audio.gain(0.4) },
        },
        audio.trem(0.5, 3, 0.15),
    }
end

function f1(f)
    return audio.add { f0(f), f0(f * 1.5), f0(f * 2) }
end

function f2(g, f, t)
    return audio.gain(audio.db(g), audio.map2(f, t))
end

return audio.mix {
    f2(-13, f0, notes0),
    f2(-18, f1, notes1),
}
