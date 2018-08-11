if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/animation \
	%reldir%/render
endif

check_HEADERS += \
	%reldir%/animation_test.h \
	%reldir%/render_test.h

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

%canon_reldir%_render_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_render_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_render_LDADD = $(check_LDADD)
%canon_reldir%_render_SOURCES = \
	%reldir%/render_test.cpp \
	%reldir%/render_test.moc.cpp
