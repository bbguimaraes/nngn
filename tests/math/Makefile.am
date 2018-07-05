if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/camera \
	%reldir%/mat \
	%reldir%/math
endif

check_HEADERS += \
	%reldir%/camera_test.h \
	%reldir%/mat_test.h \
	%reldir%/math_test.h

%canon_reldir%_camera_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_camera_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_camera_LDADD = $(check_LDADD)
%canon_reldir%_camera_SOURCES = \
	src/math/camera.cpp \
	src/timing/timing.cpp \
	%reldir%/camera_test.cpp \
	%reldir%/camera_test.moc.cpp

%canon_reldir%_mat_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_mat_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_mat_LDADD = $(check_LDADD)
%canon_reldir%_mat_SOURCES = \
	%reldir%/mat_test.cpp \
	%reldir%/mat_test.moc.cpp

%canon_reldir%_math_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_math_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_math_LDADD = $(check_LDADD)
%canon_reldir%_math_SOURCES = \
	%reldir%/math_test.cpp \
	%reldir%/math_test.moc.cpp
