#include "components.h"

#include <optional>

#include <QVBoxLayout>
#include <QToolButton>

#include "tools/utils.h"

#include "component_button.h"
#include "flow_grid_layout.h"

using namespace std::string_view_literals;

namespace {

constexpr auto add_str = "audio.add {...}"sv;
constexpr auto add_desc = R"(

Serially applies each item in the sequence over the entire contents of the
buffer.

Example:

audio.add {
    audio.sine(440),
    audio.gain(0.5),
    audio.fade(0, 0, 0, 1),
}
)"sv;

constexpr auto cat_str = "audio.cat {...}"sv;
constexpr auto cat_desc = R"(

Concatenates every item in the sequence.

Example:

audio.cat {
    audio.dur(rate, audio.sine(440)),
    audio.dur(rate, audio.sine(660)),
    audio.dur(rate, audio.sine(880)),
}
)"sv;

constexpr auto chord_str = "audio.chord{n0, n1, ...}"sv;
constexpr auto chord_desc = R"(

Applies a generator to all notes in the sequence.

Example:

audio.chord({440, 880, 1320, 1760}, audio.sine)
)"sv;

constexpr auto dur_str = "audio.dur(dur)"sv;
constexpr auto dur_desc = R"(

Limits the duration to `dur` samples.

Example:

audio.dur(rate, audio.sine(440))
)"sv;

constexpr auto env_str = "audio.env(a, d, s, r)"sv;
constexpr auto env_desc = R"(

Attack/delay/sustain/release envolope.

Example:

audio.env(rate, rate, 0.5, rate, audio.sine(440))
)"sv;

constexpr auto exp_fade_str = "audio.exp_fade(pos0, gain0, pos1, gain1, exp)"sv;
constexpr auto exp_fade_desc = R"(

Exponential fade.  Audio level is interpolated exponentially with exponent
`exp`.  Other parameters are the same as `fade`.

Example:

audio.exp_fade(0, 0, 0, 1, 2) -- quadratic fade over the entire sample
                              -- from silence to normal
)"sv;

constexpr auto fade_str = "audio.fade(pos0, gain0, pos1, gain1)"sv;
constexpr auto fade_desc = R"(

Linear fade.  Audio level is interpolated linearly from `gain0` to `gain1` in
the range `[pos0, pos1)`.  Both `pos0` and `pos1` can be `-n`, meaning "`n`
samples before the end".  `pos1 == 0` means "at the end".

Example:

audio.fade(0, 0, 0, 1) -- linear fade over the entire sample from silence to
                       -- normal
)"sv;

constexpr auto fn_str = "audio.fn(function() end)"sv;
constexpr auto fn_desc = R"(

Applies an arbitrary function.  Arguments are the `Audio` object, destination
buffer, and sample range (`[b, e)`).

Example:

audio.fn(function(a, dst, b, e)
    audio.sine(a, dst, b, e)
end)
)"sv;

constexpr auto gain_str = "audio.gain(mul)"sv;
constexpr auto gain_desc = R"(

Multiplies the level of each sample by `mul`.

Example:

audio.gain(0.5, audio.sine(440))
audio.add { audio.sine(440), audio.gain(audio.db(-2)) }
)"sv;

constexpr auto harm_str =
    "audio.harm"
    "(gen, dur, freq, {gain0, fade0, mul0, gain1, fade1, mul1, ...})"sv;
constexpr auto harm_desc = R"(

Generates a harmonic sequence using the generator `gen`, duration `dur`, and
input frequency `freq`.  Every group of three items in the sequence denotes the
gain, exponential fade factor, and frequency multiplier of one harmonic.

Example:

audio.harm(audio.sine, rate, 440 {
    0x1p-1  rate, 1,
    0x1p-2  rate, 2,
    0x1p-3  rate, 3,
    0x1p-4, rate, 4,
})
)"sv;

constexpr auto map_str = "audio.map(f, {...}, dur)"sv;
constexpr auto map_desc = R"(

Applies the function `f` to each frequency in the sequence, each with duration
`dur`.

Example:

audio.map(audio.sine, {440, 880, 1320, 1760}, rate)
)"sv;

constexpr auto map2_str = "audio.map2(f, {...})"sv;
constexpr auto map2_desc = R"(

Applies the function `f` to each group of frequency/duration in the sequence.

Example:

audio.map2(audio.sine, {440, rate, 880, rate, 1320, rate, 1760, rate})
)"sv;

constexpr auto mix_str = "audio.mix {...}"sv;
constexpr auto mix_desc = R"(

Similar to `add`, but each item in the sequence is generated in its own buffer,
independently of the others.

Example:

audio.dur(rate, audio.mix {
    audio.gain(0.5,  audio.sine(440)),
    audio.gain(0.25, audio.sine(880)),
})
)"sv;

constexpr auto noise_str = "audio.noise()"sv;
constexpr auto noise_desc = R"(

Generates white noise (random).

Example:

audio.noise()
)"sv;

constexpr auto nop_str = "audio.nop()"sv;
constexpr auto nop_desc = R"(

Does nothing.

Example:

audio.nop()
)"sv;

constexpr auto over_str = "audio.over(mul, mix)"sv;
constexpr auto over_desc = R"(

Exponential overdrive.  `mul` is the input multiplier, `mix` is the linear
interpolation factor from the original signal (0) to the overddriven one (1).

Example:

audio.over(0.5, 0.2, audio.sine(440))
)"sv;

constexpr auto rep_str = "audio.rep(f)"sv;
constexpr auto rep_desc = R"(

Repeats a generator indefinitely.

Example:

metronome = audio.rep(audio.dur(w/2, audio.dur(rate/20, audio.sine(880))))
)"sv;

constexpr auto saw_str = "audio.saw(freq)"sv;
constexpr auto saw_desc = R"(

Produces a saw tooth wave.

Example:

audio.saw(440)
)"sv;

constexpr auto seek_str = "audio.seek(n)"sv;
constexpr auto seek_desc = R"(

Moves `n` samples forward.

Example:

audio.dur(2 * rate, audio.add {
    audio.sine(440),
    audio.seek(rate, audio.sine(880)),
})
)"sv;

constexpr auto seq_str =
    R"(audio.seq(dur, {gen0, "o o ", gen1, " o o", ...})"sv;
constexpr auto seq_desc = R"(

Graphical (ASCII) sequencer.  Each group of two items in the sequence denote a
generator and the sequence of notes/rests.  For the latter, a space denotes a
rest and any other character indicates a note, both with duration `dur`.


Example:

audio.seq(rate, {
    audio.sine(440),   "o   o   ",
    audio.square(440), "  o   o ",
    audio.saw(440),    "oooooooo",
}
)"sv;

constexpr auto sine_str = "audio.sine(freq)"sv;
constexpr auto sine_desc = R"(

Produces a sine wave.

Example:

audio.sine(440)
)"sv;

constexpr auto sine_fm_str = "audio.sine_fm(freq, lfo_a, lfo_freq, lfo_d)"sv;
constexpr auto sine_fm_desc = R"(

Produces a sine wave with frequency modulation using another sine wave as a LFO.
The `lfo_*` parameters control the oscillator: `a` is the amplitude, `freq` is
the frequency, and `d` is the displacement.

Example:

audio.sine_fm(440, 1.5, 0.5, 2)
)"sv;

constexpr auto slide_str = "audio.slide(freq0, freq1, steps)"sv;
constexpr auto slide_desc = R"(

Example:

Divides the buffer into `steps` parts and applies the generator to each, using a
frequency linearly interpolated between `[freq0, freq1)` for each part.
)"sv;

constexpr auto square_str = "audio.square(freq)"sv;
constexpr auto square_desc = R"(

Produces a square wave.

audio.square(440)
)"sv;

constexpr auto trem_str = "audio.trem(a, f, mix)"sv;
constexpr auto trem_desc = R"(

Tremolo effect which applies a simple sine wave oscillator to the signal.  `a`
is the amplitude, `f` is the frequency.  `mix` is a linear interpolation factor
between the origin signal (0) and the one with the tremolo (1).

Example:

audio.trem(0.5, 3, 0.15, audio.sine(440))
)"sv;

}

namespace nngn {

Components::Components(QWidget *parent, Qt::WindowFlags flags)
    : QWidget{parent, flags}
{
    constexpr int icon_size = 64;
    const auto font = [] {
        QFont f = {};
        f.setFamily("monospace");
        f.setStyleHint(QFont::Monospace);
        f.setFixedPitch(true);
        return f;
    }();
    auto add = [
        this, &font,
        icon = QString{"tools/audio/img/%1.png"},
        size = icon_size + 2 * QFontMetrics{font}.height(),
        l = new FlowGridLayout{this}
    ](auto name, auto code, auto desc) mutable {
        auto *const b = new ComponentButton{
            icon_size, name, nngn::qstring_from_view(code),
            nngn::qstring_from_view(desc), icon.arg(name),
        };
        b->setFont(font);
        b->setMinimumSize(size, size);
        b->setMaximumSize(size, size);
        l->addWidget(b);
        QObject::connect(
            b, &QToolButton::clicked,
            [this, b] { emit this->clicked(b->code()); });
    };
    add("add", add_str, add_desc);
    add("cat", cat_str, cat_desc);
    add("chord", chord_str, chord_desc);
    add("dur", dur_str, dur_desc);
    add("env", env_str, env_desc);
    add("exp_fade", exp_fade_str, exp_fade_desc);
    add("fade", fade_str, fade_desc);
    add("fn", fn_str, fn_desc);
    add("gain", gain_str, gain_desc);
    add("harm", harm_str, harm_desc);
    add("map", map_str, map_desc);
    add("map2", map2_str, map2_desc);
    add("mix", mix_str, mix_desc);
    add("noise", noise_str, noise_desc);
    add("nop", nop_str, nop_desc);
    add("over", over_str, over_desc);
    add("rep", rep_str, rep_desc);
    add("saw", saw_str, saw_desc);
    add("seek", seek_str, seek_desc);
    add("seq", seq_str, seq_desc);
    add("sine", sine_str, sine_desc);
    add("sine_fm", sine_fm_str, sine_fm_desc);
    add("slide", slide_str, slide_desc);
    add("square", square_str, square_desc);
    add("trem", trem_str, trem_desc);
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
}

}
