#ifndef NNGN_GRAPHICS_PSEUDO_H
#define NNGN_GRAPHICS_PSEUDO_H

#include "graphics.h"

namespace nngn {

struct Pseudograph : Graphics {
    Version version() const override { return {0, 0, 0, "no backend"}; }
    bool init_backend() override { return true; }
    bool init_instance() override { return true; }
    bool init_device() override { return true; }
    bool init_device(std::size_t) override { return true; }
    std::size_t n_extensions() const override { return 0; }
    std::size_t n_layers() const override { return 0; }
    std::size_t n_devices() const override { return 0; }
    std::size_t n_device_extensions(std::size_t) const override { return 0; }
    std::size_t n_queue_families(std::size_t) const override { return 0; }
    std::size_t n_present_modes() const override { return 0; }
    std::size_t n_heaps(std::size_t) const override { return 0; }
    std::size_t n_memory_types(std::size_t, std::size_t) const override
        { return 0; }
    std::size_t selected_device() const override { return 0; }
    void extensions(Extension*) const override {}
    void layers(Layer*) const override {}
    void device_infos(DeviceInfo*) const override {}
    void device_extensions(std::size_t, Extension*) const override {}
    void queue_families(std::size_t, QueueFamily*) const override {}
    SurfaceInfo surface_info() const override { return {}; }
    void present_modes(PresentMode*) const override {}
    void heaps(std::size_t, MemoryHeap*) const override {}
    void memory_types(std::size_t, std::size_t, MemoryType*) const override {}
    bool error() override { return false; }
    bool window_closed() const override { return false; }
    int swap_interval() const override { return 1; }
    GraphicsStats stats() override { return {}; }
    bool set_n_frames(std::size_t) override { return true; }
    bool set_n_swap_chain_images(std::size_t) override { return true; }
    void set_swap_interval(int) override {}
    void set_window_title(const char*) override {}
    void set_cursor_mode(CursorMode) override {}
    void set_key_callback(void*, key_callback_f) override {}
    void set_mouse_button_callback(void*, mouse_button_callback_f) override {}
    void set_mouse_move_callback(void*, mouse_move_callback_f) override {}
    void resize(int, int) override {}
    u32 create_pipeline(const PipelineConfiguration&) override { return 1; }
    u32 create_buffer(const BufferConfiguration&) override { return 1; }
    bool set_buffer_capacity(u32, u64) override { return true; }
    bool set_buffer_size(u32, u64) override { return true; }
    bool write_to_buffer(
        u32 b, u64 offset, u64 n, u64 size, void *data,
        void f(void*, void*, u64, u64)) override;
    bool resize_textures(u32) override { return true; }
    bool load_textures(u32, u32, const std::byte*) override { return true; }
    void poll_events() const override {}
    bool set_render_list(const RenderList&) override { return true; }
    bool render() override { return true; }
    bool vsync() override;
};

}

#endif
