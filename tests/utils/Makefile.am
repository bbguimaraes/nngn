if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/log
endif

check_HEADERS += \
	%reldir%/log_test.h

%canon_reldir%_log_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_log_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_log_LDADD = $(check_LDADD)
%canon_reldir%_log_SOURCES = \
	src/utils/log.cpp \
	%reldir%/log_test.cpp \
	%reldir%/log_test.moc.cpp
