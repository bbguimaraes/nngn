#ifndef NNGN_AUDIO_AUDIO_H
#define NNGN_AUDIO_AUDIO_H

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

#include "math/math.h"
#include "math/vec3.h"
#include "utils/def.h"

namespace nngn {

/** Audio manager.  Generates, stores, and controls audio streams. */
class Audio {
public:
    enum class source : std::size_t {};
    // Utility
    /** Multiplier to decibel conversion. */
    static constexpr float db(float x) { return std::pow(10.0f, x / 20.0f); }
    // Audio generation
    /** Applies the gain multiplier to each element. */
    static void gain(std::span<float> s, float g);
    /** Applies an overdrive effect with input multiplier \p m. */
    static void over(std::span<float> s, float m, float mix);
    /** Linearly fades from \p g0 to \p g1. */
    static void fade(std::span<float> s, float g0, float g1);
    /**
     * Exponentially fades from \p g0 to \p g1.
     * The interpolation is done in reverse (i.e. <tt>1 - ((1 - x) ^ e)</tt> in
     * case <tt>g1 < g0</tt> so that the same exponent generates similar curves
     * when fading either in or out.  \p ep is the real end position, which has
     * to satisfy <tt>s.size() <= ep</tt>.
     */
    static void exp_fade(
        std::span<float> s, std::size_t ep, float g0, float g1, float exp);
    /** Attack/decay/sustain/release envelope. */
    static void env(
        std::span<float> s,
        std::size_t a, std::size_t d, float st, std::size_t r);
    /** Adds the samples from both buffers. */
    static void mix(std::span<float> dst, std::span<const float> src);
    /** Generates 16-bit PCM data from floating-point data. */
    static void normalize(std::span<i16> dst, std::span<const float> src);
    // Constructors, destructors
    NNGN_MOVE_ONLY(Audio)
    Audio(void) = default;
    ~Audio(void);
    // Initialization, accessors
    /**
     * Initializes the manager to work with a given sample rate.
     * Must be called before any other non-static member function.
     * \param m May be \c null if \ref noise is never called.
     */
    bool init(Math *m, std::size_t rate);
    std::size_t rate(void) const { return this->m_rate; }
    std::size_t n_sources(void) const;
    // I/O
    /** Reads WAV data from a file. */
    bool read_wav(std::string_view path, std::vector<std::byte> *v) const;
    /** Writes WAV data to a file. */
    bool write_wav(FILE *f, std::span<const std::byte> s) const;
    /** Generates WAV header for a buffer. */
    void gen_wav_header(
        std::span<std::byte> dst, std::span<const std::byte> src) const;
    /** Generates a WAV file from floating-point data. */
    std::vector<std::byte> gen_wav(std::span<const float> s) const;
    // Audio generation
    /** Applies a tremolo effect with amplitude \p a and frequency \p f. */
    void trem(std::span<float> s, float a, float freq, float mix) const;
    /** Generates a sine wave with amplitude \c 1 and frequency \p freq. */
    void gen_sine(std::span<float> s, float freq) const;
    /**
     * Generates a sine wave with frequency modulation.
     * \param lfo_a Amplitude of the message signal.
     * \param lfo_freq Frequency of the message signal.
     * \param lfo_d
     *     Phase delta (i.e. angular displacement, in radians) to apply to the
     *     message signal.
     */
    void gen_sine_fm(
        std::span<float> s, float freq,
        float lfo_a, float lfo_freq, float lfo_d) const;
    /** Generates a square wave with amplitude \c 1 and frequency \p freq. */
    void gen_square(std::span<float> s, float freq) const;
    /** Generates a saw-tooth wave with amplitude \c 1 and frequency \p freq. */
    void gen_saw(std::span<float> s, float freq) const;
    /** Generates white (random) noise. */
    void gen_noise(std::span<float> s) const;
    // Sources
    bool set_pos(vec3 p);
    source add_source(void);
    source add_source(std::span<const std::byte> v);
    bool remove_source(source s);
    vec3 source_pos(source s) const;
    std::size_t source_sample_pos(source s) const;
    bool set_source_data(
        source s, std::size_t channels, std::size_t bit_depth,
        std::span<const std::byte> v);
    bool set_source_pos(source s, vec3 p);
    bool set_source_sample_pos(source s, std::size_t p);
    bool set_source_loop(source s, bool l);
    bool set_source_gain(source s, float g);
    bool play(source s) const;
    bool stop(source s) const;
private:
    bool init_openal(void);
    std::unique_ptr<void, void(*)(void*)> data = {nullptr, [](auto){}};
    Math *math = nullptr;
    std::size_t m_rate = 0;
};

}

#endif
