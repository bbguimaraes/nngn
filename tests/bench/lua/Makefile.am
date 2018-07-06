EXTRA_PROGRAMS += \
	%reldir%/lua

if ENABLE_BENCHMARKS
bin_PROGRAMS += \
	%reldir%/lua
endif

check_HEADERS += \
	%reldir%/lua.h

%canon_reldir%_lua_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS)
%canon_reldir%_lua_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_lua_LDADD = $(nngn_LDADD) $(TEST_DEPS_LIBS)
%canon_reldir%_lua_SOURCES = \
	%reldir%/lua.cpp \
	%reldir%/lua.moc.cpp
