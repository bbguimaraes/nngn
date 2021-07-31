#ifndef NNGN_TOOLS_AUDIO_GEN_H
#define NNGN_TOOLS_AUDIO_GEN_H

#include <QString>

#include "audio/audio.h"
#include "lua/state.h"
#include "utils/flags.h"

namespace nngn {

class Generator {
public:
    bool init(std::size_t rate);
    bool set_loop(bool b);
    void set_rewind(bool b) { this->flags.set(Flag::REWIND, b); }
    bool set_gain(float g);
    auto release_data(void) { return std::move(this->wav_data); }
    std::string_view error(void) const { return this->m_error; }
    bool exec(const QString &prog, float param);
    bool generate(void);
    bool stop(void) { return this->a.stop(this->source); }
private:
    enum Flag : u8 {
        LOOP = 1 << 0,
        REWIND = 1 << 1,
    };
    nngn::lua::value push_msgh(void);
    bool play(void);
    nngn::lua::state L = {};
    nngn::Math m = {};
    nngn::Audio a = {};
    std::size_t source = {};
    float gain = 1;
    std::vector<std::byte> pre_gain = {}, wav_data = {};
    std::string m_error = {};
    nngn::Flags<Flag> flags = {};
};

}

#endif
