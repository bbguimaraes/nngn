if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/entity
endif

check_HEADERS += \
	%reldir%/entity_test.h \
	%reldir%/registry.h \
	%reldir%/tests.h

%canon_reldir%_entity_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_entity_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_entity_LDADD = $(check_LDADD)
%canon_reldir%_entity_SOURCES = \
	src/entity.cpp \
	src/timing/profile.cpp \
	src/timing/stats.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	%reldir%/entity_test.cpp \
	%reldir%/entity_test.moc.cpp

CLEANFILES += \
	%reldir%/*.moc.cpp \
	%reldir%/*/*.moc.cpp

include %reldir%/bench/Makefile.am
include %reldir%/graphics/Makefile.am
include %reldir%/input/Makefile.am
include %reldir%/lua/Makefile.am
include %reldir%/math/Makefile.am
include %reldir%/render/Makefile.am
include %reldir%/timing/Makefile.am
include %reldir%/utils/Makefile.am
