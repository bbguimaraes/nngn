/**
 * \file
 * \brief Operations on light/full user data values.
 *
 * These are the low-level primitives used to manipulate user types.  \ref
 * src/lua/register.h "register.h" describes the user API.
 */
#ifndef NNGN_LUA_USER_DATA_H
#define NNGN_LUA_USER_DATA_H

#include <string_view>

#include "os/platform.h"
#include "utils/alloc/block.h"
#include "utils/concepts/fundamental.h"
#include "utils/def.h"
#include "utils/fixed_string.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "lua.h"
#include "state.h"
#include "value.h"

namespace nngn::lua {

namespace detail {

/**
 * Base operations which do not depend on the template type.
 * \see nngn::lua::user_data
 */
struct user_data_base {
protected:
    /** Pushes the table stored as \p meta in the global table. */
    static table push_metatable(state_view lua, std::string_view meta);
    static bool check_type(
        state_view lua, int i, std::string_view meta);
    static bool check_pointer_type(
        state_view lua, int i, std::string_view meta);
};

/**
 * Prevents forming a type with an abstract type as a member.
 * The \ref nngn::empty member itself is purely declarative and never used.
 */
template<typename T>
using stored_type = std::conditional_t<std::is_abstract_v<T>, empty, T>;

}

template<typename T>
inline constexpr auto user_data_header_align =
    std::max(alignof(T), alignof(T*));

/** Header placed before a user data allocation. */
template<typename T>
struct alignas(user_data_header_align<T>) user_data_header {
    /** Pointer to the user data object.  \see user_data */
    T *p = nullptr;
};

/**
 * Data block allocated for user data objects.
 * This object is owning (i.e. must be destroyed / garbage collected) if
 * `header.p` points to `data`.  Otherwise,`header.p` points to an existing
 * (external) object and no space is allocated for `data`.
 */
template<typename T>
class user_data :
    detail::user_data_base,
    alloc_block<user_data_header<T>, detail::stored_type<T>>::storage
{
    using header_type = user_data_header<T>;
    using base_type = typename alloc_block<header_type, T>::storage;
    static constexpr auto meta =
        nngn::lua::metatable_name<
            std::remove_const_t<std::remove_pointer_t<T>>>;
    friend class user_data<T*>;
public:
    using get_type = std::remove_pointer_t<T>*;
    /** Retrieves a user data of type \p T from the stack. */
    static get_type get(state_view lua, int i);
    /**
     * Retrieves a user data of type \p T from a light user data value.
     * No type verification is performed.
     * \param p
     *     Must be a pointer to an object in the same format as what is pushed
     *     by \ref push.
     */
    static get_type from_light(const void *p);
    /** Verifies that the value on the stack is a user data of type \p T. */
    static bool check_type(state_view lua, int i);
    /** Pushes the type's meta table onto the stack. */
    static table push_metatable(state_view lua);
    /**
     * Function to be used as a user data's `__gc` meta-method.
     * E.g.:
     * \code{.cpp}
     * mt["__gc"] = &user_data<T>::gc;
     * \endcode
     */
    static int gc(lua_State *L);
    /** Function to be used as a user data's `__eq` meta-method.
     * Compares objects by identity (i.e. address).
     * \code{.cpp}
     * mt["__eq"] = &user_data<T>::eq;
     * \endcode
     */
    static int eq(lua_State *L);
    /**
     * Pushes a value on the stack as a user data of type \p T.
     * \p R may be either a pointer, reference, or value type.  For pointers and
     * lvalue references, the user data is a reference to an existing value.
     * For value or rvalue references, the user data is pushed as an independent
     * copy of the value.
     */
    template<typename R> static int push(state_view lua, R &&r);
    /** Creates a non-owning (reference) object. */
    explicit user_data(const T &x);
    /** Creates an owning (copy) object. */
    explicit user_data(T &&x);
    /** Pointer to the contained/referenced object. */
    get_type get(void) const { return this->header.p; }
    /**
     * Destroys the `T` value if this is an owning user data value.
     * This is done when the value was initially pushed as a value / rvalue
     * reference type.
     * \see push
     */
    void destroy(void);
private:
    static get_type get_pointer(state_view lua, int i);
};

template<typename T>
requires(std::same_as<void, std::decay_t<T>>)
struct user_data<T> {
    static void *from_light(const void *p);
};

template<typename T>
auto user_data<T>::get_pointer(state_view lua, int i) -> get_type {
    if constexpr(std::is_pointer_v<T>)
        return user_data<std::remove_pointer_t<T>>::get_pointer(lua, i);
    else {
        auto *const p = Math::align_ptr<user_data>(lua.get<void*>(i));
        return p ? p->header.p : nullptr;
    }
}

template<typename T>
auto user_data<T>::get(state_view lua, int i) -> get_type {
    assert(user_data::check_type(lua, i));
    return user_data::get_pointer(lua, i);
}

template<typename T>
auto user_data<T>::from_light(const void *p) -> get_type {
    return p ? static_cast<const user_data*>(p)->get() : nullptr;
}

template<typename T>
bool user_data<T>::check_type(state_view lua, int i) {
    if constexpr(std::is_pointer_v<T>)
        return user_data_base::check_pointer_type(lua, i, user_data::meta);
    else
        return user_data_base::check_type(lua, i, user_data::meta);
}

template<typename T>
template<typename R>
int user_data<T>::push(state_view lua, R &&r) {
    NNGN_LOG_CONTEXT_CF(user_data);
    constexpr auto is_pointer = pointer<std::decay_t<R>>;
    if constexpr(is_pointer)
        if(!r)
            return stack_push<>::push(lua, nil);
    using UT = std::conditional_t<
        is_pointer, user_data::header_type, user_data>;
    new (lua.new_user_data<UT>()) UT{FWD(r)};
    static_assert(
        !std::same_as<decltype(meta), const empty>,
        "meta table_name not defined for type");
    auto mt = user_data_base::push_metatable(lua, meta);
    lua_setmetatable(lua, -2);
    mt.release();
    return 1;
}

template<typename T>
int user_data<T>::gc(lua_State *L) {
    const state_view lua = {L};
    assert(user_data::check_type(lua, 1));
    Math::align_ptr<user_data>(lua.get<void*>(1))->destroy();
    return 0;
}

template<typename T>
int user_data<T>::eq(lua_State *L) {
    const auto [p0, p1] = state_view{L}.get<std::tuple<T*, T*>>(1);
    return stack_push<>::push(L, p0 == p1);
}

template<typename T>
user_data<T>::user_data(const T &x)
    : base_type{.header = {.p = &x}, .data = x} {}

template<typename T>
user_data<T>::user_data(T &&x)
    : base_type{.header = {.p = &this->data}, .data = FWD(x)} {}

template<typename T>
void user_data<T>::destroy(void) {
    auto *const p = this->header.p;
    auto *const d = &this->data;
    if constexpr(std::same_as<empty, detail::stored_type<T>>)
        assert(static_cast<void*>(p) != static_cast<void*>(d));
    else if(p == d) // XXX technically undefined behavior when false
        std::destroy_at(d);
}

}

#endif
