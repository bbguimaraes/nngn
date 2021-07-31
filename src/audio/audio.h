#ifndef NNGN_AUDIO_AUDIO_H
#define NNGN_AUDIO_AUDIO_H

#include <cstddef>
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
    // Utility
    static constexpr float db(float x) { return std::pow(10.0f, x / 20.0f); }
    // Audio generation
    /** Applies the gain multiplier to each element. */
    static void gain(std::span<float> s, float g);
    /** Linearly fades from \c g0 to \c g1. */
    static void fade(std::span<float> s, float g0, float g1);
    /**
     * Exponentially fades from \c g0 to \c g1.
     * The interpolation is done in reverse (i.e. <tt>1 - ((1 - x) ^ e)</tt> in
     * case <tt>g1 < g0</tt> so that the same exponent generates similar curves
     * when fading either in or out.  \c ep is the real end position, which
     * should be <tt>>= s.size()</tt>.
     */
    static void exp_fade(
        std::span<float> s, std::size_t ep, float g0, float g1, float e);
    /** Attack/decay/sustain/release envelope. */
    static void env(
        std::span<float> s,
        std::size_t a, std::size_t d, float st, std::size_t r);
    /** Adds the samples from both buffers. */
    static void mix(std::span<float> dst, std::span<const float> src);
    /** Generates 16-bit PCM data from floating-point data. */
    static void normalize(std::span<i16> dst, std::span<const float> src);
    ~Audio(void);
    /**
     * Initializes the manager to work with a given sample rate.
     * Must be called befone any other non-static member function.
     * \param m
     *     Used to generate random noise, may be \c null if \c noise is never
     *     called.
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
    // Audio generation
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
    void gen_square(std::span<float> s, float freq) const;
    void gen_saw(std::span<float> s, float freq) const;
    void gen_noise(std::span<float> s) const;
    // Source management
    bool set_pos(vec3 p);
    std::size_t add_source(std::span<const std::byte> v);
    bool remove_source(std::size_t source);
    vec3 source_pos(std::size_t source) const;
    std::size_t source_sample_pos(std::size_t source) const;
    bool set_source_pos(std::size_t source, vec3 p);
    bool set_source_sample_pos(std::size_t source, std::size_t p);
    bool set_source_loop(std::size_t source, bool l);
    bool play(std::size_t source) const;
    bool stop(std::size_t source) const;
private:
    bool init_openal(void);
    void *data = nullptr;
    Math *math = nullptr;
    std::size_t m_rate = 0;
};

}

#endif
