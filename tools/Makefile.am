if ENABLE_TOOLS
bin_PROGRAMS += \
	%reldir%/bin/launcher
endif

EXTRA_PROGRAMS += \
	%reldir%/bin/launcher

noinst_HEADERS += \
	%reldir%/launcher/window.h

%canon_reldir%_bin_launcher_CPPFLAGS = $(AM_CPPFLAGS) $(QT_DEPS_CFLAGS)
%canon_reldir%_bin_launcher_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_bin_launcher_LDADD = $(QT_DEPS_LIBS)
%canon_reldir%_bin_launcher_SOURCES = \
	%reldir%/launcher/main.cpp \
	%reldir%/launcher/window.cpp

CLEANFILES += \
	%reldir%/*.moc.cpp \
	%reldir%/*/*.moc.cpp
