local n, db <const> = audio.note, audio.db
local n8, n16 <const> = rate / 3, rate / 6

local bass <const> = function(f)
    return audio.gain(db(-5), audio.bass(rate, f))
end
local kick <const> = audio.gain(db(-5), audio.drums.kick(rate, n"a1"))
local snare <const> = audio.gain(db(-8), audio.drums.snare(rate, n"e2"))
local hihat <const> = audio.gain(db(-35), audio.drums.hihat(rate, n"a5", 0.25))

return audio.mix {
    audio.map2(bass, {
        n"e2",  n8,                 0, n16, n"e2", n16,
        n"e3", n16, n"e3", n16,     0, n16, n"e3",  n8,
                    n"d3", n16, n"b2", n16, n"e2", n16,
        n"a2", n16, n"b2", n16, n"g2",  n8,
        n"e2",  n8,             n"d2", n16, n"eb2", n16,
        n"e2", n16, n"g2", n16, n"e2", n16, n"d2",  n8 + n16,
                                n"a1", n16, n"bb1", n16,
        n"b1", n16, n"d2", n16, n"b1", n8,
        n"e2", rate,
    }),
    audio.seq(n16, {
        kick,  "|o  o      o    o|o  o      o   o |o",
        snare, "|    o       o   |    o       o   | ",
        hihat, "|o o o o o o o o |o o o o o o o o |o",
    }),
}
