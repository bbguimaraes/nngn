if ENABLE_TOOLS
bin_PROGRAMS += \
	%reldir%/bin/camera_server \
	%reldir%/bin/configure \
	%reldir%/bin/inspect \
	%reldir%/bin/launcher \
	%reldir%/bin/plot \
	%reldir%/bin/timeline \
	%reldir%/bin/websocket
endif

EXTRA_PROGRAMS += \
	%reldir%/bin/camera_server \
	%reldir%/bin/configure \
	%reldir%/bin/inspect \
	%reldir%/bin/launcher \
	%reldir%/bin/plot \
	%reldir%/bin/timeline \
	%reldir%/bin/websocket

noinst_HEADERS += \
	%reldir%/launcher/window.h \
	%reldir%/poller.h \
	%reldir%/reader.h \
	%reldir%/worker.h \
	%reldir%/configure/float_widget.h \
	%reldir%/configure/int_widget.h \
	%reldir%/configure/window.h \
	%reldir%/inspect/window.h \
	%reldir%/inspect/worker.h \
	%reldir%/plot/graph.h \
	%reldir%/plot/window.h \
	%reldir%/plot/worker.h \
	%reldir%/timeline/graph.h \
	%reldir%/timeline/worker.h

%canon_reldir%_bin_camera_server_SOURCES = tools/camera_server/main.rs

%canon_reldir%_bin_configure_CPPFLAGS = $(AM_CPPFLAGS) $(QT_DEPS_CFLAGS)
%canon_reldir%_bin_configure_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_bin_configure_LDADD = $(QT_DEPS_LIBS)
%canon_reldir%_bin_configure_SOURCES = \
	src/utils/log.cpp \
	%reldir%/configure/main.cpp \
	%reldir%/poller.cpp \
	%reldir%/reader.cpp \
	%reldir%/worker.cpp \
	%reldir%/worker.moc.cpp \
	%reldir%/configure/int_widget.cpp \
	%reldir%/configure/int_widget.moc.cpp \
	%reldir%/configure/float_widget.cpp \
	%reldir%/configure/float_widget.moc.cpp \
	%reldir%/configure/window.cpp \
	%reldir%/configure/window.moc.cpp

%canon_reldir%_bin_inspect_CPPFLAGS = $(AM_CPPFLAGS) $(QT_DEPS_CFLAGS)
%canon_reldir%_bin_inspect_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_bin_inspect_LDADD = $(QT_DEPS_LIBS)
%canon_reldir%_bin_inspect_SOURCES = \
	src/utils/log.cpp \
	%reldir%/inspect/main.cpp \
	%reldir%/poller.cpp \
	%reldir%/reader.cpp \
	%reldir%/worker.cpp \
	%reldir%/worker.moc.cpp \
	%reldir%/inspect/window.cpp \
	%reldir%/inspect/window.moc.cpp \
	%reldir%/inspect/worker.cpp \
	%reldir%/inspect/worker.moc.cpp

%canon_reldir%_bin_launcher_CPPFLAGS = $(AM_CPPFLAGS) $(QT_DEPS_CFLAGS)
%canon_reldir%_bin_launcher_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_bin_launcher_LDADD = $(QT_DEPS_LIBS)
%canon_reldir%_bin_launcher_SOURCES = \
	%reldir%/launcher/main.cpp \
	%reldir%/launcher/window.cpp

%canon_reldir%_bin_plot_CPPFLAGS = $(AM_CPPFLAGS) $(QT_DEPS_CFLAGS)
%canon_reldir%_bin_plot_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_bin_plot_LDADD = $(QT_DEPS_LIBS)
%canon_reldir%_bin_plot_SOURCES = \
	src/utils/log.cpp \
	%reldir%/plot/main.cpp \
	%reldir%/poller.cpp \
	%reldir%/reader.cpp \
	%reldir%/worker.cpp \
	%reldir%/worker.moc.cpp \
	%reldir%/plot/window.cpp \
	%reldir%/plot/window.moc.cpp \
	%reldir%/plot/worker.cpp \
	%reldir%/plot/worker.moc.cpp

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

%canon_reldir%_bin_websocket_SOURCES = tools/websocket.rs

%reldir%/bin/websocket$(EXEEXT): %reldir%/websocket.rs
	rustc -o $@ $^
%reldir%/bin/camera_server$(EXEEXT): %reldir%/camera_server/main.rs
	rustc -o $@ $^

CLEANFILES += \
	%reldir%/*.moc.cpp \
	%reldir%/*/*.moc.cpp \
	%reldir%/bin/camera_server \
	%reldir%/bin/websocket
