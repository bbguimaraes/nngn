if ENABLE_TESTS
if ENABLE_FREETYPE2
check_PROGRAMS += \
	%reldir%/font
endif
check_PROGRAMS += \
	%reldir%/text \
	%reldir%/textbox
endif

check_HEADERS += \
	%reldir%/font_test.h \
	%reldir%/text_test.h \
	%reldir%/textbox_test.h

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

%canon_reldir%_textbox_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_textbox_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_textbox_LDADD = $(check_LDADD)
%canon_reldir%_textbox_SOURCES = \
	src/font/font.cpp \
	src/font/textbox.cpp \
	src/font/text.cpp \
	src/utils/log.cpp \
	%reldir%/textbox_test.cpp \
	%reldir%/textbox_test.moc.cpp
