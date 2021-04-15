#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

#include "os/platform.h"

#ifdef NNGN_PLATFORM_HAS_LIBPNG
#include <png.h>
#endif

#include "math/math.h"
#include "utils/log.h"

#include "graphics.h"
#include "texture.h"

namespace {

std::uint32_t find(const std::vector<nngn::Hash> &v, std::string_view name) {
    return static_cast<std::uint32_t>(
        std::distance(begin(v), std::find(begin(v), end(v), nngn::hash(name))));
}

std::uint32_t find_empty(const std::vector<std::uint32_t> &v) {
    return static_cast<std::uint32_t>(
        std::distance(begin(v), std::find(begin(v), end(v), 0)));
}

bool check_max(std::size_t n, std::uint32_t i) {
    if(i < n)
        return true;
    nngn::Log::l()
        << "cannot load more textures "
        "(max = " << n << ")" << std::endl;
    return false;
}

}

namespace nngn {

#ifndef NNGN_PLATFORM_HAS_LIBPNG

std::vector<std::byte> Textures::read(const char*) {
    Log::l() << "compiled without libpng support\n";
    return {};
}

#else

std::vector<std::byte> Textures::read(const char *filename) {
    NNGN_LOG_CONTEXT_CF(Textures);
    NNGN_LOG_CONTEXT(filename);
    std::vector<std::byte> ret;
    png_image img;
    std::memset(&img, 0, sizeof(img));
    img.version = PNG_IMAGE_VERSION;
    constexpr auto extent = Graphics::TEXTURE_EXTENT;
    if(!png_image_begin_read_from_file(&img, filename))
        Log::l() << img.message << std::endl;
    else if(img.width != extent && img.height != extent)
        Log::l()
            << "only " << extent << "x" << extent
            << " images are supported, got "
            << img.width << "x" << img.height
            << std::endl;
    else {
        img.format = PNG_FORMAT_RGBA;
        ret.resize(PNG_IMAGE_SIZE(img));
        png_image_finish_read(&img, nullptr, ret.data(), 0, nullptr);
    }
    return ret;
}

#endif

std::uint32_t Textures::insert(
    std::uint32_t i, std::string_view name, const std::byte *p
) {
    assert(i < this->counts.size());
    if(p && !this->update_data(static_cast<std::uint32_t>(i), p))
        return 0;
    this->hashes[i] = hash(name);
    this->counts[i] = 1;
    this->names[i] = name;
    ++this->gen;
    return i;
}

void Textures::flip_y(std::vector<std::byte> *v) {
    constexpr auto n = Graphics::TEXTURE_EXTENT, n4 = 4 * n;
    std::vector<std::byte> tmp_v(4 * n);
    auto *p = v->data(), *tmp = tmp_v.data();
    for(std::size_t i = 0, e = n / 2; i < e; ++i) {
        auto *const src = 4 * n * (n - 1 - i) + p;
        auto *const dst = 4 * n * i + p;
        std::memcpy(tmp, src, n4);
        std::memcpy(src, dst, n4);
        std::memcpy(dst, tmp, n4);
    }
}

std::uint32_t Textures::n() const {
    constexpr auto identity = [](auto &&x) { return FWD(x); };
    return static_cast<std::uint32_t>(
        std::count_if(begin(this->counts), end(this->counts), identity));
}

void Textures::set_max(std::uint32_t n) {
    this->hashes.resize(n);
    this->counts.resize(n);
    this->names.resize(n);
}

std::uint32_t Textures::load_data(const char *name, const std::byte *p) {
    NNGN_LOG_CONTEXT_CF(Textures);
    NNGN_LOG_CONTEXT(name);
    if(const auto i = find(this->hashes, name); i < this->counts.size()) {
        Log::l() << "already loaded, ignoring data" << std::endl;
        ++this->counts[i];
        return i;
    }
    const auto i = find_empty(this->counts);
    if(!check_max(this->counts.size(), i))
        return 0;
    return this->insert(i, name, p);
}

std::uint32_t Textures::load(const char *filename) {
    NNGN_LOG_CONTEXT_CF(Textures);
    NNGN_LOG_CONTEXT(filename);
    if(const auto i = find(this->hashes, filename); i < this->counts.size()) {
        ++this->counts[i];
        return i;
    }
    const auto i = find_empty(this->counts);
    if(!check_max(this->counts.size(), i))
        return 0;
    const auto data = Textures::read(filename);
    if(data.empty())
        return 0;
    return this->insert(i, filename, data.data());
}

bool Textures::reload(std::uint32_t i) {
    NNGN_LOG_CONTEXT_CF(Textures);
    if(!this->graphics)
        return true;
    const auto data = Textures::read(this->names[i].c_str());
    return !data.empty() && this->update_data(i, data.data());
}

bool Textures::reload_all() {
    for(std::size_t i = 1, n = this->counts.size(); i < n; ++i)
        if(this->counts[i] && !this->reload(static_cast<std::uint32_t>(i)))
            return false;
    return true;
}

bool Textures::update_data(std::uint32_t i, const std::byte *p) const {
    NNGN_LOG_CONTEXT_CF(Textures);
    return !this->graphics || this->graphics->load_textures(i, 1, p);
}

void Textures::remove(std::uint32_t i) {
    if(!i)
        return;
    assert(i);
    assert(this->counts[i]);
    --this->counts[i];
    ++this->gen;
}

void Textures::add_ref(std::uint32_t i, std::size_t n) {
    NNGN_LOG_CONTEXT_CF(Textures);
    this->counts[i] = static_cast<std::uint32_t>(this->counts[i] + n);
    ++this->gen;
}

void Textures::add_ref(const char *filename, std::size_t n) {
    NNGN_LOG_CONTEXT_CF(Textures);
    NNGN_LOG_CONTEXT(filename);
    const auto i = find(this->hashes, filename);
    assert(i < this->counts.size());
    this->counts[i] = static_cast<std::uint32_t>(this->counts[i] + n);
    ++this->gen;
}

std::vector<std::tuple<std::string_view, std::uint32_t>>
Textures::dump() const {
    std::vector<std::tuple<std::string_view, std::uint32_t>> ret;
    ret.reserve(this->counts.size());
    for(std::size_t i = 0, n = this->counts.size(); i < n; ++i)
        ret.emplace_back(this->names[i], this->counts[i]);
    return ret;
}

}
