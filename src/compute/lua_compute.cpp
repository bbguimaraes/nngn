#include <algorithm>
#include <bit>
#include <cstddef>

#include <sol/state_view.hpp>
#include <sol/usertype_proxy.hpp>

#include "luastate.h"

#include "utils/log.h"
#include "utils/utils.h"
#include "utils/vector.h"

#include "compute.h"

using nngn::Compute;
using Type = nngn::Compute::Type;

namespace {

void invalid_type(Type t)
    { nngn::Log::l() << "invalid type: " << static_cast<unsigned>(t) << '\n'; }

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

bool check_size(std::size_t n, const std::vector<std::byte> &v)
    { return check_size(v.data(), n, v); }

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

template<typename T>
auto read_table(const auto &t, std::size_t i, std::size_t n, std::byte *p) {
    for(n += i; i < n; ++i, p += sizeof(T)) {
        const auto v = t.template raw_get<T>(i);
        std::memcpy(p, &v, sizeof(T));
    }
    return p;
}

auto write_table(auto *t, std::size_t i, std::size_t n, const auto *p) {
    for(n += i; i < n; ++i, ++p)
        t->raw_set(i, *p);
    return p;
}

template<typename T>
auto write_table(auto *t, std::size_t i, std::size_t n, const std::byte *p) {
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

bool to_bytes(Type type, const sol::stack_table &t, std::vector<std::byte> *v) {
    NNGN_LOG_CONTEXT_F();
    return map_vector(type, false, [&t, v]<typename T>(T) {
        const auto n = t.size();
        return check_size(n, *v)
            && (read_table<T>(t, 1, n, v->data()), true);
    });
}

auto read_vector(
    sol::this_state sol, const std::vector<std::byte> &v,
    std::size_t off, std::size_t n, Type type
) {
    NNGN_LOG_CONTEXT_F();
    using R = sol::table;
    using O = std::optional<sol::table>;
    return map_vector(type, O{}, [&sol, &v, off, n]<typename T>(T) mutable {
        constexpr auto size = sizeof(T);
        if(!n) {
            if(v.size() % size)
                return nngn::Log::l() << "invalid vector size for type\n", O{};
            n = v.size() / size;
        }
        R ret = {sol, sol::new_table(static_cast<int>(n))};
        write_table<T>(&ret, 1, n, v.data() + off * size);
        return O{ret};
    });
}

bool fill_vector(
    std::vector<std::byte> *v, std::size_t off, std::size_t n,
    Type type, const sol::stack_table &t
) {
    NNGN_LOG_CONTEXT_F();
    std::vector<std::byte> tv(t.size());
    if(!to_bytes(type, t, &tv))
        return false;
    const auto i = begin(*v) + static_cast<std::ptrdiff_t>(off);
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
auto write_vector_type(const auto &t, std::size_t i, std::byte *p) {
    if constexpr(std::is_same_v<T, std::byte>)
        if(const std::optional<std::string_view> v = t[i]) {
            const auto n = v->size();
            std::memcpy(p, v->data(), n);
            return p + n;
        }
    const sol::table tt = t[i];
    return read_table<T>(tt, 1, tt.size(), p);
}

bool write_vector(
    std::vector<std::byte> *v, std::size_t off, const sol::stack_table &t
) {
    NNGN_LOG_CONTEXT_F();
    auto *p = v->data() + off;
    for(std::size_t i = 1, n = t.size(); i <= n;) {
        const Type type = t[i++];
        if(type == Type::DATA) {
            const sol::table tt = t[i++];
            const auto size = tt.get_or(1, std::size_t{});
            if(!check_size(p, size, *v))
                return false;
            std::memcpy(p, tt[2], size);
            p += size;
            continue;
        }
        if(!map(
            type,
            [v, &t, &p, &i]<typename T>(T) {
                return check_size(p, sizeof(T), *v)
                    && (p = write_scalar_type<T>(t, i++, p), true);
            },
            [v, &t, &p, &i]<typename T>(T) {
                const sol::table tt = t[i++];
                const auto nt = tt.size();
                return check_size(p, nt, *v)
                    && (p = read_table<T>(tt, 1, nt, p));
            },
            [type](auto) { invalid_type(type); return false; }
        ))
            return false;
    }
    return true;
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
    const auto *const v = *ov;
    if(!n)
        return ov
            ? opt(c.create_buffer(flags, v->size(), v->data()))
            : opt(c.create_buffer(flags, 0, nullptr));
    const auto size = vector_size(type);
    return size
        ? opt(c.create_buffer(flags, n * size, nullptr))
        : opt({});
}

bool read_buffer(
    const Compute &c, std::uint32_t b, Type type, std::size_t n,
    std::vector<std::byte> *v
) {
    NNGN_LOG_CONTEXT_F();
    const auto size = n * vector_size(type);
    return size
        && check_size(size, *v)
        && c.read_buffer({{b}}, 0, size, v->data(), {});
}

bool fill_buffer(
    const Compute &c, std::uint32_t b, std::size_t off, std::size_t n,
    Type type, const sol::stack_table &t
) {
    NNGN_LOG_CONTEXT_F();
    std::vector<std::byte> v(t.size());
    if(!to_bytes(type, t, &v))
        return false;
    const auto size = vector_size(type);
    return size
        && c.fill_buffer({{b}}, off * size, n * size, v.size(), v.data(), {});
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
    Type type, std::vector<std::byte> *v
) {
    NNGN_LOG_CONTEXT_F();
    constexpr std::size_t channels = 4;
    const auto size = w * h * vector_size(type);
    return size
        && check_size(channels * size, *v)
        && c.read_image({{i}}, w, h, v->data(), {});
}

bool fill_image(
    const Compute &c, std::uint32_t i, std::size_t w, std::size_t h,
    Type type, const sol::stack_table &t
) {
    NNGN_LOG_CONTEXT_F();
    const auto size = vector_size(type);
    if(!size)
        return false;
    std::vector<std::byte> v(size * t.size());
    return to_bytes(type, t, &v)
        && c.fill_image({{i}}, w, h, v.data(), {});
}

auto create_sampler(Compute &c) { return opt(c.create_sampler()); }
auto create_program(Compute &c, std::string_view src, const char *opts)
    { return opt(c.create_program(src, opts)); }

auto prof_info(
    const Compute &c, Compute::ProfInfo info,
    const sol::as_table_t<std::vector<Compute::Event>> &events
) {
    NNGN_LOG_CONTEXT_F();
    const auto &v = events.value();
    const auto n = v.size();
    const auto pc = static_cast<std::size_t>(
        std::popcount(
            static_cast<std::underlying_type_t<Compute::ProfInfo>>(info)));
    std::vector<uint64_t> ret(n * pc);
    using R = sol::as_table_t<decltype(ret)>;
    return c.prof_info(info, n, v.data(), ret.data()) ? R(ret) : R{};
}

auto wait(
    const Compute &c,
    const sol::as_table_t<std::vector<Compute::Event>> &events
) {
    const auto &v = events.value();
    return c.wait(v.size(), v.data());
}

template<auto Compute::*f>
bool release(Compute &c, std::uint32_t id) { return (c.*f)({{id}}); }

bool release_events(
    const Compute &c,
    sol::as_table_t<std::vector<Compute::Event>> events
) {
    const auto &v = events.value();
    return c.release_events(v.size(), v.data());
}

bool execute(
    Compute &c, std::uint32_t program, const std::string &func,
    Compute::ExecFlag flags,
    sol::as_table_t<std::vector<std::size_t>> global_size,
    sol::as_table_t<std::vector<std::size_t>> local_size,
    const sol::stack_table &data,
    std::optional<sol::object> wait_opt,
    std::optional<sol::stack_table> events_opt
) {
    NNGN_LOG_CONTEXT_F();
    const auto data_size = [](const sol::stack_table &t) {
        const auto n = t.size();
        std::vector<std::size_t> ret;
        ret.reserve(n / 2);
        for(std::size_t i = 1; i <= n;) {
            const Type type = t[i++];
            if(type == Type::DATA)
                ret.push_back(t[i][1].get_or(std::size_t{}));
            else if(type == Type::LOCAL || Compute::is_handle_type(type))
                ret.push_back(sizeof(std::uint32_t));
            else
                map(
                    type,
                    [&ret]<typename T>(T) { ret.push_back(sizeof(T)); },
                    [&ret, &t, i]<typename T>(T) {
                        if constexpr(std::is_same_v<T, std::byte>)
                            if(const std::optional<std::string_view> v = t[i])
                                return ret.push_back(v->size());
                        ret.push_back(sizeof(T) * t.get<sol::table>(i).size());
                    },
                    [type](auto) { invalid_type(type); });
            ++i;
        }
        return ret;
    };
    const auto read_data = [](const sol::stack_table &t, auto *data_v) {
        const auto n = t.size();
        std::vector<Type> types;
        std::vector<const std::byte*> ret;
        types.reserve(n / 2);
        ret.reserve(n / 2);
        auto *p = data_v->data();
        for(std::size_t si = 0, i = 1, e = n; i <= e; ++si) {
            const auto type = types.emplace_back(t[i++]);
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
        return std::tuple(types, ret);
    };
    const auto global_size_v = global_size.value();
    const auto size = data_size(data);
    std::vector<std::byte> data_v(nngn::reduce(std::span{size}));
    const auto [types, data_p] = read_data(data, &data_v);
    Compute::Events events = {};
    std::vector<Compute::Event> wait_v = {}, events_v = {};
    if(wait_opt) {
        wait_v = wait_opt->as<sol::as_table_t<std::vector<Compute::Event>>>();
        events.n_wait = wait_v.size();
        events.wait_list = wait_v.data();
    }
    if(events_opt) {
        events_v.resize(1 + c.n_events(types.size(), types.data()));
        events.events = events_v.data();
    }
    if(!c.execute(
            {{program}}, func, flags,
            static_cast<std::uint32_t>(global_size_v.size()),
            global_size_v.data(), local_size.value().data(),
            data.size() / 2, types.data(), size.data(), data_p.data(), events))
        return false;
    if(auto &x = *events_opt; events_opt)
        for(std::size_t i = 0, e = events_v.size(), b = x.size(); i < e; ++i)
            x.raw_set(b + i + 1, events_v[i]);
    return true;
}

std::optional<Compute::OpenCLParameters> opencl_params(sol::stack_table t) {
    NNGN_LOG_CONTEXT_F();
    Compute::OpenCLParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ko = k.as<std::optional<std::string_view>>();
        if(!ko) {
            nngn::Log::l() << "only string keys are allowed\n";
            return std::nullopt;
        }
        const auto ks = *ko;
        if(ks == "debug")
            ret.debug = v.as<bool>();
        else if(ks == "preferred_device")
            ret.preferred_device = v.as<Compute::DeviceType>();
    }
    return {ret};
}

}

NNGN_LUA_PROXY(Compute,
    sol::no_constructor,
    "SIZEOF_INT", sol::var(sizeof(std::int32_t)),
    "SIZEOF_UINT", sol::var(sizeof(std::uint32_t)),
    "SIZEOF_FLOAT", sol::var(sizeof(float)),
    "PSEUDOCOMP", sol::var(Compute::Backend::PSEUDOCOMP),
    "OPENCL_BACKEND", sol::var(Compute::Backend::OPENCL_BACKEND),
    "DEVICE_TYPE_GPU", sol::var(Compute::DeviceType::GPU),
    "DEVICE_TYPE_CPU", sol::var(Compute::DeviceType::CPU),
    "LOCAL", sol::var(Type::LOCAL),
    "BYTE", sol::var(Type::BYTE),
    "INT", sol::var(Type::INT),
    "UINT", sol::var(Type::UINT),
    "FLOAT", sol::var(Type::FLOAT),
    "BYTEV", sol::var(Type::BYTEV),
    "INTV", sol::var(Type::INTV),
    "UINTV", sol::var(Type::UINTV),
    "FLOATV", sol::var(Type::FLOATV),
    "DATA", sol::var(Type::DATA),
    "BUFFER", sol::var(Type::BUFFER),
    "IMAGE", sol::var(Type::IMAGE),
    "SAMPLER", sol::var(Type::SAMPLER),
    "READ_ONLY", sol::var(Compute::MemFlag::READ_ONLY),
    "WRITE_ONLY", sol::var(Compute::MemFlag::WRITE_ONLY),
    "READ_WRITE", sol::var(Compute::MemFlag::READ_WRITE),
    "BLOCKING", sol::var(Compute::ExecFlag::BLOCKING),
    "COMPUTE_UNITS", sol::var(Compute::Limit::COMPUTE_UNITS),
    "WORK_GROUP_SIZE", sol::var(Compute::Limit::WORK_GROUP_SIZE),
    "LOCAL_MEMORY", sol::var(Compute::Limit::LOCAL_MEMORY),
    "QUEUED", sol::var(Compute::ProfInfo::QUEUED),
    "SUBMIT", sol::var(Compute::ProfInfo::SUBMIT),
    "START", sol::var(Compute::ProfInfo::START),
    "END", sol::var(Compute::ProfInfo::END),
    "PROF_INFO_ALL", sol::var(Compute::ProfInfo::PROF_INFO_ALL),
    "opencl_params", opencl_params,
    "get_limits", [](Compute &c) { return sol::as_table(c.get_limits()); },
    "platform_name", &Compute::platform_name,
    "device_name", &Compute::device_name,
    "create_vector", [](std::size_t n) { return std::vector<std::byte>(n); },
    "vector_data", [](std::vector<std::byte> *v)
        { return static_cast<void*>(v->data()); },
    "read_vector", read_vector,
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
    "prof_info", prof_info,
    "wait", wait,
    "release_events", release_events,
    "execute", execute)
