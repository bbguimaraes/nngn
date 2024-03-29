nngn
====

[https://bbguimaraes.com/nngn](https://bbguimaraes.com/nngn)

Graphics/physics/audio/game engine.

- [documentation](https://bbguimaraes.com/nngn/docs)
- [building](#building)

Documentation
-------------

Documentation generated from comments using `Doxygen` is available
[here](https://bbguimaraes.com/nngn/docs).  It can of course also be read inline
in the source code.  Source code is organized in the following directories:

- [`src`](https://bbguimaraes.com/nngn/docs/dir_68267d1309a1af8e8297ef4c3efbcdba.html):
  main source code directory.  Most of the code is contained in one of the
  subdirectories, which form mostly-independent modules.
- [`src/lua`](https://bbguimaraes.com/nngn/docs/dir_eb2a6c909ccee19b40dc174b15a80916.html):
  Lua libray and scripts.
- `scripts`: auxiliary scripts for building and interacting with the program.
- `tests`: unit and integration tests for the C++ and Lua components.
- `tools`: external and independent tools that interact with the engine to
  display information and control execution (inspection, profiling,
  configuration, etc.).

Building
--------

All targets use `autotools`.  For a native binary, simply execute:

```sh
./configure
make
```

See [dependencies](#dependencies) for the required packages.

The following build environments are regularly tested and known to work:

- Linux with GCC/Clang
- [FreeBSD with GCC/Clang](#freebsd)
- [Windows with `mingw`](#mingw)
- [WebAssembly with `emscripten`](#emscripten)

### MinGW

Cross-compiling with `mingw` is supported with the usual `configure` arguments:

```sh
./configure --build=x86_64-pc-linux-gnu --host=x86_64-w64-mingw32 # …
```

See [scripts/src_build.sh](./scripts/src_build.sh) for an example of how to
build dependencies from source.

### Emscripten

A WebAssembly application can be built using `emscripten`.  It provides all
[dependencies](#dependencies) listed below, except for Lua, which must be built
from source.  See [scripts/src_build.sh](./scripts/src_build.sh) for an example.

Then, follow the `autotools` process, prefixing `configure` with `emconfigure`.
`pkg-config` doesn't currently work with it, but the
[scripts/emscripten/pkgconfig](./scripts/emscripten/pkgconfig) directory has
dummy files that can be used:

```sh
EM_PKG_CONFIG_LIBDIR=scripts/emscripten/pkgconfig emconfigure ./configure #…
make nngn.js
```

The following `configure` options are relevant when targeting WebAssembly:

- `--disable-benchmarks`
- `--disable-tests`
- `--disable-tools`
- `--without-opencl`
- `--without-vulkan`

### Optional features

Some additional capabilities can be enabled via the usual `configure` script
option format.  Some of them, described in [dependencies](#dependencies),
require additional libraries.  The ones in this section do not, but can be
selectively enabled.

- `--enable-lua-alloc`: use a custom Lua allocator (see `lua_Alloc` in the
  documentation) which tracks all memory alloctions for each state.  This
  information can be visualized at runtime.

### Dependencies

These are the required packages for building the main program, along with the
versions known to work:

- `lua` (5.4.4): embedded scripting language

The default build of the program is very minimal.  Additional functionality can
be enabled by passing extra flags to the `configure` script.

See [scripts/container.sh](./scripts/container.sh) for an example script that
creates a container with all the build dependencies.

#### `--with-opencl`

Enables the OpenCL compute backend.  Requires:

- `OpenCL` (2.2): heterogeneous computing library

#### `--with-opengl`

Enables the OpenGL graphics backend.  Requires:

- `glfw` (3.3): portable GUI library
- `gl` (4.5, 3.1es): graphics library
- `glew` (2.1.0): OpenGL extension loading library

#### `--with-vulkan`

Enables the Vulkan graphics backend.  Requires:

- `glfw` (3.3): portable GUI library
- `vulkan` (1.1.130): graphics library
- `glslang` (11.1.0): GLSL compiler

Additionally, if the VulkanMemoryAllocator library is present (determined by the
`vk_mem_alloc.h` header file), it is used for some of the memory allocations.
This default can be disabled with `--without-vma`.

#### `--with-libpng`

Enables loading images.  Requires:

- `libpng` (1.6.37): PNG loading library

#### `--with-freetype2`

Enables loading fonts.  Requires:

- `freetype2` (2.10.1): font library

#### Tests

Building the tests requires `Qt5Core` and `Qt5Test` (5.14.0) and can be enabled
with the `--enable-tests` flag.  A `clang-tidy` check is also available via
`make tidy`.

The tests can be compiled and executed with several compiler protections and
sanitizers.

The following compilers have been tested:

- `gcc` (11.1.0)
- `clang` (12.0.1)

The following compiler flags have been tested:

- `-D_GLIBCXX_SANITIZE_VECTOR`
- `-fsanitize-address-use-after-scope`
- `-fstack-protector`

The following sanitizers have been tested:

- `address`
- `leak`
- `undefined`
- `pointer-compare`
- `pointer-subtract`

#### Benchmarks

Building the benchmakrs requires `Qt5Core` and `Qt5Test` (5.14.0) and can be
enabled with the `--enable-benchmarks` flag.

#### Tools

Buildings auxiliary tools requires `Qt5Widgets`, `QtNetwork`, `Qt5Charts`, and
`rustc` (1.41.1) and can be enabled with the `--enable-tools` flag.

#### FreeBSD

The packages required for a minimal build in FreeBSD are (available as pre-built
packages via `pkg`):

- `autoconf`
- `automake`
- `gcc11`
- `pkgconf`
