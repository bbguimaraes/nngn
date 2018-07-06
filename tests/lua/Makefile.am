if ENABLE_TESTS
TESTS += \
	%reldir%/run.sh
check_PROGRAMS += \
	%reldir%/state \
	%reldir%/table
endif

check_HEADERS += \
	%reldir%/state_test.h \
	%reldir%/table_test.h

%canon_reldir%_state_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_state_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_state_LDADD = $(check_LDADD)
%canon_reldir%_state_SOURCES = \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/utils/log.cpp \
	%reldir%/state_test.cpp \
	%reldir%/state_test.moc.cpp

%canon_reldir%_table_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_table_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_table_LDADD = $(check_LDADD)
%canon_reldir%_table_SOURCES = \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/utils/log.cpp \
	%reldir%/table_test.cpp \
	%reldir%/table_test.moc.cpp

dist_check_DATA += \
	%reldir%/common.lua \
	%reldir%/run.sh \
	%reldir%/unit.lua \
	%reldir%/utils.lua
