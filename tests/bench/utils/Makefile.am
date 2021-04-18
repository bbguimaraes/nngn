EXTRA_PROGRAMS += \
	%reldir%/flag_array

if ENABLE_BENCHMARKS
bin_PROGRAMS += \
	%reldir%/flag_array
endif

check_HEADERS += \
	%reldir%/flag_array.h

%canon_reldir%_flag_array_CPPFLAGS = $(AM_CPPFLAGS) $(TEST_DEPS_CFLAGS)
%canon_reldir%_flag_array_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
%canon_reldir%_flag_array_LDADD = $(nngn_LDADD) $(TEST_DEPS_LIBS)
%canon_reldir%_flag_array_SOURCES = \
	%reldir%/flag_array.cpp \
	%reldir%/flag_array.moc.cpp
