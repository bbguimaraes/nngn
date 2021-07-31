#include "audio.h"

#include "utils/log.h"

#include "wav.h"

namespace nngn {

bool Audio::write_wav(FILE *f, std::span<const std::byte> s) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    std::array<std::byte, WAV::HEADER_SIZE> h = {};
    this->gen_wav_header(h, s);
    fwrite(h.data(), 1, h.size(), f);
    if(ferror(f))
        return Log::perror(), false;
    fwrite(s.data(), 1, s.size(), f);
    if(ferror(f))
        return Log::perror(), false;
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

bool Audio::init(nngn::Math *m, std::size_t rate) {
    NNGN_LOG_CONTEXT_CF(Audio);
    this->math = m;
    this->m_rate = rate;
    return this->init_openal();
}

}
