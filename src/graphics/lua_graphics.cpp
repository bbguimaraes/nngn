#include <optional>

#include "lua/state.h"
#include "lua/utils.h"
#include "utils/log.h"
#include "utils/utils.h"

#include "graphics.h"

using nngn::u32;
using nngn::Graphics;

namespace {

std::optional<Graphics::TerminalParameters> terminal_params(
    nngn::lua::table_view t
) {
    NNGN_LOG_CONTEXT_F();
    Graphics::TerminalParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ko = k.as<std::optional<std::string_view>>();
        if(!ko) {
            nngn::Log::l() << "only string keys are allowed\n";
            return std::nullopt;
        }
        const auto ks = *ko;
        if(ks == "fd")
            ret.fd = v.as<int>();
    }
    return {ret};
}

std::optional<Graphics::OpenGLParameters> opengl_params(
    nngn::lua::table_view t
) {
    NNGN_LOG_CONTEXT_F();
    Graphics::OpenGLParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ko = k.as<std::optional<std::string_view>>();
        if(!ko) {
            nngn::Log::l() << "only string keys are allowed\n";
            return std::nullopt;
        }
        const auto ks = *ko;
        if(ks == "hidden")
            ret.flags.set(Graphics::Parameters::Flag::HIDDEN, v.as<bool>());
        else if(ks == "debug")
            ret.flags.set(Graphics::Parameters::Flag::DEBUG, v.as<bool>());
        else if(ks == "maj")
            ret.maj = v.as<int>();
        else if(ks == "min")
            ret.min = v.as<int>();
    }
    return {ret};
}

std::optional<Graphics::VulkanParameters> vulkan_params(
    nngn::lua::table_view t, nngn::lua::state_arg lua_
) {
    NNGN_LOG_CONTEXT_F();
    auto lua = nngn::lua::state_view{lua_};
    Graphics::VulkanParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ko = k.as<std::optional<std::string_view>>();
        if(!ko) {
            nngn::Log::l() << "only string keys are allowed\n";
            return std::nullopt;
        }
        const auto ks = *ko;
        if(ks == "hidden")
            ret.flags.set(Graphics::Parameters::Flag::HIDDEN, v.as<bool>());
        else if(ks == "debug")
            ret.flags.set(Graphics::Parameters::Flag::DEBUG, v.as<bool>());
        else if(ks == "version") {
            const nngn::lua::table tt = nngn::lua::push(lua, v);
            ret.version = Graphics::Version{
                .major = tt.get<u32>(1),
                .minor = tt.get<u32>(2),
                .patch = tt.get<u32>(3),
                .name = {},
            };
        } else if(ks == "log_level")
            ret.log_level = v.as<Graphics::LogLevel>();
    }
    return {ret};
}

auto create(Graphics::Backend b, std::optional<const void*> params) {
    return Graphics::create(
        b, params ? static_cast<const uintptr_t*>(*params) + 1 : nullptr);
}

auto version(Graphics &g) {
    const auto ret = g.version();
    return std::tuple{ret.major, ret.minor, ret.patch, ret.name};
}

bool init_device(Graphics &g, std::optional<std::size_t> i)
    { return i ? g.init_device(*i) : g.init_device(); }

auto to_lua(nngn::lua::state_view lua, const Graphics::Extension &e) {
    return nngn::lua::table_map(lua,
        "name", std::string(e.name.data()),
        "version", e.version
    ).release();
}

auto to_lua(nngn::lua::state_view lua, const Graphics::Layer &l) {
    return nngn::lua::table_map(lua,
        "name", std::string(l.name.data()),
        "description", std::string(l.description.data()),
        "spec_version", std::string(l.spec_version.data()),
        "version", l.version
    ).release();
}

auto to_lua(nngn::lua::state_view lua, const Graphics::DeviceInfo &i) {
    constexpr auto fmt = [](auto x) {
        std::stringstream s;
        s << std::hex << std::showbase << x;
        return s.str();
    };
    return nngn::lua::table_map(lua,
        "name", std::string(i.name.data()),
        "version", std::string(i.version.data()),
        "driver_version", fmt(i.driver_version),
        "vendor_id", fmt(i.vendor_id),
        "device_id", fmt(i.device_id),
        "type", Graphics::enum_str(i.type)
    ).release();
}

auto to_lua(nngn::lua::state_view lua, const Graphics::QueueFamily &f) {
    return nngn::lua::table_map(lua,
        "flags", Graphics::flags_str(f.flags),
        "count", f.count
    ).release();
}

auto to_lua(nngn::lua::state_view, const Graphics::PresentMode &m) {
    return Graphics::enum_str(m);
}

auto to_lua(nngn::lua::state_view lua, const Graphics::MemoryHeap &h) {
    return nngn::lua::table_map(lua,
        "size", h.size,
        "flags", Graphics::flags_str(h.flags)
    ).release();
}

auto to_lua(nngn::lua::state_view lua, const Graphics::MemoryType &h) {
    return nngn::lua::table_map(lua,
        "flags", Graphics::flags_str(h.flags)
    ).release();
}

auto surface_info(const Graphics &g, nngn::lua::state_arg lua_) {
    const auto lua = nngn::lua::state_view{lua_};
    const auto i = g.surface_info();
    auto ret = lua.create_table(0, 5);
    ret["min_images"] = i.min_images,
    ret["max_images"] = i.max_images,
    ret["min_extent"] = nngn::lua::table_array(
        lua, i.min_extent.x, i.min_extent.y);
    ret["max_extent"] = nngn::lua::table_array(
        lua, i.max_extent.x, i.max_extent.y);
    ret["cur_extent"] = nngn::lua::table_array(
        lua, i.cur_extent.x, i.cur_extent.y);
    return ret.release();
}

template<typename T, auto count_f, auto get_f, typename ...Is>
auto get(nngn::lua::state_view lua, const Graphics &g, Is ...is) {
    const auto n = (g.*count_f)(is...);
    std::vector<T> v(n);
    (g.*get_f)(is..., v.data());
    auto ret = lua.create_table(static_cast<int>(n), 0);
    for(std::size_t i = 0; i < n; ++i)
        ret.raw_set(i + 1, to_lua(lua, v[i]));
    return ret;
}

auto extensions(const Graphics &g, nngn::lua::state_arg lua) {
    return get<
        Graphics::Extension,
        &Graphics::n_extensions,
        &Graphics::extensions>(static_cast<lua_State*>(lua), g);
}

auto layers(const Graphics &g, nngn::lua::state_arg lua) {
    return get<
        Graphics::Layer,
        &Graphics::n_layers,
        &Graphics::layers>(static_cast<lua_State*>(lua), g);
}

auto device_infos(const Graphics &g, nngn::lua::state_arg lua) {
    return get<
        Graphics::DeviceInfo,
        &Graphics::n_devices,
        &Graphics::device_infos>(static_cast<lua_State*>(lua), g);
}

auto device_extensions(
    const Graphics &g, std::size_t i, nngn::lua::state_arg lua
) {
    return get<
        Graphics::Extension,
        &Graphics::n_device_extensions,
        &Graphics::device_extensions>(static_cast<lua_State*>(lua), g, i);
}

auto queue_families(
    const Graphics &g, std::size_t i, nngn::lua::state_arg lua
) {
    return get<
        Graphics::QueueFamily,
        &Graphics::n_queue_families,
        &Graphics::queue_families>(static_cast<lua_State*>(lua), g, i);
}

auto present_modes(const Graphics &g, nngn::lua::state_arg lua) {
    return get<
        Graphics::PresentMode,
        &Graphics::n_present_modes,
        &Graphics::present_modes>(static_cast<lua_State*>(lua), g);
}

auto heaps(const Graphics &g, std::size_t i, nngn::lua::state_arg lua) {
    return get<
        Graphics::MemoryHeap,
        &Graphics::n_heaps,
        &Graphics::heaps>(static_cast<lua_State*>(lua), g, i);
}

auto memory_types(
    const Graphics &g, std::size_t i, std::size_t ih, nngn::lua::state_arg lua
) {
    return get<
        Graphics::MemoryType,
        &Graphics::n_memory_types,
        &Graphics::memory_types>(static_cast<lua_State*>(lua), g, i, ih);
}

auto window_size(const Graphics &g) {
    const auto s = g.window_size();
    return std::tuple(s.x, s.y);
}

auto stats(Graphics *g, nngn::lua::state_arg lua_) {
    const auto s = g->stats();
    const auto lua = nngn::lua::state_view{lua_};
    auto ret = lua.create_table(0, 2);
    ret["staging"] = nngn::lua::table_map(lua,
        "req_n_allocations", s.staging.req.n_allocations,
        "req_total_memory", s.staging.req.total_memory,
        "n_allocations", s.staging.n_allocations,
        "n_reused", s.staging.n_reused,
        "n_free", s.staging.n_free,
        "total_memory", s.staging.total_memory);
    ret["buffers"] = nngn::lua::table_map(lua,
        "n_writes", s.buffers.n_writes,
        "total_writes_bytes", s.buffers.total_writes_bytes);
    return ret.release();
}

}

NNGN_LUA_PROXY(Graphics,
    "TEXTURE_SIZE", nngn::lua::var(Graphics::TEXTURE_SIZE),
    "PSEUDOGRAPH", nngn::lua::var(Graphics::Backend::PSEUDOGRAPH),
    "TERMINAL_BACKEND", nngn::lua::var(Graphics::Backend::TERMINAL_BACKEND),
    "OPENGL_BACKEND", nngn::lua::var(Graphics::Backend::OPENGL_BACKEND),
    "OPENGL_ES_BACKEND", nngn::lua::var(Graphics::Backend::OPENGL_ES_BACKEND),
    "VULKAN_BACKEND", nngn::lua::var(Graphics::Backend::VULKAN_BACKEND),
    "LOG_LEVEL_DEBUG", nngn::lua::var(Graphics::LogLevel::DEBUG),
    "LOG_LEVEL_WARNING", nngn::lua::var(Graphics::LogLevel::WARNING),
    "LOG_LEVEL_ERROR", nngn::lua::var(Graphics::LogLevel::ERROR),
    "CURSOR_MODE_NORMAL", nngn::lua::var(Graphics::CursorMode::NORMAL),
    "CURSOR_MODE_HIDDEN", nngn::lua::var(Graphics::CursorMode::HIDDEN),
    "CURSOR_MODE_DISABLED", nngn::lua::var(Graphics::CursorMode::DISABLED),
    "terminal_params", terminal_params,
    "opengl_params", opengl_params,
    "vulkan_params", vulkan_params,
    "create_backend", create,
    "version", version,
    "init", &Graphics::init,
    "init_backend", &Graphics::init_backend,
    "init_instance", &Graphics::init_instance,
    "init_device", init_device,
    "selected_device", &Graphics::selected_device,
    "extensions", extensions,
    "layers", layers,
    "device_infos", device_infos,
    "device_extensions", device_extensions,
    "queue_families", queue_families,
    "surface_info", surface_info,
    "present_modes", present_modes,
    "heaps", heaps,
    "memory_types", memory_types,
    "error", &Graphics::error,
    "swap_interval", &Graphics::swap_interval,
    "window_size", window_size,
    "stats", stats,
    "set_n_frames", &Graphics::set_n_frames,
    "set_swap_interval", &Graphics::set_swap_interval,
    "set_cursor_mode", &Graphics::set_cursor_mode,
    "set_shadow_map_size", &Graphics::set_shadow_map_size,
    "set_shadow_cube_size", &Graphics::set_shadow_cube_size,
    "resize_textures", &Graphics::resize_textures)
