nngn
====

[https://bbguimaraes.com/nngn](https://bbguimaraes.com/nngn)

Game/graphics/physics engine.

Building
--------

All targets use `autotools`.  For a native binary, simply execute:

```sh
./configure
make
```

See [dependencies](#dependencies) for the required packages.

### emscripten

A WebAssembly application can be built using `emscripten`.  It provides all
[dependencies](#dependencies) listed below, except for one that must be built
from source.  See [scripts/emscripten/build.sh](./scripts/emscripten/build.sh)
for the steps to do that.

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

### dependencies

These are the required packages for building the main program, along with the
versions known to work:

- `lua` (5.3.5): embedded scripting language
- `sol` (3.2.0): c++/lua wrapper

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

#### tests

Building the tests requires `Qt5Core` and `Qt5Test` (5.14.0) and can be enabled
with the `--enable-tests` flag.  A `clang-tidy` check is also available via
`make tidy`.

The tests can be compiled and executed with several compiler protections and
sanitizers.

The following compilers have been tested:

- `gcc` (10.2.0)
- `clang` (11.1.0)

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

#### benchmarks

Building the benchmakrs requires `Qt5Core` and `Qt5Test` (5.14.0) and can be
enabled with the `--enable-benchmarks` flag.

#### tools

Buildings auxiliary tools requires `Qt5Widgets`, `QtNetwork`, `Qt5Charts`, and
`rustc` (1.41.1) and can be enabled with the `--enable-tools` flag.
