#include <cstring>
#include <fstream>
#include <vector>

#include "os/platform.h"

using namespace std::string_view_literals;

#ifdef NNGN_PLATFORM_HAS_LIBPNG
#include <png.h>
#endif

#include "math/math.h"
#include "utils/log.h"

#include "graphics.h"
#include "texture.h"

using nngn::u32;

namespace {

constexpr auto SIZE = nngn::Graphics::TEXTURE_SIZE;
constexpr auto EXTENT = nngn::Graphics::TEXTURE_EXTENT;

bool check_max(std::size_t n, u32 i) {
    if(static_cast<std::size_t>(i) < n)
        return true;
    nngn::Log::l()
        << "cannot load more textures "
        "(max = " << n << ")" << std::endl;
    return false;
}

#ifndef NNGN_PLATFORM_HAS_LIBPNG

bool write_png(const char*, std::span<const std::byte>) {
    nngn::Log::l() << "compiled without libpng support\n";
    return false;
}

#else

bool write_png(const char *filename, std::span<const std::byte> s) {
    png_image img = {
        .version = PNG_IMAGE_VERSION,
        .width = EXTENT,
        .height = EXTENT,
        .format = PNG_FORMAT_RGBA,
    };
    png_image_write_to_file(&img, filename, 0, s.data(), 0, nullptr);
    switch(img.warning_or_error & 0b11) {
    case 1:
        nngn::Log::l() << "warning: " << img.message << '\n';
        [[fallthrough]];
    case 0:
        return true;
    default:
        nngn::Log::l() << img.message << '\n';
        return false;
    }
}

#endif

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
    if(!png_image_begin_read_from_file(&img, filename))
        Log::l() << img.message << std::endl;
    else if(img.width != EXTENT && img.height != EXTENT)
        Log::l()
            << "only " << EXTENT << "x" << EXTENT
            << " images are supported, got "
            << img.width << "x" << img.height
            << std::endl;
    else {
        img.format = PNG_FORMAT_RGBA;
        ret.resize(static_cast<std::size_t>(PNG_IMAGE_SIZE(img)));
        png_image_finish_read(&img, nullptr, ret.data(), 0, nullptr);
    }
    return ret;
}

#endif

bool Textures::write(
    const char *filename, std::span<const std::byte> s, Format fmt
) {
    NNGN_LOG_CONTEXT_CF(Textures);
    [[maybe_unused]] constexpr std::size_t channels = 4;
    assert(s.size() == channels * EXTENT * EXTENT);
    switch(fmt) {
    case Format::PNG: return write_png(filename, s);
    case Format::PPM: {
        std::ofstream f(filename, std::ios::binary);
        if(!(f && (f << "P3 "sv << EXTENT << ' ' << EXTENT << "\n255\n")))
            return nngn::Log::perror(), false;
        for(std::size_t i = 0; i < s.size(); i += 4)
            if(!(f << static_cast<unsigned>(s[i]) << ' '
                << static_cast<unsigned>(s[i + 1]) << ' '
                << static_cast<unsigned>(s[i + 2]) << '\n'
            ))
                return nngn::Log::perror(), false;
        return true;
    }
    default:
        Log::l() << "invalid format: " << static_cast<int>(fmt) << '\n';
        return false;
    }
}

u32 Textures::insert(u32 i, const std::byte *p) {
    if(!this->update_data(i, p))
        return 0;
    ++this->m_n;
    return i;
}

void Textures::red_to_rgba(
    std::span<std::byte> dst, std::span<const std::byte> src)
{
    constexpr auto n = SIZE / 4;
    assert(SIZE <= dst.size());
    assert(n <= src.size());
    for(std::size_t i = 0; i != n; ++i)
        std::fill(&dst[4 * i], &dst[4 * (i + 1)], src[i]);
}

void Textures::flip_y(std::span<std::byte> s) {
    constexpr auto n = static_cast<std::size_t>(EXTENT);
    constexpr auto n4 = 4 * n;
    std::vector<std::byte> tmp_v(4 * n);
    auto *p = s.data(), *tmp = tmp_v.data();
    for(std::size_t i = 0, e = n / 2; i < e; ++i) {
        auto *const src = 4 * n * (n - 1 - i) + p;
        auto *const dst = 4 * n * i + p;
        std::memcpy(tmp, src, n4);
        std::memcpy(src, dst, n4);
        std::memcpy(dst, tmp, n4);
    }
}

u32 Textures::load_data(const char *name, const std::byte *p) {
    NNGN_LOG_CONTEXT_CF(Textures);
    NNGN_LOG_CONTEXT(name);
    const auto i = this->m_n;
    if(!check_max(this->m_max, i))
        return 0;
    return this->insert(i, p);
}

u32 Textures::load(const char *filename) {
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

bool Textures::update_data(u32 i, const std::byte *p) const {
    NNGN_LOG_CONTEXT_CF(Textures);
    return !this->graphics || this->graphics->load_textures(i, 1, p);
}

}
