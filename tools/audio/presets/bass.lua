function f(f)
    return audio.add {
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
            audio.dur(rate / 32, audio.add {
                audio.noise(),
                audio.gain(audio.db(-33)),
                audio.exp_fade(0, 1, 0, 0, 4),
            }),
        },
        audio.env(rate / 128, rate / 4, 0.3, 0),
        audio.exp_fade(rate / 4, 1, 0, 0, 1.5),
        audio.fade(rate / -64, 1, 0, 0),
        audio.gain(audio.db(-2)),
    }
end

return audio.dur(4 * rate, f(audio.note "a2"))
