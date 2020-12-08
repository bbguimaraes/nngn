if ENABLE_TESTS
if ENABLE_OPENCL
check_PROGRAMS += \
	%reldir%/compute
endif
check_PROGRAMS += \
	%reldir%/native
endif

check_HEADERS += \
	%reldir%/collision_test.h \
	%reldir%/compute_test.h \
	%reldir%/native_test.h

%canon_reldir%_compute_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_compute_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_compute_LDADD = $(check_LDADD)
%canon_reldir%_compute_SOURCES = \
	src/entity.cpp \
	src/collision/colliders.cpp \
	src/collision/collision.cpp \
	src/collision/compute.cpp \
	src/compute/compute.cpp \
	src/compute/opencl.cpp \
	src/compute/pseudo.cpp \
	src/lua/alloc.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/math/camera.cpp \
	src/os/platform.cpp \
	src/render/animation.cpp \
	src/render/light.cpp \
	src/render/sun.cpp \
	src/timing/profile.cpp \
	src/timing/stats.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	src/utils/utils.cpp \
	%reldir%/collision_test.cpp \
	%reldir%/collision_test.moc.cpp \
	%reldir%/compute_test.cpp \
	%reldir%/compute_test.moc.cpp

%canon_reldir%_native_CPPFLAGS = $(check_CPPFLAGS)
%canon_reldir%_native_CXXFLAGS = $(check_CXXFLAGS)
%canon_reldir%_native_LDADD = $(check_LDADD)
%canon_reldir%_native_SOURCES = \
	src/entity.cpp \
	src/collision/colliders.cpp \
	src/collision/collision.cpp \
	src/collision/native.cpp \
	src/lua/alloc.cpp \
	src/lua/state.cpp \
	src/lua/traceback.cpp \
	src/math/camera.cpp \
	src/render/animation.cpp \
	src/render/light.cpp \
	src/render/sun.cpp \
	src/timing/profile.cpp \
	src/timing/stats.cpp \
	src/timing/timing.cpp \
	src/utils/log.cpp \
	%reldir%/collision_test.cpp \
	%reldir%/collision_test.moc.cpp \
	%reldir%/native_test.cpp \
	%reldir%/native_test.moc.cpp
