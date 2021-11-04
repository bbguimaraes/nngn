return audio.dur(rate, audio.add {
    audio.sine(audio.note "e4"),
    audio.env(rate / 16, rate / 2, 0.5, rate / 16),
})
