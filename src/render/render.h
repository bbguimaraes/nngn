/**
 * \dir src/render
 * \brief High-level rendering module.
 */
#ifndef NNGN_RENDER_RENDER_H
#define NNGN_RENDER_RENDER_H

#include <unordered_set>
#include <vector>

#include "lua/table.h"
#include "utils/flags.h"

#include "renderers.h"

namespace nngn {

struct Colliders;
struct Graphics;
class Fonts;
class Grid;
class Textures;
class Textbox;

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
    /** Enabled debugging features. */
    enum Debug : u8 {
        DEBUG_RENDERERS = 1u << 0,
        DEBUG_CIRCLE = 1u << 1,
        DEBUG_BB = 1u << 2,
        DEBUG_ALL = (1u << 3) - 1,
    };
    // Initialization
    /** Partially initializes this system.  \see set_graphics */
    void init(
        Textures *t, const Fonts *f, const Textbox *tb, const Grid *g,
        const Colliders *c);
    // Configuration
    auto debug(void) const { return *this->m_debug; }
    bool perspective(void) const;
    auto max_sprites(void) const { return this->sprites.capacity(); }
    auto max_screen_sprites(void) const;
    auto max_cubes(void) const { return this->cubes.capacity(); }
    auto max_voxels(void) const { return this->voxels.capacity(); }
    /** Total number of active renderers. */
    std::size_t n(void) const;
    /** Number of active sprite renderers. */
    std::size_t n_sprites(void) const { return this->sprites.size(); }
    /** Number of active screen sprite renderers. */
    std::size_t n_screen_sprites(void) const { return this->sprites.size(); }
    /** Number of active cube renderers. */
    std::size_t n_cubes(void) const { return this->cubes.size(); }
    /** Number of active voxel renderers. */
    std::size_t n_voxels(void) const { return this->voxels.size(); }
    bool selected(const Renderer *p) const;
    void set_debug(Debug d);
    void set_perspective(bool p);
    bool set_max_sprites(std::size_t n);
    bool set_max_screen_sprites(std::size_t n);
    bool set_max_cubes(std::size_t n);
    bool set_max_voxels(std::size_t n);
    bool set_max_text(std::size_t n);
    bool set_max_colliders(std::size_t n);
    /**
     * Associates this system with a graphics back end.
     * Must be called before \c update.  It is assumed that the current
     * associated back end, if it exists, will be destroyed shortly, so previous
     * resources are not released.
     */
    bool set_graphics(Graphics *g);
    // Operations
    /** Adds a new renderer object according to a description in a Lua table. */
    Renderer *load(nngn::lua::table_view t);
    /** Removes a renderer.  The corresponding entity is not changed. */
    void remove(Renderer *p);
    /** Processes changed renderers/configuration and updates graphics. */
    void add_selection(const Renderer *p);
    void remove_selection(const Renderer *p);
    bool update(void);
private:
    bool update_renderers(
        bool sprites_updated, bool screen_sprites_updated, bool cubes_updated,
        bool voxels_updated);
    bool update_debug(
        bool sprites_updated, bool screen_sprites_updated, bool cubes_updated,
        bool voxels_updated);
    enum Flag : u8 {
        SPRITES_UPDATED = 1u << 0,
        SCREEN_SPRITES_UPDATED = 1u << 1,
        CUBES_UPDATED = 1u << 2,
        VOXELS_UPDATED = 1u << 3,
        DEBUG_UPDATED = 1u << 4,
        SELECTION_UPDATED = 1u << 5,
        PERSPECTIVE = 1u << 6,
    };
    Flags<Flag> flags = {};
    Flags<Debug> m_debug = {};
    Textures *textures = nullptr;
    Graphics *graphics = nullptr;
    const Fonts *fonts = nullptr;
    const Textbox *textbox = nullptr;
    const Grid *grid = nullptr;
    const Colliders *colliders = nullptr;
    std::vector<SpriteRenderer> sprites = {};
    std::vector<SpriteRenderer> screen_sprites = {};
    std::vector<CubeRenderer> cubes = {};
    std::vector<VoxelRenderer> voxels = {};
    std::unordered_set<const Renderer*> selections = {};
    u32
        sprite_vbo = {}, sprite_ebo = {},
        sprite_debug_vbo = {}, sprite_debug_ebo = {},
        screen_sprite_vbo = {}, screen_sprite_ebo = {},
        screen_sprite_debug_vbo = {}, screen_sprite_debug_ebo = {},
        cube_vbo = {}, cube_ebo = {},
        cube_debug_vbo = {}, cube_debug_ebo = {},
        voxel_vbo = {}, voxel_ebo = {},
        voxel_debug_vbo = {}, voxel_debug_ebo = {},
        text_vbo = {}, text_ebo = {},
        textbox_vbo = {}, textbox_ebo = {},
        selection_vbo = {}, selection_ebo = {},
        aabb_vbo = {}, aabb_ebo = {},
        aabb_circle_vbo = {}, aabb_circle_ebo = {},
        bb_vbo = {}, bb_ebo = {},
        bb_circle_vbo = {}, bb_circle_ebo = {};
};

inline bool Renderers::perspective(void) const {
    return this->flags.is_set(Flag::PERSPECTIVE);
}

inline auto Renderers::max_screen_sprites(void) const {
    return this->screen_sprites.capacity();
}

inline bool Renderers::selected(const Renderer *p) const {
    return this->selections.find(p) != cend(this->selections);
}

}

#endif
