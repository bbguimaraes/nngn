if ENABLE_TESTS
if ENABLE_FREETYPE2
check_PROGRAMS += \
	%reldir%/font
endif
check_PROGRAMS += \
	%reldir%/text
endif

check_HEADERS += \
	%reldir%/font_test.h \
	%reldir%/text_test.h

%canon_reldir%_font_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_font_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_font_LDADD = $(check_LDADD)
%canon_reldir%_font_SOURCES = \
	src/font/font.cpp \
	src/utils/log.cpp \
	%reldir%/font_test.cpp \
	%reldir%/font_test.moc.cpp

%canon_reldir%_text_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_text_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_text_LDADD = $(check_LDADD)
%canon_reldir%_text_SOURCES = \
	src/font/text.cpp \
	%reldir%/text_test.cpp \
	%reldir%/text_test.moc.cpp
