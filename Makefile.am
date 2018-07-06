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

noinst_HEADERS =

bin_PROGRAMS = @NNGN_BIN@
EXTRA_PROGRAMS = nngn$(EXEEXT) nngn.js

nngn_LDADD = $(DEPS_LIBS)
nngn_SOURCES =

nngn_js_SOURCES = $(nngn_SOURCES)
nngn_js_CPPFLAGS = $(AM_CPPFLAGS) $(DEPS_CFLAGS)
nngn_js_LDFLAGS = \
	$(nngn_js_CXXFLAGS) \
	-s ASSERTIONS=2 -s DEMANGLE_SUPPORT=1 -s ALLOW_MEMORY_GROWTH=1 \
	-s USE_GLFW=3 \
	--source-map-base /
nngn_js_LDADD = $(DEPS_LIBS)

TESTS = $(check_PROGRAMS)
check_PROGRAMS =

check_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS) -I@srcdir@/src
check_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
check_LDADD = $(DEPS_LIBS) $(TEST_DEPS_LIBS)
check_HEADERS =

EXTRA_DIST = \
	nngn.html \
	scripts

CLEANFILES =

%.moc.cpp: %.h
	moc -o $@ $^

.PHONY: \
	check-programs \
	clean-docs \
	docs \
	tidy

check-programs: $(TESTS)
clean-local: clean-docs
clean-docs:
	cd @srcdir@ && rm -rf docs/html docs/latex

docs:
	cd @srcdir@ && doxygen $(DOXYGEN_ARGS)
	sed -i 's|&lt;\(/\)\?tt&gt;|<\1code>|g' @srcdir@/docs/html/index.html

tidy: $(nngn_SOURCES:.cpp=.cpp.tidy)
%.tidy: %
	clang-tidy \
		--config "$$(<"$(srcdir)/scripts/clang-tidy.conf")" \
		$^ -- $(AM_CPPFLAGS) $(CPPFLAGS) -Isrc $(AM_CXXFLAGS) $(CXXFLAGS)
TIDY_IGNORE =
$(TIDY_IGNORE):
	@true

include $(srcdir)/src/Makefile.am
include $(srcdir)/tests/Makefile.am
