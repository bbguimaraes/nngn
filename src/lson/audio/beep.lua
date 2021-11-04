local d <const> = rate / 20
local g <const> = 0.1
local f <const> = 440

return audio.dur(d, audio.gain(g, audio.sine(f)))
