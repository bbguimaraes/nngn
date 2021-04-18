EXTRA_PROGRAMS += \
	%reldir%/entity

if ENABLE_BENCHMARKS
bin_PROGRAMS += \
	%reldir%/entity
endif

check_HEADERS += \
	%reldir%/entity.h

%canon_reldir%_entity_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS)
%canon_reldir%_entity_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_entity_LDADD = $(nngn_LDADD) $(TEST_DEPS_LIBS)
%canon_reldir%_entity_SOURCES = \
	src/entity.cpp \
	src/lua/user.cpp \
	src/math/camera.cpp \
	src/render/animation.cpp \
	src/render/light.cpp \
	src/render/sun.cpp \
	src/timing/profile.cpp \
	src/timing/stats.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	%reldir%/entity.cpp \
	%reldir%/entity.moc.cpp

include %reldir%/collision/Makefile.am
include %reldir%/lua/Makefile.am
include %reldir%/utils/Makefile.am
