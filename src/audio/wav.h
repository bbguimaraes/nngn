#ifndef NNGN_AUDIO_WAV_H
#define NNGN_AUDIO_WAV_H

#include <bit>
#include <cassert>
#include <climits>
#include <cstring>
#include <span>
#include <string_view>

#include "utils/def.h"
#include "utils/literals.h"
#include "utils/utils.h"

namespace nngn {

using namespace std::string_view_literals;
using namespace nngn::literals;

/** A non-owning wrapper for a byte buffer containing a WAV file. */
class WAV {
    static_assert(std::endian::native == std::endian::little, "TODO");
public:
    static constexpr std::size_t HEADER_SIZE = 44;
    /** Constructs an empty object, must be initialized before it is used. */
    WAV() = default;
    constexpr explicit WAV(std::span<std::byte> buffer) : b{buffer} {};
    constexpr u32 fmt_size(void) const;
    constexpr u16 format(void) const;
    constexpr u16 channels(void) const;
    constexpr u32 rate(void) const;
    constexpr u16 bits_per_sample(void) const;
    constexpr std::size_t n_samples(void) const;
    constexpr void set_size(u32 s) const;
    constexpr void set_channels(u16 c) const;
    constexpr void set_rate(u32 r) const;
    constexpr void fill(void) const;
    constexpr std::span<std::byte> data(void) const;
    bool check(void) const;
private:
    enum class offset : std::size_t {
        RIFF_SEG = 0,
        SEG_SIZE = 4,
        WAVE_SEG = 8,
        FMT_SEG = 12,
        FMT_SIZE = 16,
        FORMAT = 20,
        CHANNELS = 22,
        RATE = 24,
        BYTES_PER_SEC = 28,
        BLOCK_ALIGN = 32,
        BITS_PER_SAMPLE = 34,
        DATA_SEG = 36,
        DATA_SIZE = 40,
        DATA = HEADER_SIZE,
    };
    struct segment {
        static constexpr auto RIFF = "RIFF"sv;
        static constexpr auto WAVE = "WAVE"sv;
        static constexpr auto FMT = "fmt "sv;
        static constexpr auto DATA = "data"sv;
    };
    constexpr std::span<std::byte> subspan(offset o, std::size_t n) const;
    template<typename T> constexpr T read(offset o) const;
    template<typename T> constexpr void write(offset o, const T &x) const;
    constexpr std::size_t size(void) const;
    std::span<std::byte> b = {};
};

inline constexpr std::span<std::byte> WAV::subspan(
    offset o, std::size_t n
) const {
    const auto oz = static_cast<std::size_t>(o);
    assert(oz + n <= this->b.size());
    return this->b.subspan(oz, n);
}

template<typename T>
constexpr T WAV::read(offset o) const {
    T ret = {};
    std::memcpy(&ret, this->subspan(o, sizeof(ret)).data(), sizeof(ret));
    return ret;
}

template<typename T>
constexpr void WAV::write(offset o, const T &x) const {
    if constexpr(std::ranges::range<T>) {
        auto s = this->subspan(o, x.size());
        std::memcpy(s.data(), x.data(), x.size());
    } else {
        auto s = this->subspan(o, sizeof(T));
        std::memcpy(s.data(), &x, sizeof(T));
    }
}

inline constexpr u32 WAV::fmt_size(void) const {
    return this->read<u32>(offset::FMT_SIZE);
}

inline constexpr u16 WAV::format(void) const {
    return this->read<u16>(offset::FORMAT);
}

inline constexpr u16 WAV::channels(void) const {
    return this->read<u16>(offset::CHANNELS);
}

inline constexpr u32 WAV::rate(void) const {
    return this->read<u32>(offset::RATE);
}

inline constexpr u16 WAV::bits_per_sample(void) const {
    return this->read<u16>(offset::BITS_PER_SAMPLE);
}

inline constexpr std::size_t WAV::n_samples(void) const {
    return this->size() * CHAR_BIT / this->bits_per_sample();
}

inline constexpr std::size_t WAV::size(void) const {
    return this->read<u32>(offset::DATA_SIZE);
}

inline constexpr std::span<std::byte> WAV::data(void) const {
    return this->subspan(offset::DATA, this->size());
}

inline constexpr void WAV::set_size(u32 s) const {
    this->write(offset::SEG_SIZE, s + 40);
    this->write(offset::DATA_SIZE, s);
}

inline constexpr void WAV::set_channels(u16 c) const {
    this->write(offset::CHANNELS, c);
}

inline constexpr void WAV::set_rate(u32 r) const {
    this->write(offset::RATE, r);
}

inline constexpr void WAV::fill(void) const {
    constexpr u16 bits_per_sample = 16;
    constexpr std::size_t bytes_per_sample = bits_per_sample / 8;
    const auto r = this->rate();
    const auto c = this->channels();
    this->write(offset::RIFF_SEG, segment::RIFF);
    this->write(offset::WAVE_SEG, segment::WAVE);
    this->write(offset::FMT_SEG, segment::FMT);
    this->write(offset::FMT_SIZE, 16_u32);
    this->write(offset::FORMAT, 1_u16);
    this->write(
        offset::BYTES_PER_SEC,
        static_cast<u32>(r * bytes_per_sample * c));
    this->write(offset::BLOCK_ALIGN, static_cast<u16>(bytes_per_sample * c));
    this->write(offset::BITS_PER_SAMPLE, bits_per_sample);
    this->write(offset::DATA_SEG, segment::DATA);
}

}

#endif
