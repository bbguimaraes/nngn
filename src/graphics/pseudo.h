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
    bool set_n_frames(std::size_t) override { return true; }
    void set_swap_interval(int) override {}
    void set_window_title(const char*) override {}
    void set_cursor_mode(CursorMode) override {}
    void set_key_callback(void*, key_callback_f) override {}
    void set_mouse_button_callback(void*, mouse_button_callback_f) override {}
    void set_mouse_move_callback(void*, mouse_move_callback_f) override {}
    void resize(int, int) override {}
    void poll_events() const override {}
    bool render() override { return true; }
    bool vsync() override;
};

}

#endif
