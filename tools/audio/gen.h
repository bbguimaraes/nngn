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
    std::size_t pos(void) const;
    bool set_loop(bool b);
    void set_rewind(bool b) { this->flags.set(Flag::REWIND, b); }
    bool set_mute(bool b);
    bool set_gain(float g);
    void set_param(float p) { this->param = p; }
    auto release_data(void) { return std::move(this->wav_data); }
    std::string_view error(void) const { return this->m_error; }
    bool generate(const QString &prog);
    bool stop(void) { return this->audio.stop(this->source); }
private:
    enum Flag : u8 {
        LOOP = 1 << 0,
        REWIND = 1 << 1,
        MUTE = 1 << 2,
    };
    bool has_source(void) const { return static_cast<bool>(this->source); }
    nngn::lua::value push_msgh(void);
    bool play(void);
    nngn::lua::state lua = {};
    nngn::Math math = {};
    nngn::Audio audio = {};
    nngn::Audio::source source = {};
    float gain = 1, param = 0;
    std::vector<std::byte> wav_data = {};
    std::string m_error = {};
    nngn::Flags<Flag> flags = {};
};

}

#endif
