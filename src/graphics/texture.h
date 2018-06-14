#ifndef NNGN_GRAPHICS_TEXTURE_H
#define NNGN_GRAPHICS_TEXTURE_H

#include <cassert>
#include <string>
#include <string_view>
#include <vector>

#include "math/hash.h"
#include "utils/def.h"

namespace nngn {

struct Graphics;

/**
 * Texture manager, loads and caches image data from files/buffers.
 *
 * All images are assumed to be rectangular and of \ref Graphics::TEXTURE_EXTENT
 * size.  Attempting to load images with the wrong size from a file will fail.
 * If a graphics back end is set (via \ref set_graphics), texture data is
 * uploaded to it via \ref Graphics::load_textures.
 *
 * Image data is associated with a string ID (usually the file name) and a
 * reference count.  Future loads with the same ID will simply increment the
 * reference count (image data, if provided, is ignored).  When a texture is
 * removed, the reference count is decremented.  No special action is taken when
 * it reaches zero, but the slot is then considered unused and the ID will be
 * reused in future loads.
 *
 * The index \c 0 is special: it is pre-allocated when the object is initialized
 * and returned in case of errors.  It is a texture with no associated data.
 */
class Textures {
public:
    enum class Format : u8 {
        PNG = 1,
        PPM,
    };
    /** Reads raw RGBA image data from a file. */
    static std::vector<std::byte> read(const char *filename);
    /** Writes raw RGBA image data to a file. */
    static bool write(
        const char *filename, std::span<const std::byte> s, Format fmt);
    /** Reverses the rows of a buffer containing raw RGBA image data. */
    static void flip_y(std::vector<std::byte> *v);
    /** Maximum number of textures that can be loaded, see \ref set_max. */
    u32 max(void) const { return static_cast<u32>(this->counts.capacity()); }
    /** Number of active images in the cache. */
    u32 n(void) const;
    /**
     * Counter incremented every time the cache is changed.
     * Can be used to quickly track changes across frames, e.g. by an external
     * tool to know when to call \ref dump.
     */
    u64 generation(void) const { return this->gen; }
    /**
     * Sets the graphics back end where textures will be uploaded.
     * This method does not load any data, any pre-existing textures have to be
     * reloaded (see \c reload and \c reload_all).
     */
    void set_graphics(Graphics *g) { this->graphics = g; }
    /**
     * Sets the maximum number of textures that can be loaded.
     * Attempting to perform a load beyond this limit will fail, unless the
     * texture is already in the cache.  It is assumed that the graphics back
     * end has been configured to hold at least \c n textures.
     */
    void set_max(u32 n);
    /** Loads RGBA texture data from a buffer. */
    u32 load_data(const char *name, const std::byte *p);
    /** Loads texture data from a file. */
    u32 load(const char *filename);
    /** Reloads data for a single texture. */
    bool reload(u32 i);
    /** Reloads all texture data. */
    bool reload_all(void);
    /** Updates a previously loaded texture. */
    bool update_data(u32 i, const std::byte *p) const;
    /** Decrements a texture's reference count and cleans up if necessary. */
    void remove(u32 i);
    /** Increments the reference count of a texture. */
    void add_ref(u32 i, std::size_t n = 1);
    /** Increments the reference count of a texture by name. */
    void add_ref(const char *filename, std::size_t n = 1);
    /** Dumps name/ref_count pairs for all loaded textures. */
    std::vector<std::tuple<std::string_view, u32>> dump(void) const;
private:
    Graphics *graphics = nullptr;
    std::vector<Hash> hashes = {{}};
    std::vector<u32> counts = {1};
    std::vector<std::string> names = {{}};
    u64 gen = 0;
    u32 insert(u32 i, std::string_view name, const std::byte *p);
};

}

#endif
