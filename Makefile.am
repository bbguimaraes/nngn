AUTOMAKE_OPTIONS = subdir-objects

bin_PROGRAMS = impero

AM_CXXFLAGS = -std=c++2a -O2 -Wall -Wextra -pedantic

dist_doc_DATA = \
    README.md

EXTRA_DIST = \
    CMakeLists.txt

noinst_HEADERS = \
    src/cmd/command.hpp \
    src/conf/conf.hpp \
    src/ui/widget.hpp \
    src/ui/utils.hpp \
    src/ui/panel.hpp

impero_CPPFLAGS= $(AM_CXXFLAGS) $(DEPS_CFLAGS)
impero_CXXFLAGS= $(AM_CPPFLAGS) -fPIC
impero_LDFLAGS= $(AM_LDFLAGS) $(DEPS_LIBS)
impero_SOURCES = \
    src/conf/conf.cpp \
    src/ui/panel.cpp \
    src/ui/panel.moc.cpp \
    src/ui/widget.cpp \
    src/ui/widget.moc.cpp \
    src/main.cpp

%.moc.cpp: %.hpp
	moc -o $@ $^
