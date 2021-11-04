#include "gen.h"

#include "audio/wav.h"
#include "lua/register.h"
#include "lua/table.h"
#include "lua/traceback.h"
#include "lua/utils.h"
#include "math/lua_vector.h"
#include "utils/string.h"

using nngn::i16, nngn::u32;

NNGN_LUA_DECLARE_USER_TYPE(nngn::Audio, "Audio")

namespace nngn {

bool Generator::init(std::size_t rate) {
    NNGN_LOG_CONTEXT_CF(Generator);
    if(this->lua)
        return true;
    if(!this->lua.init())
        return false;
    NNGN_ANON_DECL(nngn::lua::stack_mark(this->lua));
    NNGN_ANON_DECL(nngn::lua::defer_settop(this->lua));
    nngn::lua::static_register::register_all(this->lua);
    this->math.init();
    if(!this->audio.init(&this->math, rate))
        return false;
    const auto gt = nngn::lua::global_table{this->lua};
    const auto msgh = this->push_msgh().release();
    const auto require = gt["require"].get<nngn::lua::value>().release();
    if(this->lua.pcall(msgh, require, "src/lua/path") != LUA_OK)
        return false;
    if(this->lua.pcall(msgh, require, "nngn.lib.audio") != LUA_OK)
        return false;
    gt["audio"] = this->lua.get(this->lua.top() - 1);
    gt["rate"] = nngn::narrow<lua_Integer>(rate);
    return true;
}

std::size_t Generator::pos(void) const {
    return this->has_source() ? this->audio.source_sample_pos(this->source) : 0;
}

bool Generator::set_loop(bool b) {
    this->flags.set(Flag::LOOP, b);
    return !this->has_source()
        || this->audio.set_source_loop(this->source, b);
}

bool Generator::set_mute(bool b) {
    this->flags.set(Flag::MUTE, b);
    return !this->has_source()
        || this->audio.set_source_gain(this->source, b ? 0 : this->gain);
}

bool Generator::set_gain(float g) {
    this->gain = g;
    return !this->has_source()
        || this->audio.set_source_gain(this->source, g);
}

bool Generator::generate(const QString &prog) {
    NNGN_LOG_CONTEXT_CF(Generator);
    NNGN_ANON_DECL(nngn::lua::stack_mark(this->lua));
    NNGN_ANON_DECL(nngn::lua::defer_settop(this->lua));
    const auto gt = nngn::lua::global_table{this->lua};
    gt["param"] = nngn::narrow<lua_Number>(this->param);
    const auto msgh = this->push_msgh().release();
    this->lua.push(gt["audio"]["gen"]).release();
    this->lua.push(&this->audio).release();
    if(luaL_loadstring(this->lua, prog.toStdString().c_str()) != LUA_OK) {
        this->m_error = lua_tostring(this->lua, -1);
        return false;
    }
    if(lua_pcall(this->lua, 0, 1, msgh.index()) != LUA_OK)
        return false;
    if(lua_pcall(this->lua, 2, 1, msgh.index()) != LUA_OK)
        return false;
    this->wav_data = this->audio.gen_wav(
        nngn::byte_cast<const float>(
            std::span{this->lua.get<nngn::lua_vector<std::byte>&>(-1)}));
    return this->play();
}

nngn::lua::value Generator::push_msgh(void) {
    this->lua.push(static_cast<void*>(this));
    lua_pushcclosure(this->lua, [](lua_State *L) {
        nngn::lua::state_view l = {L};
        chain_cast<Generator*, void*>(l.get(lua_upvalueindex(1)))->m_error =
            nngn::fmt(lua_tostring(l, -1), "\n\n", nngn::lua::traceback{l});
        return 0;
    }, 1);
    return this->lua.get(this->lua.top());
}

bool Generator::play(void) {
    std::size_t pos = {};
    if(this->has_source()) {
        if(!this->flags.is_set(Flag::REWIND))
            pos = this->audio.source_sample_pos(this->source);
        this->audio.remove_source(this->source);
    }
    const auto wav = nngn::WAV{this->wav_data};
    const auto s = this->source = this->audio.add_source(wav.data());
    return this->has_source()
        && this->audio.set_source_gain(s, this->gain)
        && this->audio.set_source_loop(s, this->flags.is_set(Flag::LOOP))
        && this->audio.set_source_sample_pos(s, std::min(pos, wav.n_samples()))
        && this->audio.play(s);
}

}
