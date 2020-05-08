if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/fps \
	%reldir%/schedule
endif

check_HEADERS += \
	%reldir%/fps_test.h \
	%reldir%/schedule_test.h

%canon_reldir%_fps_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_fps_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_fps_LDADD = $(check_LDADD)
%canon_reldir%_fps_SOURCES = \
	src/timing/fps.cpp \
	%reldir%/fps_test.cpp \
	%reldir%/fps_test.moc.cpp

%canon_reldir%_schedule_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_schedule_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_schedule_LDADD = $(check_LDADD)
%canon_reldir%_schedule_SOURCES = \
	src/timing/schedule.cpp \
	src/utils/log.cpp \
	%reldir%/schedule_test.cpp \
	%reldir%/schedule_test.moc.cpp
