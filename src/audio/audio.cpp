#include "audio.h"

#include "utils/log.h"

#include "wav.h"

namespace nngn {

bool Audio::write_wav(FILE *f, std::span<const std::byte> s) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    std::array<std::byte, WAV::HEADER_SIZE> h = {};
    this->gen_wav_header(h, s);
    if(fwrite(h.data(), 1, h.size(), f) != h.size())
        return Log::perror("fwrite"), false;
    if(fwrite(s.data(), 1, s.size(), f) != s.size())
        return Log::perror("fwrite"), false;
    return true;
}

void Audio::gen_wav_header(
    std::span<std::byte> dst, std::span<const std::byte> src
) const {
    const auto wav = WAV{std::span{dst}};
    wav.set_size(static_cast<u32>(src.size()));
    wav.set_channels(1);
    wav.set_rate(static_cast<u32>(this->m_rate));
    wav.fill();
}

std::vector<std::byte> Audio::gen_wav(std::span<const float> s) const {
    constexpr auto h = WAV::HEADER_SIZE;
    auto ret = std::vector<std::byte>(h + s.size() * sizeof(i16));
    this->gen_wav_header(ret, std::span{ret}.subspan(h));
    Audio::normalize(nngn::byte_cast<i16>(WAV{ret}.data()), s);
    return ret;
}

bool Audio::init(nngn::Math *m, std::size_t rate) {
    NNGN_LOG_CONTEXT_CF(Audio);
    this->math = m;
    this->m_rate = rate;
    return this->init_openal();
}

}
