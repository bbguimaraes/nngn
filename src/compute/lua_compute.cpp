#include <algorithm>
#include <bit>
#include <cstddef>

#include "lua/state.h"
#include "utils/literals.h"
#include "utils/log.h"
#include "utils/ranges.h"
#include "utils/utils.h"

#include "compute.h"

using namespace nngn::literals;
using nngn::u32, nngn::Compute;
using Type = nngn::Compute::Type;

namespace {

template<typename T>
T from_table_array(nngn::lua::table_view t) {
    const auto n = static_cast<std::size_t>(t.size());
    T ret = {};
    ret.reserve(n);
    for(auto i = 1_z; i <= n; ++i)
        ret.push_back(t[i]);
    return ret;
}

void invalid_type(Type t) {
    nngn::Log::l() << "invalid type: " << static_cast<unsigned>(t) << '\n';
}

bool check_size(
    const std::byte *p, std::size_t n,
    const std::vector<std::byte> &v
) {
    const auto b = v.data(), e = b + v.size();
    assert(b <= p && p < e);
    if(p + n <= e)
        return true;
    nngn::Log::l() << "insufficient size\n";
    return false;
}

bool check_size(std::size_t n, const std::vector<std::byte> &v) {
    return check_size(v.data(), n, v);
}

auto vector_size(Type t) {
    constexpr std::array v = {
        std::size_t{}, // NONE
        std::size_t{}, // LOCAL
        std::size_t{}, // BYTE
        std::size_t{}, // INT
        std::size_t{}, // UINT
        std::size_t{}, // FLOAT
        sizeof(std::byte), // BYTEV
        sizeof(std::int32_t), // INTV
        sizeof(std::uint32_t), // UINTV
        sizeof(float), // FLOATV
        std::size_t{}, // DATA
        std::size_t{}, // BUFFER
        std::size_t{}, // IMAGE
        std::size_t{}, // SAMPLER
    };
    static_assert(v.size() == static_cast<std::size_t>(Type::N));
    const auto i = static_cast<std::size_t>(t);
    assert(i < v.size());
    const auto ret = v[i];
    if(!ret)
        nngn::Log::l() << "invalid type: " << i << '\n';
    return ret;
}

template<typename T, typename Table>
auto from_table(const Table &t) {
    const auto n = t.size();
    std::vector<T> ret;
    ret.reserve(static_cast<std::size_t>(n));
    for(lua_Integer i = 1; i <= n; ++i)
        ret.push_back(t[i]);
    return ret;
}

template<typename T>
auto read_table(const auto &t, std::size_t i, std::size_t n, std::byte *p) {
    for(n += i; i < n; ++i, p += sizeof(T)) {
        const auto v = t.template raw_get<T>(i);
        std::memcpy(p, &v, sizeof(T));
    }
    return p;
}

auto write_table(
    nngn::lua::table_view t,
    std::size_t i, std::size_t n, const auto *p
) {
    for(n += i; i < n; ++i, ++p)
        t.raw_set(i, *p);
    return p;
}

template<typename T>
auto write_table(
    nngn::lua::table_view t, std::size_t i, std::size_t n, const std::byte *p
) {
    const auto *tp = static_cast<const T*>(static_cast<const void*>(p));
    return nngn::as_bytes(write_table(t, i, n, tp));
}

decltype(auto) map(Type t, auto &&sf, auto &&vf, auto &&of) {
    switch(t) {
    case Type::BYTE: return FWD(sf)(std::byte{});
    case Type::INT: return FWD(sf)(std::int32_t{});
    case Type::UINT: return FWD(sf)(std::uint32_t{});
    case Type::FLOAT: return FWD(sf)(float{});
    case Type::BYTEV: return FWD(vf)(std::byte{});
    case Type::INTV: return FWD(vf)(std::int32_t{});
    case Type::UINTV: return FWD(vf)(std::uint32_t{});
    case Type::FLOATV: return FWD(vf)(float{});
    case Type::NONE:
    case Type::LOCAL:
    case Type::DATA:
    case Type::BUFFER:
    case Type::IMAGE:
    case Type::SAMPLER:
    case Type::N:
    default: return FWD(of)(t);
    }
}

decltype(auto) map_vector(Type t, auto &&def, auto &&f) {
    const auto i = [t, def = FWD(def)](auto)
        { invalid_type(t); return std::move(def); };
    return map(t, i, f, i);
}

bool to_bytes(Type type, nngn::lua::table_view t, std::vector<std::byte> &v) {
    NNGN_LOG_CONTEXT_F();
    return map_vector(type, false, [&t, &v]<typename T>(T) {
        const auto n = static_cast<std::size_t>(t.size());
        return check_size(n, v)
            && (read_table<T>(t, 1, n, v.data()), true);
    });
}

auto read_vector(
    const std::vector<std::byte> &v, std::size_t off, std::size_t n, Type type,
    nngn::lua::state_arg lua
) {
    NNGN_LOG_CONTEXT_F();
    using R = nngn::lua::table_view;
    using O = std::optional<R>;
    return map_vector(type, O{}, [lua, &v, off, n]<typename T>(T) mutable {
        constexpr auto size = sizeof(T);
        if(!n) {
            if(v.size() % size)
                return nngn::Log::l() << "invalid vector size for type\n", O{};
            n = v.size() / size;
        }
        auto ret = nngn::lua::state_view{lua}
            .create_table(static_cast<int>(n), 0);
        write_table<T>(ret, 1, n, v.data() + off * size);
        return O{ret.release()};
    });
}

void zero_vector(
    std::vector<std::byte> &v, std::size_t off, std::size_t n
) {
    std::memset(v.data() + off, 0, n);
}

bool fill_vector(
    std::vector<std::byte> &v, std::size_t off, std::size_t n,
    Type type, nngn::lua::table_view t
) {
    NNGN_LOG_CONTEXT_F();
    std::vector<std::byte> tv(static_cast<std::size_t>(t.size()));
    if(!to_bytes(type, t, tv))
        return false;
    const auto i = begin(v) + static_cast<std::ptrdiff_t>(off);
    nngn::fill_with_pattern(
        begin(tv), end(tv),
        i, i + static_cast<std::ptrdiff_t>(n));
    return true;
}

template<typename T>
auto write_scalar_type(const auto &t, std::size_t i, std::byte *p) {
    const T v = t[i];
    std::memcpy(p, &v, sizeof(T));
    return p + sizeof(T);
}

template<typename T>
auto write_vector_type(nngn::lua::table_view t, std::size_t i, std::byte *p) {
    if constexpr(std::is_same_v<T, std::byte>)
        if(const auto v = t.get<std::optional<std::string_view>>(i)) {
            const auto n = v->size();
            std::memcpy(p, v->data(), n);
            return p + n;
        }
    const nngn::lua::table_view tt = t[i];
    return read_table<T>(tt, 1, static_cast<std::size_t>(tt.size()), p);
}

bool write_vector(
    std::vector<std::byte> &v, std::size_t off, nngn::lua::table_view t
) {
    NNGN_LOG_CONTEXT_F();
    auto *p = v.data() + off;
    for(auto i = 1_z, n = static_cast<std::size_t>(t.size()); i <= n;) {
        const auto type = nngn::chain_cast<Type, lua_Integer>(t[i++]);
        if(type == Type::DATA) {
            const nngn::lua::table tt = t[i++];
            const auto size = tt.get<std::size_t>(1);
            if(!check_size(p, size, v))
                return false;
            std::memcpy(p, tt[2], size);
            p += size;
            continue;
        }
        if(!map(
            type,
            [&v, &t, &p, &i]<typename T>(T) {
                return check_size(p, sizeof(T), v)
                    && (p = write_scalar_type<T>(t, i++, p), true);
            },
            [&v, &t, &p, &i]<typename T>(T) {
                const nngn::lua::table tt = t[i++];
                const auto nt = static_cast<std::size_t>(tt.size());
                return check_size(p, nt, v)
                    && (p = read_table<T>(tt, 1, nt, p));
            },
            [type](auto) { invalid_type(type); return false; }
        ))
            return false;
    }
    return true;
}

auto read_events(
    const Compute &c,
    Compute::Events *events,
    std::span<const Compute::Type> types,
    std::optional<nngn::lua::object> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    using E = Compute::Event;
    using V = std::vector<E>;
    std::tuple<V, V> ret = {};
    auto &[w, e] = ret;
    if(wait_opt) {
        const nngn::lua::state_view lua = wait_opt->lua_state();
        const nngn::lua::table t = nngn::lua::push(lua, *wait_opt);
        const auto tmp_w =
            from_table_array<std::vector<nngn::lua::sol_user_type<E>>>(t);
        w.reserve(tmp_w.size());
        for(auto x : tmp_w)
            w.push_back(x.value);
        events->n_wait = w.size();
        events->wait_list = w.data();
    }
    if(events_opt) {
        e.resize(
            1 + !!events->wait_list
            + c.n_events(types.size(), types.data()));
        events->events = e.data();
    }
    return ret;
}

void write_events(
    std::span<const Compute::Event> events,
    std::optional<nngn::lua::table_view> events_opt
) {
    if(!events_opt)
        return;
    auto &x = *events_opt;
    const auto e = static_cast<std::size_t>(events.size());
    const auto b = static_cast<std::size_t>(x.size());
    for(std::size_t i = 0; i < e; ++i)
        x.raw_set(b + i + 1, nngn::lua::sol_user_type{events[i]});
}

auto opt(const Compute::Handle &h) {
    using O = std::optional<std::uint32_t>;
    return h.id ? O{h.id} : O{};
}

auto create_buffer(
    Compute &c, Compute::MemFlag flags, Type type, std::size_t n,
    std::optional<const std::vector<std::byte>*> ov
) {
    NNGN_LOG_CONTEXT_F();
    const auto *const v = ov.value_or(nullptr);
    if(!n)
        return v
            ? opt(c.create_buffer(flags, v->size(), v->data()))
            : opt(c.create_buffer(flags, 0, nullptr));
    const auto size = vector_size(type);
    return size
        ? opt(c.create_buffer(flags, n * size, v ? v->data() : nullptr))
        : opt({});
}

bool read_buffer(
    const Compute &c, std::uint32_t b, Type type, std::size_t n,
    std::vector<std::byte> &v
) {
    NNGN_LOG_CONTEXT_F();
    const auto size = n * vector_size(type);
    return size
        && check_size(size, v)
        && c.read_buffer({{b}}, 0, size, v.data(), {});
}

bool fill_buffer(
    const Compute &c, std::uint32_t b, std::size_t off, std::size_t n,
    Type type, nngn::lua::table_view pattern_t,
    std::optional<nngn::lua::object> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    NNGN_LOG_CONTEXT_F();
    std::vector<std::byte> pattern(static_cast<std::size_t>(pattern_t.size()));
    if(!to_bytes(type, pattern_t, pattern))
        return false;
    const auto size = vector_size(type);
    if(!size)
        return false;
    Compute::Events events = {};
    const auto [wait_v, events_v] =
        read_events(c, &events, {}, wait_opt, events_opt);
    if(!c.fill_buffer(
            {{b}}, off * size, n * size, pattern.size(), pattern.data(),
            events))
        return false;
    write_events(events_v, events_opt);
    return true;
}

bool write_buffer(
    const Compute &c, std::uint32_t b, std::size_t off, std::size_t n,
    Type type, const std::vector<std::byte> &v
) {
    NNGN_LOG_CONTEXT_F();
    const auto size = vector_size(type);
    return size && c.write_buffer({{b}}, off * size, n * size, v.data(), {});
}

auto create_image(
    Compute &c, Type type, std::size_t w, std::size_t h,
    Compute::MemFlag flags, std::optional<const std::vector<std::byte>*> ov
) {
    NNGN_LOG_CONTEXT_F();
    const auto *const v = *ov;
    return opt(c.create_image(type, w, h, flags, ov ? v->data() : nullptr));
}

auto read_image(
    const Compute &c, std::uint32_t i, std::size_t w, std::size_t h,
    Type type, std::vector<std::byte> &v
) {
    NNGN_LOG_CONTEXT_F();
    constexpr std::size_t channels = 4;
    const auto size = w * h * vector_size(type);
    return size
        && check_size(channels * size, v)
        && c.read_image({{i}}, w, h, v.data(), {});
}

bool fill_image(
    const Compute &c, std::uint32_t i, std::size_t w, std::size_t h,
    Type type, nngn::lua::table_view t
) {
    NNGN_LOG_CONTEXT_F();
    const auto size = vector_size(type);
    if(!size)
        return false;
    std::vector<std::byte> v(size * static_cast<std::size_t>(t.size()));
    return to_bytes(type, t, v)
        && c.fill_image({{i}}, w, h, v.data(), {});
}

auto create_sampler(Compute &c) { return opt(c.create_sampler()); }
auto create_program(Compute &c, std::string_view src, const char *opts)
    { return opt(c.create_program(src, opts)); }

auto prof_info(
    const Compute &c, Compute::ProfInfo info,
    const nngn::lua::as_table_t<std::vector<Compute::Event>> &events
) {
    NNGN_LOG_CONTEXT_F();
    const auto &v = events.value();
    const auto n = v.size();
    const auto pc = static_cast<std::size_t>(
        std::popcount(
            static_cast<std::underlying_type_t<Compute::ProfInfo>>(info)));
    std::vector<uint64_t> ret(n * pc);
    using R = nngn::lua::as_table_t<decltype(ret)>;
    return c.prof_info(info, n, v.data(), ret.data()) ? R(ret) : R{};
}

auto wait(
    const Compute &c,
    const nngn::lua::as_table_t<std::vector<Compute::Event>> &events
) {
    const auto &v = events.value();
    return c.wait(v.size(), v.data());
}

template<auto Compute::*f>
bool release(Compute &c, std::uint32_t id) { return (c.*f)({{id}}); }

bool release_events(
    const Compute &c,
    nngn::lua::as_table_t<std::vector<Compute::Event>> events
) {
    const auto &v = events.value();
    return c.release_events(v.size(), v.data());
}

std::vector<std::size_t> data_size(nngn::lua::table_view t) {
    const auto n = static_cast<std::size_t>(t.size());
    std::vector<std::size_t> ret;
    ret.reserve(n / 2);
    for(std::size_t i = 1; i <= n;) {
        const auto type = nngn::chain_cast<Type, lua_Integer>(t[i++]);
        if(type == Type::DATA)
            ret.push_back(t[i][1].get<std::size_t>());
        else if(type == Type::LOCAL || Compute::is_handle_type(type))
            ret.push_back(sizeof(std::uint32_t));
        else
            map(
                type,
                [&ret]<typename T>(T) { ret.push_back(sizeof(T)); },
                [&ret, &t, i]<typename T>(T) {
                    if constexpr(std::is_same_v<T, std::byte>)
                        if(const auto v =
                                t.get<std::optional<std::string_view>>(i))
                            return ret.push_back(v->size());
                    ret.push_back(
                        sizeof(T) * static_cast<std::size_t>(
                            t.get<nngn::lua::table>(i).size()));
                },
                [type](auto) { invalid_type(type); });
        ++i;
    }
    return ret;
}

auto read_data(nngn::lua::table_view t, auto *data_v) {
    const auto n = static_cast<std::size_t>(t.size());
    std::vector<Type> types;
    std::vector<const std::byte*> ret;
    types.reserve(n / 2);
    ret.reserve(n / 2);
    auto *p = data_v->data();
    for(std::size_t si = 0, i = 1, e = n; i <= e; ++si) {
        const auto type = types.emplace_back(
            nngn::chain_cast<Type, lua_Integer>(t[i++]));
        if(type == Type::DATA)
            ret.push_back(nngn::as_bytes(t[i][2].get<const void*>()));
        else if(type == Type::LOCAL || Compute::is_handle_type(type))
            ret.push_back(
                std::exchange(p,
                    write_scalar_type<std::uint32_t>(t, i, p)));
        else
            map(
                type,
                [&ret, &p, &t, i]<typename T>(T) {
                    ret.push_back(
                        std::exchange(p, write_scalar_type<T>(t, i, p)));
                },
                [&ret, &p, &t, i]<typename T>(T) {
                    ret.push_back(
                        std::exchange(p, write_vector_type<T>(t, i, p)));
                },
                [type](auto) { invalid_type(type); });
        ++i;
    }
    return std::tuple{types, ret};
}

auto create_kernel(
    Compute &c, u32 program, const std::string &func,
    nngn::lua::table_view data,
    std::optional<nngn::lua::object> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    NNGN_LOG_CONTEXT_F();
    const auto size = data_size(data);
    std::vector<std::byte> data_v(nngn::reduce(size));
    const auto [types, data_p] = read_data(data, &data_v);
    Compute::Events events = {};
    const auto [wait_v, events_v] =
        read_events(c, &events, types, wait_opt, events_opt);
    const auto ret = c.create_kernel(
        {{program}}, func.data(), static_cast<std::size_t>(data.size() / 2),
        types.data(), size.data(), data_p.data(), events);
    return ret
        ? (write_events(events_v, events_opt), opt(ret))
        : opt({});
}

bool execute_kernel(
    Compute &c, u32 kernel,
    Compute::ExecFlag flags,
    nngn::lua::as_table_t<std::vector<std::size_t>> global_size,
    nngn::lua::as_table_t<std::vector<std::size_t>> local_size,
    std::optional<nngn::lua::object> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    const auto global_size_v = global_size.value();
    Compute::Events events = {};
    const auto [wait_v, events_v] =
        read_events(c, &events, {}, wait_opt, events_opt);
    if(!c.execute(
            {{kernel}}, flags, static_cast<u32>(global_size_v.size()),
            global_size_v.data(), local_size.value().data(), events))
        return false;
    write_events(events_v, events_opt);
    return true;
}

bool execute(
    Compute &c, u32 program, const std::string &func,
    Compute::ExecFlag flags,
    nngn::lua::table_view global_size,
    nngn::lua::table_view local_size,
    nngn::lua::table_view data,
    std::optional<nngn::lua::object> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    NNGN_LOG_CONTEXT_F();
    const auto global_size_v = from_table<std::size_t>(global_size);
    const auto local_size_v = from_table<std::size_t>(local_size);
    const auto size = data_size(data);
    std::vector<std::byte> data_v(nngn::reduce(size));
    const auto [types, data_p] = read_data(data, &data_v);
    Compute::Events events = {};
    const auto [wait_v, events_v] =
        read_events(c, &events, types, wait_opt, events_opt);
    if(!c.execute(
            {{program}}, func, flags, static_cast<u32>(global_size_v.size()),
            global_size_v.data(), local_size_v.data(),
            static_cast<std::size_t>(data.size() / 2),
            types.data(), size.data(), data_p.data(), events))
        return false;
    write_events(events_v, events_opt);
    return true;
}

std::optional<Compute::OpenCLParameters> opencl_params(
    nngn::lua::table_view t
) {
    NNGN_LOG_CONTEXT_F();
    const nngn::lua::state_view lua = t.state();
    Compute::OpenCLParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ko = k.as<std::optional<std::string_view>>();
        if(!ko) {
            nngn::Log::l() << "only string keys are allowed\n";
            return std::nullopt;
        }
        const auto ks = *ko;
        if(ks == "version") {
            const nngn::lua::table tt = nngn::lua::push(lua, v);
            ret.version = {.major = tt[1], .minor = tt[2]};
        } else if(ks == "debug")
            ret.debug = v.as<bool>();
        else if(ks == "preferred_device")
            ret.preferred_device = v.as<Compute::DeviceType>();
    }
    return {ret};
}

}

using nngn::lua::var;

NNGN_LUA_PROXY(Compute,
    "SIZEOF_INT", var(sizeof(std::int32_t)),
    "SIZEOF_UINT", var(sizeof(std::uint32_t)),
    "SIZEOF_FLOAT", var(sizeof(float)),
    "PSEUDOCOMP", var(Compute::Backend::PSEUDOCOMP),
    "OPENCL_BACKEND", var(Compute::Backend::OPENCL_BACKEND),
    "DEVICE_TYPE_GPU", var(Compute::DeviceType::GPU),
    "DEVICE_TYPE_CPU", var(Compute::DeviceType::CPU),
    "LOCAL", var(Type::LOCAL),
    "BYTE", var(Type::BYTE),
    "INT", var(Type::INT),
    "UINT", var(Type::UINT),
    "FLOAT", var(Type::FLOAT),
    "BYTEV", var(Type::BYTEV),
    "INTV", var(Type::INTV),
    "UINTV", var(Type::UINTV),
    "FLOATV", var(Type::FLOATV),
    "DATA", var(Type::DATA),
    "BUFFER", var(Type::BUFFER),
    "IMAGE", var(Type::IMAGE),
    "SAMPLER", var(Type::SAMPLER),
    "READ_ONLY", var(Compute::MemFlag::READ_ONLY),
    "WRITE_ONLY", var(Compute::MemFlag::WRITE_ONLY),
    "READ_WRITE", var(Compute::MemFlag::READ_WRITE),
    "BLOCKING", var(Compute::ExecFlag::BLOCKING),
    "COMPUTE_UNITS", var(Compute::Limit::COMPUTE_UNITS),
    "WORK_GROUP_SIZE", var(Compute::Limit::WORK_GROUP_SIZE),
    "LOCAL_MEMORY", var(Compute::Limit::LOCAL_MEMORY),
    "QUEUED", var(Compute::ProfInfo::QUEUED),
    "SUBMIT", var(Compute::ProfInfo::SUBMIT),
    "START", var(Compute::ProfInfo::START),
    "END", var(Compute::ProfInfo::END),
    "PROF_INFO_ALL", var(Compute::ProfInfo::PROF_INFO_ALL),
    "opencl_params", opencl_params,
    "get_limits", [](Compute &c) {
        return nngn::lua::as_table(c.get_limits());
    },
    "platform_name", &Compute::platform_name,
    "device_name", &Compute::device_name,
    "create_vector", [](std::size_t n) { return std::vector<std::byte>(n); },
    "vector_size", [](std::vector<std::byte> &v) { return v.size(); },
    "vector_data", [](std::vector<std::byte> &v)
        { return static_cast<void*>(v.data()); },
    "read_vector", read_vector,
    "zero_vector", zero_vector,
    "fill_vector", fill_vector,
    "write_vector", write_vector,
    "to_bytes", to_bytes,
    "create_buffer", create_buffer,
    "read_buffer", read_buffer,
    "fill_buffer", fill_buffer,
    "write_buffer", write_buffer,
    "release_buffer", release<&Compute::release_buffer>,
    "create_image", create_image,
    "read_image", read_image,
    "fill_image", fill_image,
    "release_image", release<&Compute::release_image>,
    "create_sampler", create_sampler,
    "release_sampler", release<&Compute::release_sampler>,
    "create_program", create_program,
    "release_program", release<&Compute::release_program>,
    "create_kernel", create_kernel,
    "release_kernel", release<&Compute::release_kernel>,
    "prof_info", prof_info,
    "wait", wait,
    "release_events", release_events,
    "execute_kernel", execute_kernel,
    "execute", execute)
