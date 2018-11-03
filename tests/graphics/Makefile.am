if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/shaders
endif

check_HEADERS += \
	%reldir%/shaders_test.h

%canon_reldir%_shaders_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_shaders_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_shaders_LDADD = $(check_LDADD)
%canon_reldir%_shaders_SOURCES = \
	src/graphics/shaders_bin.cpp \
	src/graphics/shaders.cpp \
	%reldir%/shaders_test.cpp \
	%reldir%/shaders_test.moc.cpp
