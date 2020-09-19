AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = $(DEPS_CFLAGS) -I@srcdir@/src
AM_CXXFLAGS = \
	-std=c++20 -fno-exceptions -fno-rtti -fstrict-aliasing \
	-O3 -DNDEBUG -DSOL_NO_EXCEPTIONS -DCL_TARGET_OPENCL_VERSION=120 \
	-fno-omit-frame-pointer \
	-Werror -Wall -Wextra -Wpedantic -pedantic-errors \
	-Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy \
	-Wdisabled-optimization -Wdouble-promotion -Weffc++ -Wformat=2 -Wimport \
	-Winit-self -Wmissing-declarations -Wmissing-format-attribute \
	-Wmissing-include-dirs -Wmissing-noreturn -Wold-style-cast \
	-Woverloaded-virtual -Wpointer-arith -Wredundant-decls -Wshadow \
	-Wsign-conversion -Wsign-promo -Wstack-protector -Wstrict-aliasing \
	-Wstrict-overflow=2 -Wswitch-enum -Wundef -Wunreachable-code -Wunused \
	-Wwrite-strings -Wno-missing-field-initializers
AM_LDFLAGS = -fuse-ld=gold -Wl,--threads,--preread-archive-symbols

BUILT_SOURCES =
noinst_HEADERS =

bin_PROGRAMS = @NNGN_BIN@ demos/ray
EXTRA_PROGRAMS = nngn nngn.js

nngn_LDADD = $(DEPS_LIBS)
nngn_SOURCES =

nngn_js_SOURCES = $(nngn_SOURCES)
nngn_js_CPPFLAGS = $(AM_CPPFLAGS) $(DEPS_CFLAGS)
nngn_js_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-s ASSERTIONS=2 -s DEMANGLE_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=1 \
	-s EXPORTED_FUNCTIONS='["_main","_lua"]' \
	-s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
	-s USE_GLFW=3 -s FULL_ES2=1 -s FULL_ES3=1 \
	-s USE_WEBGL2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 \
	-s USE_LIBPNG=1 -s USE_FREETYPE=1
nngn_js_LDFLAGS = \
	$(nngn_js_CXXFLAGS) \
	--source-map-base / \
	--embed-file $(srcdir)/src/cl/@src/cl \
	--embed-file $(srcdir)/src/lua/@src/lua \
	--embed-file $(srcdir)/src/lson/@src/lson \
	--embed-file $(srcdir)/demos/@demos \
	--embed-file $(srcdir)/img/@img \
	--embed-file $(srcdir)/maps/@maps \
	--embed-file /usr/share/fonts/TTF/DejaVuSans.ttf@DejaVuSans.ttf
nngn_js_LDADD = $(DEPS_LIBS)

demos_ray_CXXFLAGS = $(AM_CXXFLAGS)
demos_ray_LDADD = $(DEPS_LIBS) -lpthread
demos_ray_SOURCES = \
	demos/ray.cpp \
	src/luastate.cpp \
	src/math/camera.cpp src/math/math.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	src/ray/dielectric.cpp src/ray/lambertian.cpp src/ray/metal.cpp \
	src/ray/sphere.cpp src/ray/tracer.cpp src/ray/lua_tracer.cpp

noinst_HEADERS += \
	src/ray/dielectric.h src/ray/lambertian.h src/ray/material.h \
	src/ray/metal.h src/ray/ray.h src/ray/sphere.h src/ray/tracer.h

nngn_SOURCES += \
	src/ray/dielectric.cpp src/ray/lambertian.cpp src/ray/metal.cpp \
	src/ray/sphere.cpp src/ray/tracer.cpp src/ray/lua_tracer.cpp


TESTS = $(check_PROGRAMS)
check_PROGRAMS =

check_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS) -I@srcdir@/src
check_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
check_LDADD = $(DEPS_LIBS) $(TEST_DEPS_LIBS)
check_HEADERS =
dist_check_DATA =

EXTRA_DIST = \
	img \
	maps \
	nngn.html \
	scripts

CLEANFILES =

%.moc.cpp: %.h
	moc -o $@ $^

.PHONY: \
	check-programs \
	clean-docs \
	docs \
	shaders \
	tidy \
	tools

check-programs: $(TESTS)
clean-local: clean-docs
clean-docs:
	cd @srcdir@ && rm -rf docs/html docs/latex

docs:
	cd @srcdir@ && doxygen

tidy: $(nngn_SOURCES:.cpp=.cpp.tidy)
%.tidy: %
	clang-tidy \
		--config "$$(<"$(srcdir)/scripts/clang-tidy.conf")" \
		$^ -- $(AM_CPPFLAGS) $(CPPFLAGS) -Isrc $(AM_CXXFLAGS) $(CXXFLAGS)
TIDY_IGNORE =
$(TIDY_IGNORE):
	@true

shaders: $(SHADERS)
tools: $(TOOLS_BIN)

include $(srcdir)/src/Makefile.am
include $(srcdir)/tests/Makefile.am
include $(srcdir)/tools/Makefile.am
