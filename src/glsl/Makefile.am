SHADERS = $(SHADERS_GL) $(SHADERS_VK)

SHADERS_GL = \
	%reldir%/gl/triangle.frag \
	%reldir%/gl/triangle.vert

SHADERS_VK = \
	%reldir%/vk/triangle.frag.spv \
	%reldir%/vk/triangle.vert.spv

%reldir%/gl/%: %reldir%/%
	cpp -P -MD -MP -MF $@.d -MT $@ -o $@ $<
%reldir%/vk/%.spv: %reldir%/%
	cpp -P -MD -MP -MF $@.d -MT $@ -DVULKAN -o $(@:.spv=) $<
	glslangValidator -V $(@:.spv=) -o $@

CLEANFILES += \
	$(SHADERS) \
	$(SHADERS_VK:.spv=) \
	$(addsuffix .d,$(SHADERS))

-include %reldir%/gl/*.d
-include %reldir%/vk/*.d
