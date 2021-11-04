#include "audio.h"

#include "os/platform.h"
#include "utils/log.h"

#ifndef NNGN_PLATFORM_HAS_OPENAL
namespace nngn {

bool Audio::init_openal(void) {
    NNGN_LOG_CONTEXT_CF(Audio);
    Log::l() << "compiled without audio support\n";
    return true;
}

bool Audio::read_wav(std::string_view, std::vector<std::byte>*) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    Log::l() << "compiled without audio support\n";
    return true;
}

Audio::~Audio(void) {}
std::size_t Audio::n_sources(void) const { return 0; }
bool Audio::set_pos(vec3) { return true; }
auto Audio::add_source(void) -> source { return {}; }
auto Audio::add_source(std::span<const std::byte>) -> source { return {}; }
bool Audio::remove_source(source) { return true; }
vec3 Audio::source_pos(source) const { return {}; }
std::size_t Audio::source_sample_pos(source) const { return 0; }
bool Audio::set_source_pos(source, vec3) { return true; }
bool Audio::set_source_sample_pos(source, std::size_t) { return true; }
bool Audio::set_source_loop(source, bool) { return true; }
bool Audio::set_source_gain(source, float) { return true; }
bool Audio::play(source) const { return true; }
bool Audio::stop(source) const { return true; }

bool Audio::set_source_data(
    source, std::size_t, std::size_t, std::span<const std::byte>)
{
    return true;
}

}
#else
#include <climits>
#include <iostream>
#include <thread>

#include <AL/al.h>
#include <AL/alc.h>

#include "wav.h"

#define LOG_RESULT(f, ...) (f(__VA_ARGS__), check_openal(#f))
#define LOG_CTX_RESULT(f, dev, ...) (f(__VA_ARGS__), check_openal_ctx(dev, #f))
#define CHECK_RESULT(f, ...) \
    do { if(!LOG_RESULT(f, __VA_ARGS__)) return {}; } while(0);
#define CHECK_CTX_RESULT(f, dev, ...) \
    do { if(!LOG_CTX_RESULT(f, dev, __VA_ARGS__)) return false; } while(0);

static_assert(AL_FORMAT_MONO8 + 1 == AL_FORMAT_MONO16);
static_assert(AL_FORMAT_STEREO8 + 1 == AL_FORMAT_STEREO16);

namespace {

struct Source {
    static_assert(sizeof(ALuint) <= sizeof(std::uintptr_t));
    std::uintptr_t id;
    std::size_t n;
    ALuint buffer;
    constexpr ALuint al_id(void) const { return static_cast<ALuint>(this->id); }
};

struct Data {
    ALCdevice *dev = nullptr;
    ALCcontext *ctx = nullptr;
    std::vector<ALuint> buffers = {};
    std::vector<Source> sources = {};
};

bool check_openal(const char *msg) {
    NNGN_LOG_CONTEXT_F();
    const auto e = alGetError();
    switch(e) {
    case AL_NO_ERROR: return true;
#define C(x) case x: nngn::Log::l() << msg << ": " #x "\n"; break;
    C(AL_INVALID_NAME)
    C(AL_INVALID_ENUM)
    C(AL_INVALID_VALUE)
    C(AL_INVALID_OPERATION)
    C(AL_OUT_OF_MEMORY)
    default: nngn::Log::l() << msg << ": unknown error\n"; break;
    }
#undef C
    return false;
}

bool check_openal_ctx(ALCdevice *dev, const char *msg) {
    NNGN_LOG_CONTEXT_F();
    const auto e = alcGetError(dev);
    switch(e) {
    case ALC_NO_ERROR: return true;
#define C(x) case x: nngn::Log::l() << msg << ": " #x "\n"; return false;
    C(ALC_INVALID_DEVICE)
    C(ALC_INVALID_CONTEXT)
    C(ALC_INVALID_ENUM)
    C(ALC_INVALID_VALUE)
    C(ALC_OUT_OF_MEMORY)
    default: nngn::Log::l() << msg << ": unknown error\n"; break;
    }
#undef C
    return false;
}

ALuint source_id(const auto &d, nngn::Audio::source i) {
    return static_cast<Data*>(d.get())
        ->sources[static_cast<std::size_t>(i)].al_id();
}

bool delete_source(const Source &s) {
    const auto id = s.al_id();
    bool ret = true;
    ret = LOG_RESULT(alSourceStop, id) && ret;
    ret = LOG_RESULT(alDeleteSources, 1, &id) && ret;
    return ret;
}

}

namespace nngn {

Audio::~Audio(void) {
    NNGN_LOG_CONTEXT_CF(Audio);
    if(!this->data)
        return;
    auto *const d = static_cast<Data*>(this->data.get());
    for(const auto &x : d->sources)
        if(const auto id = x.al_id())
            delete_source(x);
    if(!d->buffers.empty())
        LOG_RESULT(alDeleteBuffers,
            static_cast<int>(d->buffers.size()), d->buffers.data());
    LOG_CTX_RESULT(alcMakeContextCurrent, d->dev, nullptr);
    LOG_CTX_RESULT(alcDestroyContext, d->dev, d->ctx);
    LOG_CTX_RESULT(alcCloseDevice, d->dev, d->dev);
}

std::size_t Audio::n_sources(void) const {
    return static_cast<Data*>(this->data.get())->sources.size();
}

bool Audio::init_openal(void) {
    ALCdevice *const dev = alcOpenDevice(nullptr);
    if(!dev)
        return false;
    ALCcontext *const ctx = alcCreateContext(dev, NULL);
    if(!ctx)
        return check_openal_ctx(dev, "alcCreateContext"), false;
    CHECK_CTX_RESULT(alcMakeContextCurrent, dev, ctx);
    CHECK_RESULT(alListenerf, AL_GAIN, 10.0f / 6.0f);
    auto *const d = new Data{.dev = dev, .ctx = ctx};
    this->data = {d, [](void *p) { delete static_cast<Data*>(p); }};
    d->sources.emplace_back();
    return true;
}

bool Audio::read_wav(std::string_view path, std::vector<std::byte> *v) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    NNGN_LOG_CONTEXT(path.data());
    if(!read_file(path, v))
        return false;
    const auto wav = WAV{*v};
    if(!wav.check())
        return false;
    if(const auto r = wav.rate(); r != this->m_rate) {
        Log::l()
            << "file sample rate (" << r << ") does not match expected rate ("
            << this->m_rate << ")\n";
        return false;
    }
    return true;
}

bool Audio::set_pos(vec3 p) {
    NNGN_LOG_CONTEXT_CF(Audio);
    (void)this;
    return LOG_RESULT(alListenerfv, AL_POSITION, &p.x);
}

auto Audio::add_source(void) -> source {
    NNGN_LOG_CONTEXT_CF(Audio);
    auto *const d = static_cast<Data*>(this->data.get());
    ALuint id = 0;
    CHECK_RESULT(alGenSources, 1, &id);
    auto &sources = d->sources;
    return narrow<source>(
        &sources.emplace_back(Source{.id = id}) - &sources.front());
}

auto Audio::add_source(std::span<const std::byte> v) -> source {
    NNGN_LOG_CONTEXT_CF(Audio);
    const auto ret = this->add_source();
    if(static_cast<bool>(ret))
        this->set_source_data(ret, 1, 16, v);
    return ret;
}

bool Audio::remove_source(source i) {
    NNGN_LOG_CONTEXT_CF(Audio);
    auto *const d = static_cast<Data*>(this->data.get());
    auto it = d->sources.begin() + static_cast<std::ptrdiff_t>(i);
    bool ret = true;
    ret = delete_source(*it);
    ret = LOG_RESULT(alDeleteBuffers, 1, &it->buffer) && ret;
    d->sources.erase(it);
    d->buffers[static_cast<std::size_t>(i)] = 0;
    return ret;
}

vec3 Audio::source_pos(source i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    vec3 ret = {};
    CHECK_RESULT(alGetSourcefv,
        source_id(this->data, i), AL_POSITION, ret.data());
    return ret;
}

std::size_t Audio::source_sample_pos(source i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    ALint ret = 0;
    CHECK_RESULT(alGetSourcei,
        source_id(this->data, i), AL_SAMPLE_OFFSET, &ret);
    return static_cast<std::size_t>(ret);
}

bool Audio::set_source_data(
    source s, std::size_t channels, std::size_t bit_depth,
    std::span<const std::byte> v)
{
    NNGN_LOG_CONTEXT_CF(Audio);
    auto *const d = static_cast<Data*>(this->data.get());
    ALuint buffer = 0;
    CHECK_RESULT(alGenBuffers, 1, &buffer);
    ALenum format = AL_INVALID_ENUM;
    switch(channels) {
    case 1: format = AL_FORMAT_MONO8; break;
    case 2: format = AL_FORMAT_STEREO8; break;
    default:
        Log::l() << "invalid number of channels: " << channels << '\n';
        return false;
    }
    switch(bit_depth) {
    case 8: break;
    case 16: ++format; break;
    default:
        Log::l() << "invalid bit depth: " << bit_depth << '\n';
        return false;
    }
    CHECK_RESULT(alBufferData,
        buffer, format, v.data(), static_cast<int>(v.size()),
        static_cast<int>(this->m_rate));
    const auto i = static_cast<std::size_t>(s);
    auto &sources = d->sources;
    auto &buffers = d->buffers;
    if(buffers.size() <= i)
        buffers.resize(sources.size());
    buffers[i] = buffer;
    CHECK_RESULT(alSourcei,
        sources[i].al_id(), AL_BUFFER, static_cast<ALint>(buffer));
    return true;
}

bool Audio::set_source_pos(source i, vec3 p) {
    NNGN_LOG_CONTEXT_CF(Audio);
    return LOG_RESULT(alSourcefv,
        source_id(this->data, i), AL_POSITION, p.data());
}

bool Audio::set_source_sample_pos(source i, std::size_t p) {
    NNGN_LOG_CONTEXT_CF(Audio);
    return LOG_RESULT(alSourcei,
        source_id(this->data, i), AL_SAMPLE_OFFSET, narrow<int>(p));
}

bool Audio::set_source_loop(source i, bool l) {
    NNGN_LOG_CONTEXT_CF(Audio);
    return LOG_RESULT(alSourcei, source_id(this->data, i), AL_LOOPING, l);
}

bool Audio::set_source_gain(source i, float g) {
    NNGN_LOG_CONTEXT_CF(Audio);
    return LOG_RESULT(alSourcef, source_id(this->data, i), AL_GAIN, g);
}

bool Audio::play(source i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    return LOG_RESULT(alSourcePlay, source_id(this->data, i));
}

bool Audio::stop(source i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    return LOG_RESULT(alSourceStop, source_id(this->data, i));
}

}
#endif
