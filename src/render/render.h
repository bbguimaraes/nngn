/**
 * \dir src/render
 * \brief High-level rendering module.
 */
#ifndef NNGN_RENDER_RENDER_H
#define NNGN_RENDER_RENDER_H

#include <vector>

#include "lua/table.h"
#include "utils/flags.h"

#include "renderers.h"

namespace nngn {

struct Graphics;

/**
 * Rendering subsystem.  Renders to the screen via the graphics subsystem.
 * This system stores all renderers and rendering configuration.  Renderers are
 * added via the Lua interface and always have an associated entity.  At every
 * frame, changes are processed and new rendering data are sent to the graphics
 * system to be displayed.
 * \see Entity
 * \see Gen
 * \see Graphics
 */
class Renderers {
public:
    // Configuration
    auto max_sprites(void) const { return this->sprites.capacity(); }
    /** Total number of active renderers. */
    std::size_t n(void) const;
    /** Number of active sprite renderers. */
    std::size_t n_sprites(void) const { return this->sprites.size(); }
    bool set_max_sprites(std::size_t n);
    /**
     * Associates this system with a graphics back end.
     * Must be called before \c update.  It is assumed that the current
     * associated back end, if it exists, will be destroyed shortly, so previous
     * resources are not released.
     */
    bool set_graphics(Graphics *g);
    // Operations
    /** Adds a new renderer object according to a description in a Lua table. */
    Renderer *load(const nngn::lua::table &t);
    /** Removes a renderer.  The corresponding entity is not changed. */
    void remove(Renderer *p);
    /** Processes changed renderers/configuration and updates graphics. */
    bool update(void);
private:
    bool update_renderers(bool sprites_updated);
    enum Flag : u8 {
        SPRITES_UPDATED = 1u << 0,
    };
    Flags<Flag> flags = {};
    Graphics *graphics = nullptr;
    std::vector<SpriteRenderer> sprites = {};
    u32 sprite_vbo = {}, sprite_ebo = {};
};

}

#endif