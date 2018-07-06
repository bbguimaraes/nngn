if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/flags \
	%reldir%/log \
	%reldir%/scoped
endif

check_HEADERS += \
	%reldir%/flags_test.h \
	%reldir%/log_test.h \
	%reldir%/scoped_test.h

%canon_reldir%_flags_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_flags_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_flags_LDADD = $(check_LDADD)
%canon_reldir%_flags_SOURCES = \
	%reldir%/flags_test.cpp \
	%reldir%/flags_test.moc.cpp

%canon_reldir%_log_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_log_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_log_LDADD = $(check_LDADD)
%canon_reldir%_log_SOURCES = \
	src/utils/log.cpp \
	%reldir%/log_test.cpp \
	%reldir%/log_test.moc.cpp

%canon_reldir%_scoped_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_scoped_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_scoped_LDADD = $(check_LDADD)
%canon_reldir%_scoped_SOURCES = \
	%reldir%/scoped_test.cpp \
	%reldir%/scoped_test.moc.cpp
