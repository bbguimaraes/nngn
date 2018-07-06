if ENABLE_TESTS
TESTS += \
	%reldir%/run.sh
check_PROGRAMS += \
	%reldir%/function \
	%reldir%/register \
	%reldir%/stack \
	%reldir%/state \
	%reldir%/table \
	%reldir%/utils
endif

check_HEADERS += \
	%reldir%/function_test.h \
	%reldir%/register_test.h \
	%reldir%/stack_test.h \
	%reldir%/state_test.h \
	%reldir%/table_test.h \
	%reldir%/utils_test.h

%canon_reldir%_function_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_function_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_function_LDADD = $(check_LDADD)
%canon_reldir%_function_SOURCES = \
	src/lua/function.cpp \
	src/lua/lua.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/utils/log.cpp \
	%reldir%/function_test.cpp \
	%reldir%/function_test.moc.cpp

%canon_reldir%_register_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_register_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_register_LDADD = $(check_LDADD)
%canon_reldir%_register_SOURCES = \
	src/lua/lua.cpp \
	src/lua/register.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/lua/user.cpp \
	src/utils/log.cpp \
	%reldir%/register_test.cpp \
	%reldir%/register_test.moc.cpp

%canon_reldir%_stack_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_stack_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_stack_LDADD = $(check_LDADD)
%canon_reldir%_stack_SOURCES = \
	src/lua/get.cpp \
	src/lua/lua.cpp \
	src/lua/push.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/lua/user.cpp \
	src/utils/log.cpp \
	%reldir%/stack_test.cpp \
	%reldir%/stack_test.moc.cpp

%canon_reldir%_state_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_state_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_state_LDADD = $(check_LDADD)
%canon_reldir%_state_SOURCES = \
	src/lua/lua.cpp \
	src/lua/register.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/lua/user.cpp \
	src/utils/log.cpp \
	%reldir%/state_test.cpp \
	%reldir%/state_test.moc.cpp

%canon_reldir%_table_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_table_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_table_LDADD = $(check_LDADD)
%canon_reldir%_table_SOURCES = \
	src/lua/iter.cpp \
	src/lua/lua.cpp \
	src/lua/register.cpp \
	src/lua/state.cpp \
	src/lua/table.cpp \
	src/lua/traceback.cpp \
	src/lua/user.cpp \
	src/utils/log.cpp \
	%reldir%/table_test.cpp \
	%reldir%/table_test.moc.cpp

%canon_reldir%_utils_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_utils_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_utils_LDADD = $(check_LDADD)
%canon_reldir%_utils_SOURCES = \
	src/lua/lua.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/lua/utils.cpp \
	src/utils/log.cpp \
	%reldir%/utils_test.cpp \
	%reldir%/utils_test.moc.cpp

LUA_TESTS = \
	%reldir%/common.lua \
	%reldir%/math.lua \
	%reldir%/run.sh \
	%reldir%/serial.lua \
	%reldir%/unit.lua \
	%reldir%/utils.lua

LUA_FILES += $(LUA_TESTS)
dist_check_DATA += $(LUA_TESTS)
