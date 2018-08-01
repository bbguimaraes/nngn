#include "lua/function.h"
#include "lua/iter.h"
#include "lua/register.h"
#include "lua/table.h"

#include "utils/log.h"
#include "utils/utils.h"

#include "graphics.h"

using nngn::u32;
using nngn::Graphics;

NNGN_LUA_DECLARE_USER_TYPE(Graphics::OpenGLParameters, "OpenGLParameters")
NNGN_LUA_DECLARE_USER_TYPE(Graphics::VulkanParameters, "VulkanParameters")

namespace {

std::optional<Graphics::OpenGLParameters> opengl_params(
    nngn::lua::table_view t)
{
    NNGN_LOG_CONTEXT_F();
    Graphics::OpenGLParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ks = k.get<std::optional<std::string_view>>();
        if(!ks) {
            nngn::Log::l() << "only string keys are allowed\n";
            return {};
        }
        using Flag = Graphics::Parameters::Flag;
        if(*ks == "hidden")
            ret.flags.set(Flag::HIDDEN, v.get<bool>());
        else if(*ks == "debug")
            ret.flags.set(Flag::DEBUG, v.get<bool>());
        else if(*ks == "maj")
            ret.maj = v.get<int>();
        else if(*ks == "min")
            ret.min = v.get<int>();
    }
    return ret;
}

std::optional<Graphics::VulkanParameters> vulkan_params(
    nngn::lua::table_view t)
{
    NNGN_LOG_CONTEXT_F();
    Graphics::VulkanParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ks = k.get<std::optional<std::string_view>>();
        if(!ks) {
            nngn::Log::l() << "only string keys are allowed\n";
            return {};
        }
        using Flag = Graphics::Parameters::Flag;
        if(*ks == "hidden")
            ret.flags.set(Flag::HIDDEN, v.get<bool>());
        else if(*ks == "debug")
            ret.flags.set(Flag::DEBUG, v.get<bool>());
        else if(*ks == "version") {
            const nngn::lua::table_view tt = {v};
            ret.version = Graphics::Version{
                .major = tt[1],
                .minor = tt[2],
                .patch = tt[3],
                .name = {},
            };
        } else if(*ks == "log_level")
            ret.log_level = v.get<Graphics::LogLevel>();
    }
    return ret;
}

auto version(const Graphics &g) {
    const auto v = g.version();
    return std::tuple{v.major, v.minor, v.patch, v.name};
}

auto create(Graphics::Backend b, const void *params) {
    auto *const p = nngn::lua::user_data<char>::from_light(params);
    return Graphics::create(b, p).release();
}

bool init_device(Graphics &g, std::optional<lua_Integer> i) {
    return i
        ? g.init_device(nngn::narrow<std::size_t>(*i))
        : g.init_device();
}

auto selected_device(const Graphics &g) {
    return nngn::narrow<lua_Integer>(g.selected_device());
}

auto to_lua(nngn::lua::state_view lua, const Graphics::Extension &e) {
    return nngn::lua::table_map(lua,
        "name", std::string(e.name.data()),
        "version", e.version);
}

auto to_lua(nngn::lua::state_view lua, const Graphics::Layer &l) {
    return nngn::lua::table_map(lua,
        "name", std::string(l.name.data()),
        "description", std::string(l.description.data()),
        "spec_version", std::string(l.spec_version.data()),
        "version", l.version);
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
        "type", Graphics::enum_str(i.type));
}

auto to_lua(nngn::lua::state_view lua, const Graphics::QueueFamily &f) {
    return nngn::lua::table_map(lua,
        "flags", Graphics::flags_str(f.flags),
        "count", f.count);
}

auto to_lua(nngn::lua::state_view, const Graphics::PresentMode &m) {
    return Graphics::enum_str(m);
}

auto to_lua(nngn::lua::state_view lua, const Graphics::MemoryHeap &h) {
    return nngn::lua::table_map(lua,
        "size", nngn::narrow<lua_Integer>(h.size),
        "flags", Graphics::flags_str(h.flags));
}

auto to_lua(nngn::lua::state_view lua, const Graphics::MemoryType &h) {
    return nngn::lua::table_map(lua,
        "flags", Graphics::flags_str(h.flags));
}

auto surface_info(const Graphics &g, nngn::lua::state_view lua) {
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
        ret.raw_set(nngn::narrow<lua_Integer>(i) + 1, to_lua(lua, v[i]));
    return ret.release();
}

auto extensions(const Graphics &g, nngn::lua::state_view lua) {
    return get<
        Graphics::Extension,
        &Graphics::n_extensions,
        &Graphics::extensions>(lua, g);
}

auto layers(const Graphics &g, nngn::lua::state_view lua) {
    return get<
        Graphics::Layer,
        &Graphics::n_layers,
        &Graphics::layers>(lua, g);
}

auto device_infos(const Graphics &g, nngn::lua::state_view lua) {
    return get<
        Graphics::DeviceInfo,
        &Graphics::n_devices,
        &Graphics::device_infos>(lua, g);
}

auto device_extensions(
    const Graphics &g, lua_Integer i, nngn::lua::state_view lua
) {
    return get<
        Graphics::Extension,
        &Graphics::n_device_extensions,
        &Graphics::device_extensions>(lua, g, nngn::narrow<std::size_t>(i));
}

auto queue_families(
    const Graphics &g, lua_Integer i, nngn::lua::state_view lua
) {
    return get<
        Graphics::QueueFamily,
        &Graphics::n_queue_families,
        &Graphics::queue_families>(lua, g, nngn::narrow<std::size_t>(i));
}

auto present_modes(const Graphics &g, nngn::lua::state_view lua) {
    return get<
        Graphics::PresentMode,
        &Graphics::n_present_modes,
        &Graphics::present_modes>(lua, g);
}

auto heaps(const Graphics &g, lua_Integer i, nngn::lua::state_view lua) {
    return get<
        Graphics::MemoryHeap,
        &Graphics::n_heaps,
        &Graphics::heaps>(lua, g, nngn::narrow<std::size_t>(i));
}

auto memory_types(
    const Graphics &g, lua_Integer i, lua_Integer ih, nngn::lua::state_view lua
) {
    return get<
        Graphics::MemoryType,
        &Graphics::n_memory_types,
        &Graphics::memory_types>(
            lua, g,
            nngn::narrow<std::size_t>(i),
            nngn::narrow<std::size_t>(ih));
}

auto stats(Graphics *g, nngn::lua::state_view lua) {
    constexpr auto cast = [](auto x) { return nngn::narrow<lua_Integer>(x); };
    const auto s = g->stats();
    auto ret = lua.create_table(0, 2);
    ret["staging"] = nngn::lua::table_map(lua,
        "req_n_allocations", cast(s.staging.req.n_allocations),
        "req_total_memory", cast(s.staging.req.total_memory),
        "n_allocations", cast(s.staging.n_allocations),
        "n_reused", cast(s.staging.n_reused),
        "n_free", cast(s.staging.n_free),
        "total_memory", cast(s.staging.total_memory));
    ret["buffers"] = nngn::lua::table_map(lua,
        "n_writes", cast(s.buffers.n_writes),
        "total_writes_bytes", cast(s.buffers.total_writes_bytes));
    return ret.release();
}

void set_n_frames(Graphics &g, lua_Integer n) {
    g.set_n_frames(nngn::narrow<std::size_t>(n));
}

void set_n_swap_chain_images(Graphics &g, lua_Integer n) {
    g.set_n_swap_chain_images(nngn::narrow<std::size_t>(n));
}

void register_graphics(nngn::lua::table_view t) {
    t["TEXTURE_SIZE"] = Graphics::TEXTURE_SIZE;
    t["PSEUDOGRAPH"] = Graphics::Backend::PSEUDOGRAPH;
    t["OPENGL_BACKEND"] = Graphics::Backend::OPENGL_BACKEND;
    t["OPENGL_ES_BACKEND"] = Graphics::Backend::OPENGL_ES_BACKEND;
    t["VULKAN_BACKEND"] = Graphics::Backend::VULKAN_BACKEND;
    t["LOG_LEVEL_DEBUG"] = Graphics::LogLevel::DEBUG;
    t["LOG_LEVEL_WARNING"] = Graphics::LogLevel::WARNING;
    t["LOG_LEVEL_ERROR"] = Graphics::LogLevel::ERROR;
    t["CURSOR_MODE_NORMAL"] = Graphics::CursorMode::NORMAL;
    t["CURSOR_MODE_HIDDEN"] = Graphics::CursorMode::HIDDEN;
    t["CURSOR_MODE_DISABLED"] = Graphics::CursorMode::DISABLED;
    t["opengl_params"] = opengl_params;
    t["vulkan_params"] = vulkan_params;
    t["create_backend"] = create;
    t["version"] = version;
    t["init"] = &Graphics::init;
    t["init_backend"] = &Graphics::init_backend;
    t["init_instance"] = &Graphics::init_instance;
    t["init_device"] = init_device;
    t["selected_device"] = selected_device;
    t["extensions"] = extensions;
    t["layers"] = layers;
    t["device_infos"] = device_infos;
    t["device_extensions"] = device_extensions;
    t["queue_families"] = queue_families;
    t["surface_info"] = surface_info;
    t["present_modes"] = present_modes;
    t["heaps"] = heaps;
    t["memory_types"] = memory_types;
    t["error"] = &Graphics::error;
    t["swap_interval"] = &Graphics::swap_interval;
    t["stats"] = stats;
    t["set_n_frames"] = set_n_frames;
    t["set_n_swap_chain_images"] = set_n_swap_chain_images;
    t["set_swap_interval"] = &Graphics::set_swap_interval;
    t["set_cursor_mode"] = &Graphics::set_cursor_mode;
    t["resize_textures"] = &Graphics::resize_textures;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Graphics::Parameters)
NNGN_LUA_DECLARE_USER_TYPE(Graphics)
NNGN_LUA_PROXY(Graphics, register_graphics)
NNGN_LUA_PROXY(Graphics::Parameters)
NNGN_LUA_PROXY(Graphics::OpenGLParameters)
NNGN_LUA_PROXY(Graphics::VulkanParameters)
