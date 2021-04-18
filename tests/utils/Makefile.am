if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/flag_array \
	%reldir%/flags \
	%reldir%/log \
	%reldir%/scoped \
	%reldir%/static_vector \
	%reldir%/utils \
	%reldir%/types
endif

check_HEADERS += \
	%reldir%/flag_array_test.h \
	%reldir%/flags_test.h \
	%reldir%/log_test.h \
	%reldir%/scoped_test.h \
	%reldir%/utils_test.h \
	%reldir%/types_test.h

dist_check_DATA += \
	%reldir%/static_vector_test.h \
	%reldir%/utils_test.txt

%canon_reldir%_flag_array_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_flag_array_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_flag_array_LDADD = $(check_LDADD)
%canon_reldir%_flag_array_SOURCES = \
	%reldir%/flag_array_test.cpp \
	%reldir%/flag_array_test.moc.cpp

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

%canon_reldir%_static_vector_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_static_vector_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_static_vector_LDADD = $(check_LDADD)
%canon_reldir%_static_vector_SOURCES = \
	%reldir%/static_vector_test.cpp \
	%reldir%/static_vector_test.moc.cpp

%canon_reldir%_utils_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_utils_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_utils_LDADD = $(check_LDADD)
%canon_reldir%_utils_SOURCES = \
	src/utils/log.cpp \
	src/utils/utils.cpp \
	%reldir%/utils_test.cpp \
	%reldir%/utils_test.moc.cpp

%canon_reldir%_types_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_types_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_types_LDADD = $(check_LDADD)
%canon_reldir%_types_SOURCES = \
	%reldir%/types_test.cpp \
	%reldir%/types_test.moc.cpp

include %reldir%/alloc/Makefile.am
