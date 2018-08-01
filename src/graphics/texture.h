#ifndef NNGN_GRAPHICS_TEXTURE_H
#define NNGN_GRAPHICS_TEXTURE_H

#include <vector>

#include "math/hash.h"

namespace nngn {

struct Graphics;

/**
 * Texture manager, loads image data from files/buffers.
 *
 * All images are assumed to be rectangular and of \ref Graphics::TEXTURE_EXTENT
 * size.  Attempting to load images with the wrong size from a file will fail.
 * If a graphics back end is set (via \ref set_graphics), texture data is
 * uploaded to it via \ref Graphics::load_textures.
 *
 * The index \c 0 is special: it is pre-allocated when the object is initialized
 * and returned in case of errors.  It is a texture with no associated data.
 */
class Textures {
public:
    /** Reads raw RGBA image data from a file. */
    static std::vector<std::byte> read(const char *filename);
    /** Reverses the rows of a buffer containing raw RGBA image data. */
    static void flip_y(std::vector<std::byte> *v);
    /** Maximum number of textures that can be loaded, see \ref set_max. */
    std::uint32_t max() const { return static_cast<uint32_t>(this->m_max); }
    /** Number of loaded images. */
    std::uint32_t n() const { return this->m_n; }
    /** Sets the graphics back end where textures will be uploaded. */
    void set_graphics(Graphics *g) { this->graphics = g; }
    /**
     * Sets the maximum number of textures that can be loaded.
     * Attempting to perform a load beyond this limit will fail.  It is assumed
     * that the graphics back end has been configured to hold at least \c n
     * textures.
     */
    void set_max(std::uint32_t n) { this->m_max = n; }
    /** Loads RGBA texture data from a buffer. */
    std::uint32_t load_data(const char *name, const std::byte *p);
    /** Loads texture data from a file. */
    std::uint32_t load(const char *filename);
    /** Updates a previously loaded texture. */
    bool update_data(std::uint32_t i, const std::byte *p) const;
private:
    Graphics *graphics = nullptr;
    std::size_t m_max = 1;
    std::uint32_t m_n = 1;
    std::uint32_t insert(std::uint32_t i, const std::byte *p);
};

}

#endif
