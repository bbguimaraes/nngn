if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/animation \
	%reldir%/light \
	%reldir%/map \
	%reldir%/render \
	%reldir%/sun
endif

check_HEADERS += \
	%reldir%/animation_test.h \
	%reldir%/light_test.h \
	%reldir%/map_test.h \
	%reldir%/render_test.h \
	%reldir%/sun_test.h

%canon_reldir%_animation_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_animation_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_animation_LDADD = $(check_LDADD)
%canon_reldir%_animation_SOURCES = \
	src/lua/alloc.cpp \
	src/lua/lua.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/lua/user.cpp \
	src/render/animation.cpp \
	src/timing/profile.cpp \
	src/timing/stats.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	%reldir%/animation_test.cpp \
	%reldir%/animation_test.moc.cpp

%canon_reldir%_light_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_light_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_light_LDADD = $(check_LDADD)
%canon_reldir%_light_SOURCES = \
	src/entity.cpp \
	src/lua/user.cpp \
	src/math/camera.cpp \
	src/math/math.cpp \
	src/render/animation.cpp \
	src/render/light.cpp \
	src/render/sun.cpp \
	src/timing/profile.cpp \
	src/timing/stats.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	%reldir%/light_test.cpp \
	%reldir%/light_test.moc.cpp

%canon_reldir%_map_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_map_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_map_LDADD = $(check_LDADD)
%canon_reldir%_map_SOURCES = \
	src/graphics/pseudo.cpp \
	src/graphics/texture.cpp \
	src/lua/alloc.cpp \
	src/lua/lua.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/render/map.cpp \
	src/utils/log.cpp \
	%reldir%/map_test.cpp \
	%reldir%/map_test.moc.cpp

%canon_reldir%_render_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_render_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_render_LDADD = $(check_LDADD)
%canon_reldir%_render_SOURCES = \
	%reldir%/render_test.cpp \
	%reldir%/render_test.moc.cpp

%canon_reldir%_sun_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_sun_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_sun_LDADD = $(check_LDADD)
%canon_reldir%_sun_SOURCES = \
	src/render/sun.cpp \
	%reldir%/sun_test.cpp \
	%reldir%/sun_test.moc.cpp
