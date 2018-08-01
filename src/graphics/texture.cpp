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

std::uint32_t Textures::insert(std::uint32_t i, const std::byte *p) {
    if(!this->update_data(i, p))
        return 0;
    ++this->m_n;
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

std::uint32_t Textures::load_data(const char *name, const std::byte *p) {
    NNGN_LOG_CONTEXT_CF(Textures);
    NNGN_LOG_CONTEXT(name);
    const auto i = this->m_n;
    if(!check_max(this->m_max, i))
        return 0;
    return this->insert(i, p);
}

std::uint32_t Textures::load(const char *filename) {
    NNGN_LOG_CONTEXT_CF(Textures);
    NNGN_LOG_CONTEXT(filename);
    const auto i = this->m_n;
    if(!check_max(this->m_max, i))
        return 0;
    const auto data = Textures::read(filename);
    if(data.empty())
        return 0;
    return this->insert(i, data.data());
}

bool Textures::update_data(std::uint32_t i, const std::byte *p) const {
    NNGN_LOG_CONTEXT_CF(Textures);
    return !this->graphics || this->graphics->load_textures(i, 1, p);
}

}
