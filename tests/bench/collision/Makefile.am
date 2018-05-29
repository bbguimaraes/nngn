EXTRA_PROGRAMS += \
	%reldir%/native

if ENABLE_BENCHMARKS
bin_PROGRAMS += \
	%reldir%/native
endif

check_HEADERS += \
	%reldir%/collision.h \
	%reldir%/native.h

%canon_reldir%_native_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS)
%canon_reldir%_native_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_native_LDADD = $(nngn_LDADD) $(TEST_DEPS_LIBS)
%canon_reldir%_native_SOURCES = \
	src/entity.cpp \
	src/collision/colliders.cpp \
	src/collision/collision.cpp \
	src/collision/native.cpp \
	src/lua/alloc.cpp \
	src/lua/lua.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/lua/user.cpp \
	src/math/camera.cpp \
	src/render/animation.cpp \
	src/render/light.cpp \
	src/render/sun.cpp \
	src/timing/profile.cpp \
	src/timing/stats.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	%reldir%/native.cpp \
	%reldir%/native.moc.cpp \
	%reldir%/collision.cpp \
	%reldir%/collision.moc.cpp
