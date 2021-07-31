local dur <const> = 8 * rate
local f <const> = audio.note "a2"

return audio.dur(dur, audio.add {
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
    audio.fade(0, 0, rate / 64, 1),
    audio.fade(rate / -64, 1, 0, 0),
    audio.exp_fade(rate / 4, 1, dur, 0, 2),
    audio.gain(audio.db(-1)),
})
