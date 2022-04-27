if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/block \
	%reldir%/realloc \
	%reldir%/tagging \
	%reldir%/tracking
endif

check_HEADERS += \
	%reldir%/block_test.h \
	%reldir%/realloc_test.h \
	%reldir%/tagging_test.h \
	%reldir%/tracking_test.h

%canon_reldir%_block_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_block_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_block_LDADD = $(check_LDADD)
%canon_reldir%_block_SOURCES = \
	%reldir%/block_test.cpp \
	%reldir%/block_test.moc.cpp

%canon_reldir%_realloc_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_realloc_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_realloc_LDADD = $(check_LDADD)
%canon_reldir%_realloc_SOURCES = \
	%reldir%/realloc_test.cpp \
	%reldir%/realloc_test.moc.cpp

%canon_reldir%_tagging_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_tagging_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_tagging_LDADD = $(check_LDADD)
%canon_reldir%_tagging_SOURCES = \
	%reldir%/tagging_test.cpp \
	%reldir%/tagging_test.moc.cpp

%canon_reldir%_tracking_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_tracking_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_tracking_LDADD = $(check_LDADD)
%canon_reldir%_tracking_SOURCES = \
	%reldir%/tracking_test.cpp \
	%reldir%/tracking_test.moc.cpp
