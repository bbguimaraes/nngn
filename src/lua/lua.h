/**
 * \dir src/lua
 * \brief Lua library and scripts.
 *
 * This module performs three functions:
 *
 * - The C++ files (`*.{h,cpp}`) are a generic library for `C++ ←→ Lua`
 *   interaction.  It provides a convenient, typed interface built on top of
 *   Lua's C API that can be used to expose functionality from C++ to Lua and
 *   interact with the VM.
 * - The Lua files in the `lib/` subdirectory are a convenience library built on
 *   top of the engine in the main `src/` directory.  It both wraps the existing
 *   components in Lua interfaces and augments them with peripheral,
 *   higher-level components.
 * - The Lua files at the root (`*.lua`) are a wrapper around the `lib/`
 *   subdirectory with preset values and functions for common tasks, used mostly
 *   in interactive exploratory and debugging sessions.
 */
