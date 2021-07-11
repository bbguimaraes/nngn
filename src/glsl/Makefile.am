SHADERS = $(SHADERS_GL) $(SHADERS_VK)

SHADERS_GL = \
	%reldir%/gl/font.frag \
	%reldir%/gl/font.vert \
	%reldir%/gl/sprite.frag \
	%reldir%/gl/sprite.vert \
	%reldir%/gl/voxel.frag \
	%reldir%/gl/sprite_depth.frag \
	%reldir%/gl/sprite_depth.vert \
	%reldir%/gl/triangle.frag \
	%reldir%/gl/triangle.vert \
	%reldir%/gl/triangle_depth.frag \
	%reldir%/gl/triangle_depth.vert \
	%reldir%/gl/post.vert \
	%reldir%/gl/post.frag

SHADERS_VK = \
	%reldir%/vk/font.frag.spv \
	%reldir%/vk/font.vert.spv \
	%reldir%/vk/sprite.frag.spv \
	%reldir%/vk/sprite.vert.spv \
	%reldir%/vk/voxel.frag.spv \
	%reldir%/vk/sprite_depth.frag.spv \
	%reldir%/vk/sprite_depth.vert.spv \
	%reldir%/vk/triangle.frag.spv \
	%reldir%/vk/triangle.vert.spv \
	%reldir%/vk/triangle_depth.frag.spv \
	%reldir%/vk/triangle_depth.vert.spv \
	%reldir%/vk/post.vert.spv \
	%reldir%/vk/post.frag.spv

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
