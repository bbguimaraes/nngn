noinst_HEADERS += \
	%reldir%/glfw.h \
	%reldir%/graphics.h \
	%reldir%/pseudo.h \
	%reldir%/shaders.h
nngn_SOURCES += \
	%reldir%/glfw.cpp \
	%reldir%/graphics.cpp \
	%reldir%/lua_graphics.cpp \
	%reldir%/opengl.cpp \
	%reldir%/pseudo.cpp \
	%reldir%/shaders.cpp \
	%reldir%/shaders_vk.cpp

BUILT_SOURCES += \
	%reldir%/shaders.h \
	%reldir%/shaders_vk.cpp
CLEANFILES += \
	%reldir%/shaders.h \
	%reldir%/shaders_vk.cpp

%reldir%/shaders.h: $(SHADERS)
	$(srcdir)/scripts/shaders.sh header $^ > $@ || { rm $@; exit 1; }
%reldir%/shaders_vk.cpp: $(SHADERS_VK)
	$(srcdir)/scripts/shaders.sh vk $^ > $@ || { rm $@; exit 1; }

include %reldir%/vulkan/Makefile.am
