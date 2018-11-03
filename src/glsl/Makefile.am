SHADERS = $(SHADERS_VK)

SHADERS_VK = \
	%reldir%/vk/triangle.frag.spv \
	%reldir%/vk/triangle.vert.spv

%reldir%/vk/%.spv: %reldir%/%
	cpp -P -MD -MP -MF $@.d -MT $@ -DVULKAN -o $(@:.spv=) $<
	glslangValidator -V $(@:.spv=) -o $@

CLEANFILES += \
	$(SHADERS) \
	$(SHADERS_VK:.spv=) \
	$(addsuffix .d,$(SHADERS))

-include %reldir%/vk/*.d
