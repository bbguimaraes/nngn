if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/luastate
endif

check_HEADERS += \
	%reldir%/luastate_test.h \
	%reldir%/registry.h

%canon_reldir%_luastate_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_luastate_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_luastate_LDADD = $(check_LDADD)
%canon_reldir%_luastate_SOURCES = \
	src/luastate.cpp \
	src/utils/log.cpp \
	%reldir%/luastate_test.cpp \
	%reldir%/luastate_test.moc.cpp

CLEANFILES += \
	%reldir%/*.moc.cpp \
	%reldir%/*/*.moc.cpp

include %reldir%/lua/Makefile.am
include %reldir%/math/Makefile.am
include %reldir%/timing/Makefile.am
include %reldir%/utils/Makefile.am
