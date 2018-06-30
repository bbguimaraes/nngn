AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = $(DEPS_CFLAGS) -I@srcdir@/src
AM_CXXFLAGS = \
	-std=c++20 -fno-exceptions -fno-rtti -fstrict-aliasing \
	-O3 -DNDEBUG -fno-omit-frame-pointer \
	-Werror -Wall -Wextra -Wpedantic -pedantic-errors \
	-Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy \
	-Wdisabled-optimization -Wdouble-promotion -Weffc++ -Wformat=2 -Wimport \
	-Winit-self -Wmissing-declarations -Wmissing-format-attribute \
	-Wmissing-include-dirs -Wmissing-noreturn -Wold-style-cast \
	-Woverloaded-virtual -Wpointer-arith -Wredundant-decls -Wshadow \
	-Wsign-conversion -Wsign-promo -Wstack-protector -Wstrict-aliasing \
	-Wstrict-overflow=2 -Wswitch-enum -Wundef -Wunreachable-code -Wunused \
	-Wwrite-strings -Wno-nonnull -Wno-missing-field-initializers
# It's C++ 20, Clang.
AM_CXXFLAGS += -Wno-gnu-zero-variadic-macro-arguments
AM_CXXFLAGS += -Wno-string-conversion

BUILT_SOURCES =
noinst_HEADERS =

bin_PROGRAMS = @NNGN_BIN@
EXTRA_PROGRAMS = nngn$(EXEEXT) nngn.js

nngn_LDADD = $(DEPS_LIBS)
nngn_SOURCES =

nngn_js_SOURCES = $(nngn_SOURCES)
nngn_js_CPPFLAGS = $(AM_CPPFLAGS) $(DEPS_CFLAGS)
nngn_js_CXXFLAGS = \
	$(AM_CXXFLAGS) \
	-s USE_LIBPNG=1 -s USE_FREETYPE=1
nngn_js_LDFLAGS = \
	$(nngn_js_CXXFLAGS) \
	-s ASSERTIONS=2 -s DEMANGLE_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=1 \
	-s EXPORTED_FUNCTIONS='["_main","_lua"]' \
	-s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
	-s USE_GLFW=3 -s FULL_ES2=1 -s FULL_ES3=1 \
	-s USE_WEBGL2=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2 \
	--source-map-base / \
	--embed-file $(srcdir)/src/lua/@src/lua \
	--embed-file $(srcdir)/src/lson/@src/lson \
	--embed-file $(srcdir)/demos/@demos \
	--embed-file $(srcdir)/img/@img \
	--embed-file /usr/share/fonts/TTF/DejaVuSans.ttf@DejaVuSans.ttf
nngn_js_LDADD = $(DEPS_LIBS)

TESTS = $(check_PROGRAMS)
check_PROGRAMS =

check_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS) -I@srcdir@/src
check_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
check_LDADD = $(DEPS_LIBS) $(TEST_DEPS_LIBS)
check_HEADERS =
dist_check_DATA =

LUA_FILES =
EXTRA_DIST = \
	img \
	nngn.html \
	scripts \
	$(LUA_FILES)

CLEANFILES =

%.moc.cpp: %.h
	moc -o $@ $^

.PHONY: \
	check-lua \
	check-programs \
	clean-docs \
	docs \
	shaders \
	tidy \
	tools

check: check-lua
check-lua: $(LUA_FILES:.lua=.lua.check)
%.lua.check: %.lua
	luac -p $^

check-programs: $(TESTS)
clean-local: clean-docs
clean-docs:
	cd @srcdir@ && rm -rf docs/html docs/latex

docs:
	cd @srcdir@ && doxygen $(DOXYGEN_ARGS)
	sed -i 's|&lt;\(/\)\?tt&gt;|<\1code>|g' @srcdir@/docs/html/index.html

tidy: $(BUILT_SOURCES) $(nngn_SOURCES:.cpp=.cpp.tidy)
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
