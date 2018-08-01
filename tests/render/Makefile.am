if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/render
endif

check_HEADERS += \
	%reldir%/render_test.h

%canon_reldir%_render_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_render_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_render_LDADD = $(check_LDADD)
%canon_reldir%_render_SOURCES = \
	%reldir%/render_test.cpp \
	%reldir%/render_test.moc.cpp
