if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/shaders
if ENABLE_LIBPNG
check_PROGRAMS += \
	%reldir%/texture
endif
endif

check_HEADERS += \
	%reldir%/shaders_test.h \
	%reldir%/texture_test.h

%canon_reldir%_shaders_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_shaders_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_shaders_LDADD = $(check_LDADD)
%canon_reldir%_shaders_SOURCES = \
	src/graphics/shaders_bin.cpp \
	src/graphics/shaders.cpp \
	%reldir%/shaders_test.cpp \
	%reldir%/shaders_test.moc.cpp

%canon_reldir%_texture_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_texture_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_texture_LDADD = $(check_LDADD)
%canon_reldir%_texture_SOURCES = \
	src/graphics/pseudo.cpp \
	src/graphics/texture.cpp \
	src/utils/log.cpp \
	%reldir%/texture_test.cpp \
	%reldir%/texture_test.moc.cpp

dist_check_DATA += \
	%reldir%/texture_test.png \
	%reldir%/texture_test_alpha.png
