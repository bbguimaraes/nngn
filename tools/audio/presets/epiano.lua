local dur <const> = 4 * rate
local f <const> = audio.note "a3"

return audio.dur(dur, audio.add {
    audio.mix {
        audio.add {
            audio.mix {
                audio.sine(f),
                audio.gain(audio.db(-12), audio.sine(f * 2)),
                audio.gain(audio.db(-17), audio.sine(f * 3)),
            },
            audio.exp_fade(0, 1, dur, 0, 2),
        },
        audio.dur(3 * rate, audio.add {
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
    audio.fade(0, 0, rate / 128, 1),
    audio.fade(rate / -128, 1, 0, 0),
})
