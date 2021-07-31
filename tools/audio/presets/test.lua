local f <const> = 440

return audio.dur(rate, audio.mix {
    audio.sine(f),
})
