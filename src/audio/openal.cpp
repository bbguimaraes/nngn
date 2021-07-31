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
std::size_t Audio::add_source(std::span<const std::byte>) { return 0; }
bool Audio::remove_source(std::size_t) { return true; }
vec3 Audio::source_pos(std::size_t) const { return {}; }
std::size_t Audio::source_sample_pos(std::size_t) const { return 0; }
bool Audio::set_source_pos(std::size_t, vec3) { return true; }
bool Audio::set_source_sample_pos(std::size_t, std::size_t) { return true; }
bool Audio::set_source_loop(std::size_t, bool) { return true; }
bool Audio::play(std::size_t) const { return true; }
bool Audio::stop(std::size_t) const { return true; }

}
#else
#include <climits>
#include <iostream>
#include <thread>

#include <AL/al.h>
#include <AL/alc.h>

#include "utils/static_vector.h"

#include "wav.h"

#define CHECK_LOG(f, ...) (f(__VA_ARGS__), check_openal(#f))
#define CHECK_CTX_LOG(f, dev, ...) (f(__VA_ARGS__), check_openal_ctx(dev, #f))
#define CHECK(f, ...) \
    do { if(!CHECK_LOG(f, __VA_ARGS__)) return {}; } while(0);
#define CHECK_CTX(f, dev, ...) \
    do { if(!CHECK_CTX_LOG(f, dev, __VA_ARGS__)) return false; } while(0);

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
    nngn::static_vector<Source> sources = {};
};

static bool check_openal(const char *msg) {
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

static bool check_openal_ctx(ALCdevice *dev, const char *msg) {
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

}

namespace nngn {

Audio::~Audio(void) {
    NNGN_LOG_CONTEXT_CF(Audio);
    if(!this->data)
        return;
    auto *const d = static_cast<Data*>(this->data);
    for(const auto &x : d->sources)
        if(const auto id = x.al_id()) {
            CHECK_LOG(alSourceStop, id);
            CHECK_LOG(alDeleteSources, 1, &id);
        }
    if(!d->buffers.empty())
        CHECK_LOG(alDeleteBuffers,
            static_cast<int>(d->buffers.size()), d->buffers.data());
    CHECK_CTX_LOG(alcMakeContextCurrent, d->dev, nullptr);
    CHECK_CTX_LOG(alcDestroyContext, d->dev, d->ctx);
    if(!alcCloseDevice(d->dev))
        check_openal_ctx(d->dev, "alcCloseDevice");
    delete static_cast<Data*>(this->data);
}

std::size_t Audio::n_sources(void) const {
    return static_cast<Data*>(this->data)->sources.size();
}

bool Audio::init_openal(void) {
    ALCdevice *const dev = alcOpenDevice(nullptr);
    if(!dev)
        return false;
    ALCcontext *const ctx = alcCreateContext(dev, NULL);
    if(!ctx) {
        check_openal_ctx(dev, "alcCreateContext");
        return false;
    }
    CHECK_CTX(alcMakeContextCurrent, dev, ctx);
    CHECK(alListenerf, AL_GAIN, 10.0f / 6.0f);
    auto *const d = new Data{.dev = dev, .ctx = ctx};
    this->data = d;
    d->sources.set_capacity(1);
    d->sources.insert({});
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
    return CHECK_LOG(alListenerfv, AL_POSITION, &p.x);
}

std::size_t Audio::add_source(std::span<const std::byte> v) {
    NNGN_LOG_CONTEXT_CF(Audio);
    auto *const d = static_cast<Data*>(this->data);
    ALuint buffer = 0, source = 0;
    CHECK(alGenBuffers, 1, &buffer);
    CHECK(alBufferData,
        buffer, AL_FORMAT_MONO16, v.data(),
        static_cast<int>(v.size()), static_cast<int>(this->m_rate));
    CHECK(alGenSources, 1, &source);
    CHECK(alSourcei, source, AL_BUFFER, static_cast<ALint>(buffer));
    if(d->sources.full())
        d->sources.set_capacity(2 * d->sources.capacity());
    const auto ret = d->sources.size();
    d->sources.insert(Source{
        .id = source,
        .n = v.size(),
        .buffer = buffer,
    });
    if(d->buffers.size() <= ret)
        d->buffers.resize(ret);
    d->buffers[ret] = buffer;
    return ret;
}

bool Audio::remove_source(std::size_t i) {
    NNGN_LOG_CONTEXT_CF(Audio);
    auto *const d = static_cast<Data*>(this->data);
    auto source = d->sources.begin() + static_cast<std::ptrdiff_t>(i);
    const auto id = source->al_id(), b = source->buffer;
    d->sources.erase(source);
    d->buffers[i] = {};
    bool ret = true;
    ret = CHECK_LOG(alSourceStop, id) && ret;
    ret = CHECK_LOG(alDeleteSources, 1, &id) && ret;
    ret = CHECK_LOG(alDeleteBuffers, 1, &b) && ret;
    return ret;
}

vec3 Audio::source_pos(std::size_t i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    const auto &source = static_cast<Data*>(this->data)->sources[i];
    vec3 ret = {};
    CHECK(alGetSourcefv, source.al_id(), AL_POSITION, &ret.x);
    return ret;
}

std::size_t Audio::source_sample_pos(std::size_t i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    const auto &source = static_cast<Data*>(this->data)->sources[i];
    ALint ret = 0;
    CHECK(alGetSourcei, source.al_id(), AL_SAMPLE_OFFSET, &ret);
    return static_cast<std::size_t>(ret);
}

bool Audio::set_source_pos(std::size_t i, vec3 p) {
    NNGN_LOG_CONTEXT_CF(Audio);
    const auto &source = static_cast<Data*>(this->data)->sources[i];
    return CHECK_LOG(alSourcefv, source.al_id(), AL_POSITION, &p.x);
}

bool Audio::set_source_sample_pos(std::size_t i, std::size_t p) {
    NNGN_LOG_CONTEXT_CF(Audio);
    const auto &source = static_cast<Data*>(this->data)->sources[i];
    return CHECK_LOG(alSourcei,
        source.al_id(), AL_SAMPLE_OFFSET, static_cast<int>(p));
}

bool Audio::set_source_loop(std::size_t i, bool l) {
    NNGN_LOG_CONTEXT_CF(Audio);
    const auto &source = static_cast<Data*>(this->data)->sources[i];
    return CHECK_LOG(alSourcei, source.al_id(), AL_LOOPING, l);
}

bool Audio::play(std::size_t i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    const auto print_time = [r = this->m_rate](std::size_t s) {
        const auto ms = s * 1000 / r;
        return std::printf(
            "%zu:%02zu.%03zu", ms / 60'000, ms % 60 / 1'000, ms % 1000);
    };
    const auto &source = static_cast<Data*>(this->data)->sources[i];
    constexpr std::size_t bits_per_sample = 16;
    const auto n = source.n / (bits_per_sample / CHAR_BIT);
    return CHECK_LOG(alSourcePlay, source.al_id());
    for(;;) {
        ALint off = 0, state = 0;
        CHECK(alGetSourcei, source.al_id(), AL_SAMPLE_OFFSET, &off);
        CHECK(alGetSourcei, source.al_id(), AL_SOURCE_STATE, &state);
        if(!off && state == AL_STOPPED)
            off = static_cast<ALint>(n);
        auto nw = print_time(static_cast<std::size_t>(off));
        nw += std::printf("/");
        nw += print_time(n);
        nw += std::printf(
            " %d/%zu (%d%%)",
            off, n, off * 100 / static_cast<int>(n));
        std::cout.flush();
        if(state == AL_STOPPED)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds{100});
        std::cout << '\r';
        for(auto c = static_cast<std::size_t>(nw); c--;)
            std::cout << ' ';
        std::cout << '\r';
    }
    std::cout << '\n';
    return CHECK_LOG(alSourceStop, source.al_id());
}

bool Audio::stop(std::size_t i) const {
    NNGN_LOG_CONTEXT_CF(Audio);
    return CHECK_LOG(alSourceStop,
        static_cast<Data*>(this->data)->sources[i].al_id());
}

}
#endif
