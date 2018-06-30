#include "font.h"

#include "os/platform.h"
#include "utils/log.h"

#ifndef NNGN_PLATFORM_HAS_FREETYPE2

namespace {

bool init(void**) { return true; }

void destroy(void*) {}

bool load(void*, const char*, nngn::Graphics*, nngn::Font*) {
    nngn::Log::l() << "compiled without freetype3 support\n";
    return false;
}

}

#else

#include <ft2build.h>

#include FT_FREETYPE_H

#include "graphics/graphics.h"
#include "math/vec4.h"
#include "utils/scoped.h"

namespace {

const char *freetype_strerror(FT_Error err) {
#if FREETYPE_MAJOR < 2 || ( \
        FREETYPE_MAJOR == 2 && ( \
        FREETYPE_MINOR < 6 || ( \
            FREETYPE_MINOR == 6 && FREETYPE_PATCH < 3)))
#    undef __FTERRORS_H__
#else
#    undef FTERRORS_H_
#endif
#define FT_ERROR_START_LIST switch (err) {
#define FT_ERRORDEF(e, v, s) case v: return s;
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H
#undef FT_ERRORDEF
    return nullptr;
}

bool init(void **p) {
    auto *ft = reinterpret_cast<FT_Library*>(p);
    if(const auto err = FT_Init_FreeType(ft); err != FT_Err_Ok) {
        nngn::Log::l()
            << "FT_Init_FreeType: "
            << freetype_strerror(err) << std::endl;
        return false;
    }
    return true;
}

void destroy(void *p) {
    FT_Done_FreeType(static_cast<FT_Library>(p));
}

void gen_tex(
    std::size_t width, std::size_t height,
    const std::byte *in, nngn::vec4_base<std::byte> *out
) {
    constexpr auto w = std::byte{255};
    for(std::size_t y = 0; y != height; ++y)
        for(std::size_t x = 0; x != width; ++x)
            *out++ = {w, w, w, *in++};
}

bool load(
        void *p_ft, const char *filename,
        nngn::Graphics *graphics, nngn::Font *f) {
    auto *ft = static_cast<FT_Library>(p_ft);
    FT_Error err = FT_Err_Ok;
    FT_Face face = {};
    if((err = FT_New_Face(ft, filename, 0, &face)) != FT_Err_Ok) {
        nngn::Log::l()
            << "FT_New_Face(" << filename << "): "
            << freetype_strerror(err) << std::endl;
        return false;
    }
    auto done_face = nngn::make_scoped([face] { FT_Done_Face(face); });
    FT_Set_Pixel_Sizes(face, 0, f->size);
    if(graphics)
        graphics->resize_font(f->size);
    const auto size = static_cast<std::size_t>(f->size);
    using bvec4 = nngn::vec4_base<std::byte>;
    std::vector<bvec4> bitmap(size * size);
    bool ret = true;
    for(unsigned char c = 0; c < nngn::Font::N; ++c) {
        if((err = FT_Load_Char(face, c, FT_LOAD_RENDER)) != FT_Err_Ok) {
            nngn::Log::l()
                << "FT_Load_Char(" << c << "): "
                << freetype_strerror(err) << std::endl;
            ret = false;
            if(graphics) {
                bitmap.clear();
                graphics->load_font(
                    c, 1, nngn::rptr(nngn::uvec2{}), &bitmap[0][0]);
            }
            continue;
        }
        const auto &g = *face->glyph;
        nngn::uvec2 bsize = {g.bitmap.width, g.bitmap.rows};
        if(f->size < bsize.x || f->size < bsize.y) {
            nngn::Log::l()
                << "character " << static_cast<unsigned>(c)
                << "('" << c << "')"
                << " larger than font size ("
                << bsize.x << "x" << bsize.y
                << " > " << f->size << "x" << f->size
                << ")\n";
            if(graphics)
                bitmap.clear();
            bsize = {f->size, f->size};
        }
        if(graphics) {
            if(bsize.x && bsize.y) {
                assert(bsize.x <= size && bsize.y <= size);
                gen_tex(
                    bsize.x, bsize.y,
                    static_cast<const std::byte*>(
                        static_cast<const void*>(g.bitmap.buffer)),
                    bitmap.data());
            } else
                std::ranges::fill(bitmap, bvec4{});
            if(!graphics->load_font(c, 1, &bsize, &bitmap[0][0])) {
                ret = false;
                continue;
            }
        }
        auto &r = f->chars[c];
        r.size = bsize;
        r.bearing = {
            g.bitmap_left,
            g.bitmap_top - static_cast<int>(g.bitmap.rows)};
        r.advance = static_cast<float>(g.advance.x) / 64.0f;
    }
    return ret;
}

}

#endif

#include "font.h"

namespace nngn {

Fonts::~Fonts() { destroy(this->ft); }
bool Fonts::init() { return ::init((&this->ft)); }

uint32_t Fonts::add(const Font &f) {
    this->v.push_back(f);
    return static_cast<uint32_t>(this->v.size() - 1);
}

uint32_t Fonts::load(unsigned int size, const char *filename) {
    NNGN_LOG_CONTEXT_CF(Fonts);
    Font f = {};
    f.size = size;
    if(::load(this->ft, filename, this->graphics, &f))
        return this->add(f);
    return 0;
}

}
