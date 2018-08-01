#include <optional>

#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "utils/log.h"
#include "utils/utils.h"

#include "graphics.h"

using nngn::Graphics;

namespace {

std::optional<Graphics::OpenGLParameters> opengl_params(
        const sol::stack_table &t) {
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
        const sol::stack_table &t) {
    NNGN_LOG_CONTEXT_F();
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
            const auto tt = v.as<sol::table>();
            ret.version = Graphics::Version{
                .major = tt[1].get_or(std::uint32_t{}),
                .minor = tt[2].get_or(std::uint32_t{}),
                .patch = tt[3].get_or(std::uint32_t{}),
                .name = {}};
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

auto to_lua(sol::state_view *sol, const Graphics::Extension &e) {
    return sol->create_table_with(
        "name", std::string(e.name.data()),
        "version" , e.version);
}

auto to_lua(sol::state_view *sol, const Graphics::Layer &l) {
    return sol->create_table_with(
        "name", std::string(l.name.data()),
        "description", std::string(l.description.data()),
        "spec_version", std::string(l.spec_version.data()),
        "version", l.version);
}

auto to_lua(sol::state_view *sol, const Graphics::DeviceInfo &i) {
    constexpr auto fmt = [](auto x) {
        std::stringstream s;
        s << std::hex << std::showbase << x;
        return s.str();
    };
    return sol->create_table_with(
        "name", std::string(i.name.data()),
        "version", std::string(i.version.data()),
        "driver_version", fmt(i.driver_version),
        "vendor_id", fmt(i.vendor_id),
        "device_id", fmt(i.device_id),
        "type", Graphics::enum_str(i.type));
}

auto to_lua(sol::state_view *sol, const Graphics::QueueFamily &f) {
    return sol->create_table_with(
        "flags", Graphics::flags_str(f.flags),
        "count", f.count);
}

auto to_lua(sol::state_view*, const Graphics::PresentMode &m)
    { return Graphics::enum_str(m); }

auto to_lua(sol::state_view *sol, const Graphics::MemoryHeap &h) {
    return sol->create_table_with(
        "size", h.size,
        "flags", Graphics::flags_str(h.flags));
}

auto to_lua(sol::state_view *sol, const Graphics::MemoryType &h)
    { return sol->create_table_with("flags", Graphics::flags_str(h.flags)); }

auto surface_info(sol::this_state sol, const Graphics &g) {
    const auto i = g.surface_info();
    sol::state_view sol_ = {sol};
    return sol_.create_table_with(
        "min_images", i.min_images,
        "max_images", i.max_images,
        "min_extent", sol_.create_table_with(
            1, i.min_extent.x,
            2, i.min_extent.y),
        "max_extent", sol_.create_table_with(
            1, i.max_extent.x,
            2, i.max_extent.y),
        "cur_extent", sol_.create_table_with(
            1, i.cur_extent.x,
            2, i.cur_extent.y));
}

template<typename T, auto count_f, auto get_f, typename ...Is>
auto get(sol::state_view sol, const Graphics &g, Is ...is) {
    const auto n = (g.*count_f)(is...);
    std::vector<T> v(n);
    (g.*get_f)(is..., v.data());
    sol::table ret(sol, sol::new_table(static_cast<int>(n)));
    for(std::size_t i = 0; i < n; ++i)
        ret.raw_set(i + 1, to_lua(&sol, v[i]));
    return ret;
}

auto extensions(sol::this_state sol, const Graphics &g) {
    return get<
        Graphics::Extension,
        &Graphics::n_extensions,
        &Graphics::extensions>(sol, g);
}

auto layers(sol::this_state sol, const Graphics &g) {
    return get<
        Graphics::Layer,
        &Graphics::n_layers,
        &Graphics::layers>(sol, g);
}

auto device_infos(sol::this_state sol, const Graphics &g) {
    return get<
        Graphics::DeviceInfo,
        &Graphics::n_devices,
        &Graphics::device_infos>(sol, g);
}

auto device_extensions(sol::this_state sol, const Graphics &g, std::size_t i) {
    return get<
        Graphics::Extension,
        &Graphics::n_device_extensions,
        &Graphics::device_extensions>(sol, g, i);
}

auto queue_families(sol::this_state sol, const Graphics &g, std::size_t i) {
    return get<
        Graphics::QueueFamily,
        &Graphics::n_queue_families,
        &Graphics::queue_families>(sol, g, i);
}

auto present_modes(sol::this_state sol, const Graphics &g) {
    return get<
        Graphics::PresentMode,
        &Graphics::n_present_modes,
        &Graphics::present_modes>(sol, g);
}

auto heaps(sol::this_state sol, const Graphics &g, std::size_t i) {
    return get<
        Graphics::MemoryHeap,
        &Graphics::n_heaps,
        &Graphics::heaps>(sol, g, i);
}

auto memory_types(
    sol::this_state sol, const Graphics &g, std::size_t i, std::size_t ih
) {
    return get<
        Graphics::MemoryType,
        &Graphics::n_memory_types,
        &Graphics::memory_types>(sol, g, i, ih);
}

auto stats(sol::this_state sol_, Graphics *g) {
    const auto s = g->stats();
    sol::state_view sol{sol_};
    return sol.create_table_with(
        "staging", sol.create_table_with(
            "req_n_allocations", s.staging.req.n_allocations,
            "req_total_memory", s.staging.req.total_memory,
            "n_allocations", s.staging.n_allocations,
            "n_reused", s.staging.n_reused,
            "n_free", s.staging.n_free,
            "total_memory", s.staging.total_memory),
        "buffers", sol.create_table_with(
            "n_writes", s.buffers.n_writes,
            "total_writes_bytes", s.buffers.total_writes_bytes));
}

}

NNGN_LUA_PROXY(Graphics,
    sol::no_constructor,
    "TEXTURE_SIZE", sol::var(Graphics::TEXTURE_SIZE),
    "PSEUDOGRAPH", sol::var(Graphics::Backend::PSEUDOGRAPH),
    "OPENGL_BACKEND", sol::var(Graphics::Backend::OPENGL_BACKEND),
    "OPENGL_ES_BACKEND", sol::var(Graphics::Backend::OPENGL_ES_BACKEND),
    "VULKAN_BACKEND", sol::var(Graphics::Backend::VULKAN_BACKEND),
    "LOG_LEVEL_DEBUG", sol::var(Graphics::LogLevel::DEBUG),
    "LOG_LEVEL_WARNING", sol::var(Graphics::LogLevel::WARNING),
    "LOG_LEVEL_ERROR", sol::var(Graphics::LogLevel::ERROR),
    "CURSOR_MODE_NORMAL", sol::var(Graphics::CursorMode::NORMAL),
    "CURSOR_MODE_HIDDEN", sol::var(Graphics::CursorMode::HIDDEN),
    "CURSOR_MODE_DISABLED", sol::var(Graphics::CursorMode::DISABLED),
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
    "stats", stats,
    "set_n_frames", &Graphics::set_n_frames,
    "set_swap_interval", &Graphics::set_swap_interval,
    "set_cursor_mode", &Graphics::set_cursor_mode,
    "resize_textures", &Graphics::resize_textures)
