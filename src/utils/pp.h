#ifndef NNGN_UTILS_PP_H
#define NNGN_UTILS_PP_H

/** Token pasting macro. */
#define NNGN_PASTE(x, y) x ## y
/**
 * Token pasting macro, with one level of indirection.
 * Expands its arguments as a result of the rules of macro expansion, which
 * state that argument expansion happens before the replacement text is
 * examined for macro expansion.
 */
#define NNGN_PASTE2(x, y) NNGN_PASTE(x, y)

/**
 * Counts the number of arguments given.
 * An empty call yields `0`.
 */
#define NNGN_ARGC(...) \
    NNGN_ARGC0(__VA_ARGS__ __VA_OPT__(,) 7, 6, 5, 4, 3, 2, 1, 0)
#define NNGN_ARGC0(_0, _1, _2, _3, _4, _5, _6, _7, ...) _7

/**
 * Calls an overloaded macro.
 * The intended usage is:
 *
 * \code{.cpp}
 * #define F(...) NNGN_OVERLOAD(F, __VA_ARGS__)
 * #define F0() // ...
 * #define F1(_0) // ...
 * #define F2(_0, _1) // ...
 * // ...
 * \endcode
 *
 * When `F` is called, the call will be forwarded to the name resulting from
 * pasting `F` with the number of arguments.
 */
#define NNGN_OVERLOAD(f, ...) \
    NNGN_PASTE2(f, NNGN_ARGC(__VA_ARGS__))(__VA_ARGS__)

#endif
