if ENABLE_TOOLS
bin_PROGRAMS += \
	%reldir%/bin/launcher \
	%reldir%/bin/timeline
endif

EXTRA_PROGRAMS += \
	%reldir%/bin/launcher \
	%reldir%/bin/timeline

noinst_HEADERS += \
	%reldir%/launcher/window.h \
	%reldir%/poller.h \
	%reldir%/reader.h \
	%reldir%/worker.h \
	%reldir%/timeline/graph.h \
	%reldir%/timeline/worker.h

%canon_reldir%_bin_launcher_CPPFLAGS = $(AM_CPPFLAGS) $(QT_DEPS_CFLAGS)
%canon_reldir%_bin_launcher_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_bin_launcher_LDADD = $(QT_DEPS_LIBS)
%canon_reldir%_bin_launcher_SOURCES = \
	%reldir%/launcher/main.cpp \
	%reldir%/launcher/window.cpp

%canon_reldir%_bin_timeline_CPPFLAGS = $(AM_CPPFLAGS) $(QT_DEPS_CFLAGS)
%canon_reldir%_bin_timeline_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_bin_timeline_LDADD = $(QT_DEPS_LIBS)
%canon_reldir%_bin_timeline_SOURCES = \
	src/utils/log.cpp \
	%reldir%/timeline/main.cpp \
	%reldir%/poller.cpp \
	%reldir%/reader.cpp \
	%reldir%/worker.cpp \
	%reldir%/worker.moc.cpp \
	%reldir%/timeline/graph.cpp \
	%reldir%/timeline/graph.moc.cpp \
	%reldir%/timeline/worker.cpp \
	%reldir%/timeline/worker.moc.cpp

CLEANFILES += \
	%reldir%/*.moc.cpp \
	%reldir%/*/*.moc.cpp
