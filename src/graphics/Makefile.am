noinst_HEADERS += \
	%reldir%/glfw.h \
	%reldir%/graphics.h \
	%reldir%/pseudo.h \
	%reldir%/shaders.h \
	%reldir%/stats.h
nngn_SOURCES += \
	%reldir%/glfw.cpp \
	%reldir%/graphics.cpp \
	%reldir%/lua_graphics.cpp \
	%reldir%/pseudo.cpp \
	%reldir%/shaders.cpp \
	%reldir%/shaders_gl.cpp \
	%reldir%/shaders_vk.cpp

BUILT_SOURCES += \
	%reldir%/shaders.h \
	%reldir%/shaders_gl.cpp \
	%reldir%/shaders_vk.cpp
CLEANFILES += \
	%reldir%/shaders.h \
	%reldir%/shaders_gl.cpp \
	%reldir%/shaders_vk.cpp

%reldir%/shaders.h: scripts/shaders.sh $(SHADERS_SRC)
%reldir%/shaders.h:
	$(srcdir)/scripts/shaders.sh header $(SHADERS) > $@ || { rm $@; exit 1; }
%reldir%/shaders_vk.cpp: %reldir%/shaders.h $(SHADERS_VK)
%reldir%/shaders_vk.cpp:
	$(srcdir)/scripts/shaders.sh vk $(SHADERS_VK) > $@ || { rm $@; exit 1; }
%reldir%/shaders_gl.cpp: %reldir%/shaders.h $(SHADERS_GL)
%reldir%/shaders_gl.cpp:
	$(srcdir)/scripts/shaders.sh vk $(SHADERS_GL) > $@ || { rm $@; exit 1; }

include %reldir%/opengl/Makefile.am
include %reldir%/vulkan/Makefile.am
