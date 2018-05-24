/**
 * \dir src/input
 * \brief Terminal/keyboard/mouse input.
 */
#ifndef NNGN_INPUT_INPUT_H
#define NNGN_INPUT_INPUT_H

#include <bitset>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "utils/def.h"
#include "utils/utils.h"

struct lua_State;

namespace nngn {

using nngn::i32;

class BindingGroup;
struct Graphics;

/** Dispatches keyboard input events to pre-registered Lua functions. */
class Input {
public:
    /**
     * Indicates that a callback should only handle a sub-set of events.
     * E.g.: a callback specified with the `PRESS` selector will only handle key
     * press events.  The absence of a selector matches both possible cases.
     */
    enum Selector : std::uint8_t {
        PRESS = 1u << 0, CTRL = 1u << 1, ALT = 1u << 2,
    };
    enum Action : std::uint8_t {
        KEY_PRESS = 0b1, KEY_RELEASE = 0,
    };
    enum Modifier : std::uint8_t {
        MOD_SHIFT = 0b1, MOD_CTRL = 0b10, MOD_ALT = 0b100,
    };
    enum {
        KEY_ESC = 256, KEY_ENTER, KEY_TAB,
        KEY_RIGHT = 262, KEY_LEFT, KEY_DOWN, KEY_UP,
        KEY_PAGE_UP = 266, KEY_PAGE_DOWN,
        KEY_MAX = 348,
    };
    struct Source {
        virtual ~Source() = 0;
        virtual void get_keys(std::span<i32> keys) const;
        virtual bool update(Input*) { return true; }
    protected:
        NNGN_DEFAULT_CONSTRUCT(Source)
    };
    enum TerminalFlag : std::uint8_t {
        OUTPUT_PROCESSING = 1u << 0,
    };
    void init(lua_State *L_) { this->L = L_; }
    void get_keys(std::span<i32> keys) const;
    void has_override(std::size_t n, i32 *keys) const;
    BindingGroup *binding_group() const { return this->m_binding_group; }
    void set_binding_group(BindingGroup *g) { this->m_binding_group = g; }
    void add_source(std::unique_ptr<Source> p);
    bool remove_source(Source *p);
    bool override_keys(bool pressed, std::span<const i32> keys);
    bool register_callback();
    bool remove_callback();
    bool update();
    bool key_callback(int key, Action action, Modifier mods) const;
private:
    lua_State *L = {};
    int callback_ref = {};
    BindingGroup *m_binding_group = {};
    std::bitset<KEY_MAX + 1> overrides = {};
    std::vector<std::unique_ptr<Source>> sources = {};
};

std::unique_ptr<Input::Source> input_terminal_source(
    int fd, Input::TerminalFlag flags);
std::unique_ptr<Input::Source> input_graphics_source(Graphics *g);

}

#endif
