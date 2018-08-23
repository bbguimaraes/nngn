SHADERS = $(SHADERS_GL) $(SHADERS_VK)

SHADERS_SRC = \
	%reldir%/font.frag \
	%reldir%/font.vert \
	%reldir%/sprite.frag \
	%reldir%/sprite.vert \
	%reldir%/sprite_depth.frag \
	%reldir%/sprite_depth.vert \
	%reldir%/triangle.frag \
	%reldir%/triangle.vert \
	%reldir%/triangle_depth.frag \
	%reldir%/triangle_depth.vert \
	%reldir%/voxel.frag

SHADERS_GL = \
	%reldir%/gl/font.frag \
	%reldir%/gl/font.vert \
	%reldir%/gl/sprite.frag \
	%reldir%/gl/sprite.vert \
	%reldir%/gl/sprite_depth.frag \
	%reldir%/gl/sprite_depth.vert \
	%reldir%/gl/triangle.frag \
	%reldir%/gl/triangle.vert \
	%reldir%/gl/triangle_depth.frag \
	%reldir%/gl/triangle_depth.vert \
	%reldir%/gl/voxel.frag

SHADERS_VK = \
	%reldir%/vk/font.frag.spv \
	%reldir%/vk/font.vert.spv \
	%reldir%/vk/sprite.frag.spv \
	%reldir%/vk/sprite.vert.spv \
	%reldir%/vk/sprite_depth.frag.spv \
	%reldir%/vk/sprite_depth.vert.spv \
	%reldir%/vk/triangle.frag.spv \
	%reldir%/vk/triangle.vert.spv \
	%reldir%/vk/triangle_depth.frag.spv \
	%reldir%/vk/triangle_depth.vert.spv \
	%reldir%/vk/voxel.frag.spv

COMPILE_GL = cpp -P -MD -MP -MF $@.d -MT $@ -o $@
PRE_COMPILE_VK = cpp -P -MD -MP -MF $@.d -MT $@ -DVULKAN -o $(@:.spv=)
COMPILE_VK = @GLSLANG@ -V $(@:.spv=) -o $@

%reldir%/gl/font.frag: %reldir%/font.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/font.frag
%reldir%/gl/font.vert: %reldir%/font.vert
	$(COMPILE_GL) $(srcdir)/%reldir%/font.vert
%reldir%/gl/sprite.frag: %reldir%/sprite.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/sprite.frag
%reldir%/gl/sprite.vert: %reldir%/sprite.vert
	$(COMPILE_GL) $(srcdir)/%reldir%/sprite.vert
%reldir%/gl/sprite_depth.frag: %reldir%/sprite_depth.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/sprite_depth.frag
%reldir%/gl/sprite_depth.vert: %reldir%/sprite_depth.vert
	$(COMPILE_GL) $(srcdir)/%reldir%/sprite_depth.vert
%reldir%/gl/triangle.frag: %reldir%/triangle.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/triangle.frag
%reldir%/gl/triangle.vert: %reldir%/triangle.vert
	$(COMPILE_GL) $(srcdir)/%reldir%/triangle.vert
%reldir%/gl/triangle_depth.frag: %reldir%/triangle_depth.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/triangle_depth.frag
%reldir%/gl/triangle_depth.vert: %reldir%/triangle_depth.vert
	$(COMPILE_GL) $(srcdir)/%reldir%/triangle_depth.vert
%reldir%/gl/voxel.frag: %reldir%/voxel.frag
	$(COMPILE_GL) $(srcdir)/%reldir%/voxel.frag

%reldir%/vk/font.frag.spv: %reldir%/font.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/font.frag; $(COMPILE_VK)
%reldir%/vk/font.vert.spv: %reldir%/font.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/font.vert; $(COMPILE_VK)
%reldir%/vk/sprite.frag.spv: %reldir%/sprite.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/sprite.frag; $(COMPILE_VK)
%reldir%/vk/sprite.vert.spv: %reldir%/sprite.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/sprite.vert; $(COMPILE_VK)
%reldir%/vk/sprite_depth.frag.spv: %reldir%/sprite_depth.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/sprite_depth.frag; $(COMPILE_VK)
%reldir%/vk/sprite_depth.vert.spv: %reldir%/sprite_depth.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/sprite_depth.vert; $(COMPILE_VK)
%reldir%/vk/triangle.frag.spv: %reldir%/triangle.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle.frag; $(COMPILE_VK)
%reldir%/vk/triangle.vert.spv: %reldir%/triangle.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle.vert; $(COMPILE_VK)
%reldir%/vk/triangle_depth.frag.spv: %reldir%/triangle_depth.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle_depth.frag; $(COMPILE_VK)
%reldir%/vk/triangle_depth.vert.spv: %reldir%/triangle_depth.vert
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/triangle_depth.vert; $(COMPILE_VK)
%reldir%/vk/voxel.frag.spv: %reldir%/voxel.frag
	$(PRE_COMPILE_VK) $(srcdir)/%reldir%/voxel.frag; $(COMPILE_VK)

CLEANFILES += \
	$(SHADERS) \
	$(SHADERS_VK:.spv=) \
	$(addsuffix .d,$(SHADERS))

-include %reldir%/gl/*.d
-include %reldir%/vk/*.d
