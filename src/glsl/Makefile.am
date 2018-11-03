SHADERS = $(SHADERS_VK)

SHADERS_SRC = \
	%reldir%/triangle.frag \
	%reldir%/triangle.vert

SHADERS_VK = \
	%reldir%/vk/triangle.frag.spv \
	%reldir%/vk/triangle.vert.spv

PRE_COMPILE_VK = cpp -P -MD -MP -MF $@.d -MT $@ -DVULKAN -o $(@:.spv=)
COMPILE_VK = @GLSLANG@ -V $(@:.spv=) -o $@

%reldir%/vk/triangle.frag.spv: %reldir%/triangle.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle.frag; $(COMPILE_VK)
%reldir%/vk/triangle.vert.spv: %reldir%/triangle.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle.vert; $(COMPILE_VK)

CLEANFILES += \
	$(SHADERS) \
	$(SHADERS_VK:.spv=) \
	$(addsuffix .d,$(SHADERS))

-include %reldir%/vk/*.d
