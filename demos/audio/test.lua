require "src/lua/path"
local input <const> = require "nngn.lib.input"
local utils <const> = require "nngn.lib.utils"

nngn:set_graphics(Graphics.PSEUDOGRAPH)
nngn:input():add_source(Input.terminal_source(0, Input.OUTPUT_PROCESSING))
input.install()

local audio <const> = require "nngn.lib.audio"
local r <const> = 44100.0
local bpm <const> = 120
local dur <const> = function(n, div)
    return audio.note_dur(r, bpm, n, div)
end
local n1 <const> = dur(4, 1)
local n2 <const> = dur(4, 2)
local n4 <const> = dur(4, 4)
local n8 <const> = dur(4, 8)
local n16 <const> = dur(4, 16)
local n32 <const> = dur(4, 32)
local n64 <const> = dur(4, 64)
local a <const> = nngn:audio()
assert(a:init(nngn:math(), r))
local n <const> = audio.note
local metronome <const> = function(n, d)
    local m0 <const> = audio.dur(n4, audio.dur(r / 16, audio.sine(1760)))
    local m1 <const> = audio.dur(n4, audio.dur(r / 16, audio.sine(880)))
    local ret = {}
    for i = 0, n // d - 1 do
        table.insert(ret, m0)
        for di = 1, d - 1 do
            table.insert(ret, m1)
        end
    end
    if n % d ~= 0 then
        table.insert(ret, m0)
        for di = 1, n % d - 1 do
            table.insert(ret, m1)
        end
    end
    return audio.cat(ret)
end
local smooth_sine <const> = function(f)
    return audio.add {
        audio.sine(f),
        audio.fade(0, 0, r / 64, 1),
        audio.fade(r / -64, 1, 0, 0),
    }
end
local epiano_dur <const> = 4 * r
local epiano <const> = function(f)
    return audio.dur(epiano_dur, audio.add {
        audio.mix {
            audio.add {
                audio.mix {
                    audio.sine(f),
                    audio.gain(audio.db(-12), audio.sine(f * 2)),
                    audio.gain(audio.db(-17), audio.sine(f * 3)),
                },
                audio.exp_fade(0, 1, epiano_dur, 0, 2),
            },
            audio.dur(3 * r, audio.add {
                audio.mix {
                    audio.gain(audio.db(-23), audio.sine(f * 4)),
                    audio.gain(audio.db(-25), audio.sine(f * 5)),
                    audio.gain(audio.db(-38), audio.sine(f * 6)),
                    audio.gain(audio.db(-35), audio.sine(f * 7)),
                    audio.gain(audio.db(-55), audio.sine(f * 8)),
                },
                audio.exp_fade(0, 1, 0, 0, 2),
            }),
        },
        audio.fade(0, 0, r / 128, 1),
        audio.fade(r / -128, 1, 0, 0),
        audio.gain(audio.db(-8)),
    })
end
local bass_dur <const> = 8 * r
local bass <const> = function(f)
    return audio.dur(bass_dur, audio.add {
        audio.mix {
            audio.sine(f),
            audio.gain(audio.db(-15), audio.sine(f * 2)),
            audio.add {
                audio.mix {
                    audio.gain(audio.db(-40), audio.sine(f * 3)),
                    audio.gain(audio.db(-30), audio.sine(f * 4)),
                    audio.gain(audio.db(-40), audio.sine(f * 5)),
                },
            },
        },
        audio.fade(0, 0, r / 64, 1),
        audio.fade(r / -64, 1, 0, 0),
        audio.exp_fade(r / 4, 1, bass_dur, 0, 2),
        audio.gain(audio.db(-1)),
    })
end
local kick <const> = function(f)
    return audio.dur(r / 16, audio.add {
        audio.mix {
            audio.gain(0.01, audio.noise()),
            audio.gain(0.99, audio.sine(f)),
        },
        audio.exp_fade(0, 0, r / 16, 1, 2),
        audio.exp_fade(0, 1, r / -16, 0, 2),
    })
end
local snare <const> = function(f)
    return kick(f * 2)
end
local bass_square <const> = function(d, f)
    return audio.mix {
        audio.dur(d, bass(f)),
        audio.add {
            audio.dur(d, audio.square(f)),
            audio.exp_fade(0, 0, 2 * n1, 1, 4),
            audio.gain(audio.db(-25)),
        },
    }
end
local lead <const> = function(f)
    return audio.mix {
            audio.sine(f / 2),
            audio.sine(f),
            audio.sine(f * 2),
            audio.gain(audio.db(-8), audio.saw(f / 2)),
            audio.gain(audio.db(-8), audio.saw(f)),
        }
end
local lead_env <const> = function(f)
    return audio.add {
        lead(f),
        audio.exp_fade(0, 0, r / 32, 1, 8),
        audio.exp_fade(r / -32, 1, 0, 0, 2),
    }
end
local first0 <const> = utils.map({
    "a4", "b4", "c5", "e5", "a5", "c6", "b5", "a5",
    "a4", "b4", "c5", "e5", "a5", "c6", "b5", "a5",
    "a4", "b4", "c5", "e5", "a5", "c6", "b5", "g#4",
    "g#5", "b5", "e5", "g#5", "d5", "e5", "c5", "d5",
}, audio.note, ipairs)
local first1 <const> = utils.map({
    "a4", "b4", "c5", "e5", "a5", "c6", "b5", "g#4",
    "a4", "b4", "c5", "e5", "a5", "c6", "b5", "a5",
    "a4", "b4", "c5", "e5", "a5", "c6", "b5", "g#4",
    "g#5", "b5", "e5", "g#5", "d5", "e5", "c5", "d5",
}, audio.note, ipairs)
local first2 <const> = utils.map({
    "a4", "b4", "c5", "e5", "a5", "b5", "c6", "e6",
}, audio.note, ipairs)
local first3 <const> = utils.map({
    "a4", "b4", "c5", "e5", "a5", "b5", "c6",
}, audio.note, ipairs)
local first4 <const> = utils.map({
    "a4", "g#4", "a4", "b4", "c5", "d5", "e5", "f5", "g#5",
}, audio.note, ipairs)
local first5 <const> = {
    n"e6", n4 + n8 + n16,
    n"b5", n16, n"e5", n16, n"g#5", n16, n"d5", n16,
    n"e5", n16,
}
local first6 <const> = {n"b4",  n"d5"}
local bass1 <const> = {
    n"a1", n1 - n8,                           n"a1", n8,
    n"a1",      n8,     0, n8, n"a1", 3 * n4,
    n"f1", n1 - n8, n"f1", n8,
    n"f1",      n8,     0, n8, n"f1", 3 * n4,
    n"g1", n1 - n8,                           n"g1", n8,
    n"e1", n2, n"c1", n2,
    n"a1", 2 * n1,
}
local second0 <const> = {n"a3", n8, n"b3", n4}
local second1 <const> = {
               n"g4", n8, n"a3", n8, n"b3", n8,
    n"c4", n8, n"g4", n8, n"a3", n8, n"b3", n8,
    n"c4", n8, n"b3", n8, n"c4", n8, n"g4", n8,
    n"a3", n8, n"c4", n8, n"a3", n8, n"b3", n4,
}
local second2 <const> = {
                 n"g4", n4,              n"g4", n8,
     n"g3", n8,      0, n8, n"g3", n8,  n"g#3", n4,
                n"g#4", n4,             n"g#4", n8,
    n"g#3", n8,      0, n8, n"g#3", n4,
}
local lead0 <const> = {
    n"a3", n16 + n64, n"c4", n64, n"e4", n64, n"g4", n64,
    n"a4", n8 + 3 * n4,
    n"a3", n16 + n64, n"c4", n64, n"e4", n64, n"g4", n64,
    n"c5", n8 + 3 * n4,
}
local lead1 <const> = {
    n"a4", n8,
    n"b4", n8 + n2 + n4, n"c5", n8,
    n"a4", n8 + n2 + n4, n"a4", n8,
    n"b4", n8 + n1 + n8 + n16,
}
local lead2 <const> = {
    n"d5", n64, n"c5", n64, n"b4", n64, n"a4", n64,
    n"g#4", n8, n"a4", n8, n"b4", n8, n"d5", n8, n"c5", n8,
}
local lemon_bass <const> = {
    n"e2",  n8,                 0, n16, n"e2", n16,
    n"e3", n16, n"e3",  n8,             n"e3",  n8,
                n"d3", n16, n"b2", n16, n"e2", n16,
    n"a2", n16, n"b2", n16, n"g2",  n8,
    n"e2",  n8,             n"d2", n16, n"eb2", n16,
    n"e2", n16, n"g2", n16, n"e2", n16, n"d2",  n8 + n16,
                            n"a1", n16, n"bb1", n16,
    n"b1", n16, n"d2", n16, n"b1", n8,
    n"e2", bass_dur,
}
local lemon_kick <const> = {
    n"e2", n2, n"e2", n2, n"e2", n2, n"e2", n2,
}
local lemon_snare <const> = {
    0, n2, n"e2", n2, n"e2", n2, n"e2", n2, n"e2", n2,
}
local b, n <const> = audio.gen(a, audio.cat {
    audio.mix {
        audio.gain(audio.db(-25), audio.cat {
            audio.dur(5 * n1, audio.add {
                audio.chord({n"a4", n"c5", n"e5"}, audio.sine),
                audio.fade(0, 0, n1, 1),
            }),
            audio.dur(n1, audio.chord({n"g4", n"b4", n"d5"}, audio.sine)),
            audio.dur(n1, audio.add {
                audio.chord({n"g#4", n"b4", n"d5"}, audio.sine),
                audio.fade(-n1, 1, 0, 0),
            }),
        }),
        audio.gain(audio.db(-2), audio.cat {
            audio.seek(n1),
            bass_square(2 * n1, n"a1"),
            bass_square(2 * n1, n"f1"),
            bass_square(n1, n"g1"),
            bass_square(n1, n"e1"),
        }),
        audio.add {
            audio.cat {
                audio.seek(n1),
                audio.gain(audio.db(-5), audio.map(audio.square, first0, n16)),
                audio.gain(audio.db(-2), audio.map(audio.saw, first1, n16)),
                audio.gain(audio.db(-5), audio.map(audio.square, first2, n16)),
                audio.gain(audio.db(-2), audio.map(audio.saw, first3, n16)),
                audio.add {
                    audio.gain(audio.db(-2),
                        audio.map(audio.square, first4, n16)),
                    audio.map(audio.saw, first4, n16),
                },
                audio.add {
                    audio.gain(audio.db(-2), audio.map2(audio.square, first5)),
                    audio.map2(audio.saw, first5),
                },
            },
            audio.fade(n1, 0, n1 + n16, 1),
            audio.gain(audio.db(-27)),
        },
    },
--    audio.gain(audio.db(-30), metronome(3, 4)),
    audio.mix {
        audio.gain(audio.db(-12), audio.cat {
            audio.seek(n4),
            audio.dur(2 * n1, audio.chord({n"a2", n"e3", n"c4"}, epiano)),
            audio.dur(2 * n1, audio.chord({n"f2", n"c3", n"a3"}, epiano)),
            audio.dur(n2, audio.chord({n"g2", n"d3"}, epiano)),
            audio.dur(n2, audio.chord({n"g2", n"d3", n"b3"}, epiano)),
            audio.dur(n2, audio.chord({n"e2", n"b2"}, epiano)),
            audio.dur(n2, audio.chord({
                n"e3", n"g#3", n"e3", n"b3", n"d4",
            }, epiano)),
            audio.dur(2 * n1, audio.chord({n"a2", n"e3", n"c4"}, epiano)),
        }),
        audio.gain(audio.db(-2), audio.cat {
            audio.seek(n4),
            audio.map2(bass, bass1),
        }),
        audio.gain(audio.db(-15), audio.cat {
            audio.map2(epiano, second0),
            audio.map2(epiano, second1),
            audio.map2(epiano, second1),
            audio.map2(epiano, second2),
        }),
        audio.add {
            audio.map(audio.square, first6, n16),
            audio.map(audio.saw, first6, n16),
            audio.gain(audio.db(-30)),
        },
        audio.gain(audio.db(-28), audio.cat {
            audio.map2(lead, lead0),
            audio.map2(lead_env, lead1),
            audio.map2(lead, lead2),
            audio.dur(2 * n1, audio.add {
                audio.mix {
                    lead_env(n"c5"),
                    audio.exp_fade(0, 0, 0, 1, 4, audio.saw(n"a3")),
                },
                audio.exp_fade(r / -8, 1, 0, 0, 2),
            }),
        }),
    },
--    audio.map2(bass, lemon_bass),
})
local master <const> = Compute.create_vector(2 * n)
a.normalize(master, b)
local source <const> = assert(a:add_source(master))
assert(a:play(source))
