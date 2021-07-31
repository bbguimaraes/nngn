#include "wav.h"

#include <algorithm>
#include <array>
#include <string_view>

#include "utils/literals.h"
#include "utils/log.h"

using namespace std::string_view_literals;
using namespace nngn::literals;

namespace nngn {

bool WAV::check(void) const {
    NNGN_LOG_CONTEXT_CF(WAV);
    const auto check_seg = [this](
        const char *name, offset o, std::string_view cmp
    ) {
        const auto s = this->subspan(o, cmp.size());
        const auto sc = std::span{
            static_cast<const char*>(static_cast<const void*>(s.data())),
            s.size(),
        };
        if(std::ranges::equal(sc, cmp))
            return true;
        auto &l = Log::l();
        l << "invalid " << name << " segment ID: ";
        std::ranges::copy(sc, std::ostream_iterator<char>{l});
        l << " != ";
        std::ranges::copy(cmp, std::ostream_iterator<char>{l});
        l << '\n';
        return false;
    };
    if(const auto n = this->b.size(), e = 44_z; n < e) {
        Log::l() << "header too short (" << n << "< " << e << ")\n";
        return false;
    }
    if(!(
        check_seg("riff", offset::RIFF_SEG, segment::RIFF)
        && check_seg("wave", offset::WAVE_SEG, segment::WAVE)
        && check_seg("format", offset::FMT_SEG, segment::FMT)
        && check_seg("data", offset::DATA_SEG, segment::DATA)
    ))
        return false;
    const auto size = this->b.size();
    constexpr auto seg_off = static_cast<u32>(offset::SEG_SIZE);
    if(const auto n = this->read<u32>(offset::SEG_SIZE); size < n + seg_off) {
        Log::l()
            << "chunk size larger than total size: "
            << size << " - " << seg_off << " < " << n << '\n';
        return false;
    }
    if(const auto n = this->fmt_size(); n != 16 && n != 18 && n != 40) {
        Log::l() << "invalid format segment size: " << n << " != 16/18/40\n";
        return false;
    }
    if(const auto f = this->format(), e = 1_u16; f != e) {
        Log::l() << "invalid format: " << f << " != " << e << " (PCM)\n";
        return false;
    }
    if(const auto c = this->channels(), e = 1_u16; c != e) {
        Log::l() << "invalid number of channels: " << c << " != " << e << '\n';
        return false;
    }
    if(const auto r = this->rate(), e = 44100_u32; r != e) {
        Log::l() << "invalid sample rate: " << r << " != " << e << '\n';
        return false;
    }
    if(const auto n = this->bits_per_sample(), e = 16_u16; n != e) {
        Log::l() << "invalid bits per sample: " << n << " != " << e << '\n';
        return false;
    }
    if(const auto n = this->size(); size < n) {
        Log::l()
            << "data size in header larger than file size: "
            << size << " < " << n << '\n';
        return false;
    }
    return true;
}

}
