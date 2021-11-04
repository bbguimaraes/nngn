local n <const> = audio.note
local n1 <const> = rate / 0.375
local n2 <const> = rate / 0.75
local n4 <const> = rate / 1.5
local n8 <const> = rate / 3
local n16 <const> = rate / 6

local kick_f  <const> = n"a1"
local snare_f <const> = n"e2"
local hihat_f <const> = n"a5"

local kick_dur  <const> = rate / 4
local snare_dur <const> = rate / 4
local hihat_dur <const> = rate

local _kick <const> = function(f)
    return audio.dur(kick_dur, audio.add {
        audio.mix {
            audio.gain(0.9, audio.sine_fm(f / 1.5, 0.5, 2)),
            audio.gain(0.05, audio.sine(f * 2)),
            audio.gain(0.01, audio.sine(f * 3)),
            audio.gain(0.01, audio.sine(f * 4)),
            audio.dur(kick_dur, audio.add {
                audio.noise(),
                audio.fade(0, 1, 0, 0),
                audio.gain(0.01)
            }),
        },
        audio.exp_fade(kick_dur / -2, 1, 0, 0, 4),
    })
end

local _snare <const> = function(f)
    if f == 0 then return audio.nop() end
    return audio.dur(snare_dur, audio.add {
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

local _hihat <const> = function(f, open)
    if f == 0 then return audio.nop() end
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

local kick <const> = audio.gain(audio.db( -5), _kick(kick_f))
local snare0 <const> = audio.gain(audio.db(-10), _snare(snare_f))
local snare1 <const> = audio.gain(audio.db(-22), _snare(snare_f))
local hihat0 <const> = audio.gain(audio.db(-25), _hihat(hihat_f, 0.25))
local hihat1 <const> = audio.gain(audio.db(-32), _hihat(hihat_f, 0.25))

return audio.seq(n16, {
    kick,   "|o      o|oo o  o |o o    o|oo o o  |",
    snare0, "|    o   |    o   |    o   |    o oo|",
    snare1, "|     o  | o o o  | o   o  |  o  o  |",
    hihat0, "|o       |o       |o       |o       |",
    hihat1, "| ooooooo| ooooooo| ooooooo| ooooooo|",
})
