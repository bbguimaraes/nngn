SHADERS = $(SHADERS_GL) $(SHADERS_VK)

SHADERS_SRC = \
	%reldir%/sprite.frag \
	%reldir%/sprite.vert \
	%reldir%/triangle.frag \
	%reldir%/triangle.vert

SHADERS_GL = \
	%reldir%/gl/sprite.frag \
	%reldir%/gl/sprite.vert \
	%reldir%/gl/triangle.frag \
	%reldir%/gl/triangle.vert

SHADERS_VK = \
	%reldir%/vk/sprite.frag.spv \
	%reldir%/vk/sprite.vert.spv \
	%reldir%/vk/triangle.frag.spv \
	%reldir%/vk/triangle.vert.spv

COMPILE_GL = cpp -P -MD -MP -MF $@.d -MT $@ -o $@
PRE_COMPILE_VK = cpp -P -MD -MP -MF $@.d -MT $@ -DVULKAN -o $(@:.spv=)
COMPILE_VK = @GLSLANG@ -V $(@:.spv=) -o $@

%reldir%/gl/sprite.frag: %reldir%/sprite.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/sprite.frag
%reldir%/gl/sprite.vert: %reldir%/sprite.vert
	$(COMPILE_GL) $(srcdir)/%reldir%/sprite.vert
%reldir%/gl/triangle.frag: %reldir%/triangle.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/triangle.frag
%reldir%/gl/triangle.vert: %reldir%/triangle.vert
	$(COMPILE_GL) $(srcdir)/%reldir%/triangle.vert

%reldir%/vk/sprite.frag.spv: %reldir%/sprite.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/sprite.frag; $(COMPILE_VK)
%reldir%/vk/sprite.vert.spv: %reldir%/sprite.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/sprite.vert; $(COMPILE_VK)
%reldir%/vk/triangle.frag.spv: %reldir%/triangle.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle.frag; $(COMPILE_VK)
%reldir%/vk/triangle.vert.spv: %reldir%/triangle.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle.vert; $(COMPILE_VK)

CLEANFILES += \
	$(SHADERS) \
	$(SHADERS_VK:.spv=) \
	$(addsuffix .d,$(SHADERS))

-include %reldir%/gl/*.d
-include %reldir%/vk/*.d
