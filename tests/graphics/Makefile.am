if ENABLE_TESTS
if ENABLE_LIBPNG
check_PROGRAMS += \
	%reldir%/texture
endif
endif

check_HEADERS += \
	%reldir%/texture_test.h

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
