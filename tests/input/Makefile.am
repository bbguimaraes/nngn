if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/input
endif

check_HEADERS += \
	%reldir%/input_test.h

%canon_reldir%_input_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_input_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_input_LDADD = $(check_LDADD)
%canon_reldir%_input_SOURCES = \
	src/input/group.cpp \
	src/input/input.cpp \
	src/lua/alloc.cpp \
	src/lua/lua.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/utils/log.cpp \
	%reldir%/input_test.cpp \
	%reldir%/input_test.moc.cpp
