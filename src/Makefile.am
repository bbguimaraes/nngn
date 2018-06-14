noinst_HEADERS += \
	%reldir%/debug.h \
	%reldir%/entity.h \
	%reldir%/luastate.h

nngn_SOURCES += \
	%reldir%/entity.cpp \
	%reldir%/lua_entity.cpp \
	%reldir%/lua_luastate.cpp \
	%reldir%/luastate.cpp \
	%reldir%/main.cpp

EXTRA_DIST += \
	%reldir%/glsl \
	%reldir%/lson \
	%reldir%/lua

include %reldir%/glsl/Makefile.am
include %reldir%/graphics/Makefile.am
include %reldir%/input/Makefile.am
include %reldir%/math/Makefile.am
include %reldir%/os/Makefile.am
include %reldir%/render/Makefile.am
include %reldir%/timing/Makefile.am
include %reldir%/utils/Makefile.am
