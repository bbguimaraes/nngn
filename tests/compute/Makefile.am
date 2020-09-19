if ENABLE_TESTS
check_PROGRAMS += \
	%reldir%/thread_pool
if ENABLE_OPENCL
check_PROGRAMS += \
	%reldir%/compute
endif
endif

check_HEADERS += \
	%reldir%/compute_test.h \
	%reldir%/thread_pool_test.h

%canon_reldir%_thread_pool_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_thread_pool_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_thread_pool_LDADD = $(check_LDADD)
%canon_reldir%_thread_pool_SOURCES = \
	%reldir%/thread_pool_test.cpp \
	%reldir%/thread_pool_test.moc.cpp

%canon_reldir%_compute_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_compute_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_compute_LDADD = $(check_LDADD)
%canon_reldir%_compute_SOURCES = \
	src/compute/compute.cpp \
	src/compute/pseudo.cpp \
	src/compute/opencl.cpp \
	src/utils/log.cpp \
	%reldir%/compute_test.cpp \
	%reldir%/compute_test.moc.cpp
