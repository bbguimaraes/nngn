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

TESTS = $(check_PROGRAMS)

if ENABLE_TESTS
check_PROGRAMS = tests/cmd/command tests/conf/conf tests/ui/panel
endif

check_HEADERS = tests/cmd/command.hpp tests/conf/conf.hpp tests/ui/panel.hpp

check_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS) -I@srcdir@/src
check_CXXFLAGS = $(AM_CXXFLAGS) -fPIC -fsanitize=address,undefined
check_LDADD = $(DEPS_LIBS) $(TEST_DEPS_LIBS)

tests_cmd_command_CPPFLAGS = $(check_CPPFLAGS)
tests_cmd_command_CXXFLAGS = $(check_CXXFLAGS)
tests_cmd_command_LDADD = $(check_LDADD)
tests_cmd_command_SOURCES = tests/cmd/command.cpp tests/cmd/command.moc.cpp

tests_conf_conf_CPPFLAGS = $(check_CPPFLAGS)
tests_conf_conf_CXXFLAGS = $(check_CXXFLAGS)
tests_conf_conf_LDADD = $(check_LDADD)
tests_conf_conf_SOURCES = \
	src/conf/conf.cpp \
	tests/conf/conf.cpp tests/conf/conf.moc.cpp

tests_ui_panel_CPPFLAGS = $(check_CPPFLAGS) -DQT_WIDGETS_LIB
tests_ui_panel_CXXFLAGS = $(check_CXXFLAGS)
tests_ui_panel_LDADD = $(check_LDADD)
tests_ui_panel_SOURCES = \
	src/ui/panel.cpp src/ui/panel.moc.cpp \
	tests/ui/panel.cpp tests/ui/panel.moc.cpp

%.moc.cpp: %.hpp
	moc -o $@ $^
