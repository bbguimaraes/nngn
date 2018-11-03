local function common0(g)
    print("  Extensions:")
    for _, x in ipairs(g:extensions()) do
        io.write("    ", x.name, " (", x.version, ")\n")
    end
    print("  Layers:")
    for _, x in ipairs(g:layers()) do
        io.write(
            "    ", x.name,
            " (", x.spec_version, ", ", x.version, ", ", x.description, ")\n")
    end
    print("  Devices:")
    for i, x in ipairs(g:device_infos()) do
        io.write("    ", x.name, "\n")
        io.write("      Version: ", x.version, "\n")
        io.write("      Driver version: ", x.driver_version, "\n")
        io.write("      Vendor ID: ", x.vendor_id, "\n")
        io.write("      Device ID: ", x.device_id, "\n")
        io.write("      Type: ", x.type, "\n")
        io.write("      Extensions: \n")
        for _, x in ipairs(g:device_extensions(i - 1)) do
            io.write("        ", x.name, " (", x.version, ")\n")
        end
        io.write("      Queue families: \n")
        for i, x in ipairs(g:queue_families(i - 1)) do
            io.write("        ", i - 1, "\n")
            io.write("          Flags: ", x.flags, "\n")
            io.write("          Count: ", x.count, "\n")
        end
        print("    Memory heaps:")
        for ih, x in ipairs(g:heaps(i - 1)) do
            io.write("      ", i - 1, "\n")
            io.write("        Size: ", x.size, "\n")
            io.write("        Flags: ", x.flags, "\n")
            print("          Memory types:")
            for _, x in ipairs(g:memory_types(i - 1, ih - 1)) do
                io.write("            Flags: ", x.flags, "\n")
            end
        end
    end
end

local function common1(g)
    local sel = g:selected_device()
    io.write(
        "  Selected device: ", sel,
        " (", g:device_infos()[sel + 1].name, ")\n")
    print("  Surface:")
    local i = g:surface_info()
    print("    Images:")
    io.write("      Min.: ", i.min_images, "\n")
    io.write("      Max.: ", i.max_images, "\n")
    print("    Extents:")
    io.write("      Min.: ", i.min_extent[1], "x", i.min_extent[2], "\n")
    io.write("      Max.: ", i.max_extent[1], "x", i.max_extent[2], "\n")
    io.write("      Cur.: ", i.cur_extent[1], "x", i.cur_extent[2], "\n")
    print("    Present modes:")
    for _, x in ipairs(g:present_modes()) do io.write("      ", x, "\n") end
end

local function gl(...)
    local gl = Graphics.create_backend(...)
    assert(gl:init_backend())
    assert(gl:init_instance())
    local maj, min, _, es = gl:version()
    if es then es = " " .. es end
    io.write("  Version: ", maj, ".", min, es, "\n")
    common0(gl)
    common1(gl)
end

local function vulkan(...)
    local vk = Graphics.create_backend(...)
    assert(vk:init_backend())
    local maj, min, patch = vk:version()
    io.write("  Version: ", maj, ".", min, ".", patch, "\n")
    assert(vk:init_instance())
    common0(vk)
    assert(vk:init_device())
    common1(vk)
end

print("OpenGL ES")
gl(
    Graphics.OPENGL_ES_BACKEND,
    Graphics.opengl_params{maj = 3, min = 1, hidden = true, debug = true})
collectgarbage()

print("OpenGL")
gl(
    Graphics.OPENGL_BACKEND,
    Graphics.opengl_params{maj = 4, min = 2, hidden = true, debug = true})
collectgarbage()

print("Vulkan")
vulkan(
    Graphics.VULKAN_BACKEND,
    Graphics.vulkan_params{version = {1, 2, 164}, hidden = true, debug = true})
collectgarbage()

nngn:exit()
