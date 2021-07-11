/**
 * \dir tools
 * \brief External tooling.
 *
 * The programs in this directory are external tools used to inspect and
 * interact with the main engine process.  They share some code with it, but are
 * executed as separate processes and communicate via pipes and the engine's Lua
 * socket.
 *
 * See the screenshots page for some examples:
 *
 * https://bbguimaraes.com/nngn/screenshots/tools.html
 *
 * Most tools have corresponding directories under `src/lua` and `src/lua/lib`.
 * Both are libraries used to launch the tool from within the engine, the former
 * offers a more convenient interface which performs common tasks for
 * interactive sessions.
 *
 * Tools can be divided into input and output.  Input tools are:
 *
 * - `launcher` is a small window with several buttons that can be used to
 *   quickly start other tools with preset configurations.  It can persist
 *   across engine invocations.
 * - `configure`: a generic configuration dialog with support for boolean,
 *   integer, floating-point, and text values.
 * - `camera_server`: a web server which receives orientation data from a
 *   (possibly external) device (optionally via Javascript using its simple web
 *   interface) and translates it into camera orientation commands.
 *   See https://bbguimaraes.com/nngn/screenshots/engine.html#videos.
 *
 * Output tools are:
 *
 * - `inspect` is a simple text field which can display textual data, such as
 *   the text dump of the texture cache.
 * - `plot` displays conventional line graphs, such as memory usage and camera
 *   parameters.
 * - `timeline` displays line graphs as segments of a fixed time period, used to
 *   display various types of profiling information.
 */
