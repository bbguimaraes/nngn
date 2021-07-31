#include "gen.h"

#include "audio/wav.h"
#include "lua/function.h"
#include "lua/traceback.h"
#include "lua/value.h"
#include "lua/utils.h"
#include "utils/string.h"

namespace nngn {

bool Generator::init(std::size_t rate) {
    NNGN_LOG_CONTEXT_CF(Generator);
    if(static_cast<lua_State*>(this->L))
        return true;
    if(!this->L.init())
        return false;
    nngn::lua::static_register::register_all(L);
    this->m.init();
    if(!this->a.init(&this->m, rate))
        return false;
    [[maybe_unused]] const auto mark = nngn::lua::stack_mark(this->L);
    const auto gt = nngn::lua::global_table(this->L);
    const auto msgh = this->push_msgh();
    const nngn::lua::value require = gt["require"];
    bool ok = this->L.pcall(msgh, require.push(), "src/lua/path") == LUA_OK;
    lua_pop(this->L, 2);
    if(!ok)
        return false;
    ok = this->L.pcall(msgh, require.push(), "nngn.lib.audio") == LUA_OK;
    lua_pop(this->L, 1);
    if(!ok)
        return false;
    gt["audio"] = nngn::lua::stack_arg{this->L, this->L.top()};
    gt["rate"] = static_cast<lua_Integer>(rate);
    return true;
}

bool Generator::set_loop(bool b) {
    this->flags.set(Flag::LOOP, b);
    return !this->source || this->a.set_source_loop(this->source, b);
}

bool Generator::set_gain(float g) {
    this->gain = g;
    if(!this->source)
        return false;
    this->generate();
    this->play();
    return true;
}

bool Generator::exec(const QString &prog, float param) {
    NNGN_LOG_CONTEXT_CF(Generator);
    using nngn::lua::stack_arg;
    [[maybe_unused]] const auto mark = nngn::lua::stack_mark(this->L);
    const auto gt = nngn::lua::global_table{this->L};
    gt["param"] = param;
    const auto msgh = this->push_msgh();
    nngn::lua::value gen = gt["audio"]["gen"];
    nngn::lua::value ap = push(this->L, nngn::lua::light_user_data{&this->a});
    push(this->L, gt["Audio"]);
    lua_setmetatable(this->L, -2);
    nngn::lua::value arg = {this->L, this->L.top() + 1};
    if(luaL_loadstring(this->L, prog.toStdString().c_str()) != LUA_OK) {
        this->m_error = lua_tostring(this->L, -1);
        return false;
    }
    if(this->L.pcall(msgh, stack_arg{arg}) != LUA_OK)
        return false;
    if(this->L.pcall(
        msgh, stack_arg{gen.release()},
        stack_arg{ap.release()}, stack_arg{arg.release()}
    ) != LUA_OK) {
        lua_pop(this->L, 1);
        return false;
    }
    auto *const p = lua_touserdata(this->L, -2);
    this->pre_gain = std::move(**static_cast<std::vector<std::byte>**>(p));
    lua_pop(this->L, 2);
    return true;
}

nngn::lua::value Generator::push_msgh(void) {
    using U = nngn::lua::light_user_data<Generator*>;
    constexpr auto f = [](lua_State *L_) {
        sol::stack::get<U>(L_, lua_upvalueindex(1))->m_error =
            nngn::fmt(lua_tostring(L_, -1), "\n\n", nngn::lua::traceback(L_));
        return 0;
    };
    return push(this->L, nngn::lua::c_closure{f, U{this}});
}

bool Generator::generate(void) {
    constexpr auto h = nngn::WAV::HEADER_SIZE;
    static_assert(!(h % sizeof(i16)));
    auto tmp = this->pre_gain;
    const auto tmp_s = nngn::byte_span_cast<float>(std::span{tmp});
    const auto n = tmp_s.size();
    this->wav_data.resize(h + n * sizeof(i16));
    const auto wav = nngn::WAV{this->wav_data};
    wav.set_size(static_cast<u32>(n * sizeof(i16)));
    wav.set_channels(1);
    wav.set_rate(static_cast<u32>(this->a.rate()));
    wav.fill();
    a.gain(tmp_s, this->gain);
    a.normalize(nngn::byte_span_cast<i16>(wav.data()), tmp_s);
    return this->play();
}

bool Generator::play(void) {
    std::size_t last_pos = {};
    if(this->source) {
        if(!this->flags.is_set(Flag::REWIND))
            last_pos = this->a.source_sample_pos(this->source);
        this->a.remove_source(this->source);
    }
    const auto wav = nngn::WAV{this->wav_data};
    if(!(this->source = this->a.add_source(wav.data())))
        return false;
    if(last_pos && last_pos <= wav.n_samples())
        if(!this->a.set_source_sample_pos(this->source, last_pos))
            return false;
    if(this->flags.is_set(Flag::LOOP))
        if(!this->a.set_source_loop(this->source, true))
            return false;
    this->a.play(this->source);
    return true;
}

}
