check_PROGRAMS += \
	%reldir%/camera \
	%reldir%/mat4 \
	%reldir%/math

check_HEADERS += \
	%reldir%/camera_test.h \
	%reldir%/mat4_test.h \
	%reldir%/math_test.h

%canon_reldir%_camera_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_camera_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_camera_LDADD = $(check_LDADD)
%canon_reldir%_camera_SOURCES = \
	src/math/camera.cpp \
	src/timing/timing.cpp \
	%reldir%/camera_test.cpp \
	%reldir%/camera_test.moc.cpp

%canon_reldir%_mat4_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_mat4_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_mat4_LDADD = $(check_LDADD)
%canon_reldir%_mat4_SOURCES = \
	%reldir%/mat4_test.cpp \
	%reldir%/mat4_test.moc.cpp

%canon_reldir%_math_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_math_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_math_LDADD = $(check_LDADD)
%canon_reldir%_math_SOURCES = \
	%reldir%/math_test.cpp \
	%reldir%/math_test.moc.cpp
