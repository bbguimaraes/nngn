local f <const> = audio.note "c3"

return audio.dur(rate, audio.add {
    audio.sine(audio.note "c3"),
    audio.sine(audio.note "c4"),
    audio.sine(audio.note "eb4"),
    audio.sine(audio.note "g4"),
    audio.gain(0.01),
})
