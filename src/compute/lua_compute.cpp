#include <algorithm>
#include <bit>
#include <cstddef>

#include "lua/function.h"
#include "lua/iter.h"
#include "lua/register.h"
#include "lua/table.h"
#include "math/lua_vector.h"
#include "utils/literals.h"
#include "utils/log.h"
#include "utils/ranges.h"
#include "utils/utils.h"

#include "compute.h"

using namespace nngn::literals;
using nngn::i16, nngn::i32, nngn::u32, nngn::u64, nngn::Compute;

using bvec = nngn::lua_vector<std::byte>;
using Type = nngn::Compute::Type;

NNGN_LUA_DECLARE_USER_TYPE(nngn::Compute::OpenCLParameters, "OpenCLParameters")

namespace {

template<typename T>
T from_table_array(nngn::lua::table_view t) {
    const auto n = t.size();
    T ret = {};
    ret.reserve(nngn::narrow<std::size_t>(n));
    for(lua_Integer i = 1; i <= n; ++i)
        ret.push_back(t[i]);
    return ret;
}

void invalid_type(Type t) {
    nngn::Log::l() << "invalid type: " << static_cast<unsigned>(t) << '\n';
}

bool check_size(
    const std::byte *p, lua_Integer n, std::span<const std::byte> s)
{
    assert(nngn::in_range(s, *p));
    if(!nngn::in_range(s, p[nngn::narrow<std::ptrdiff_t>(n)])) {
        nngn::Log::l()
            << "insufficient size: "
            << s.size() << " < " << n << '\n';
        return false;
    }
    return true;
}

bool check_size(lua_Integer n, const bvec &v) {
    return check_size(v.data(), n, v);
}

auto elem_size(Type t) {
    constexpr std::array v = {
        lua_Integer{}, // NONE
        lua_Integer{}, // LOCAL
        lua_Integer{}, // BYTE
        lua_Integer{}, // INT
        lua_Integer{}, // UINT
        lua_Integer{}, // FLOAT
        static_cast<lua_Integer>(sizeof(std::byte)), // BYTEV
        static_cast<lua_Integer>(sizeof(i32)), // INTV
        static_cast<lua_Integer>(sizeof(u32)), // UINTV
        static_cast<lua_Integer>(sizeof(float)), // FLOATV
        lua_Integer{}, // DATA
        lua_Integer{}, // BUFFER
        lua_Integer{}, // IMAGE
        lua_Integer{}, // SAMPLER
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
auto read_table(const auto &t, lua_Integer i, lua_Integer n, std::byte *p) {
    for(n += i; i < n; ++i, p += sizeof(T)) {
        const auto v = t.template raw_get<T>(i);
        std::memcpy(p, &v, sizeof(T));
    }
    return p;
}

template<typename T>
auto write_table(
    nngn::lua::table_view t,
    lua_Integer i, lua_Integer n, const T *p
) {
    for(n += i; i < n; ++i, ++p)
        if constexpr(std::same_as<T, float>)
            t.raw_set(i, nngn::narrow<lua_Number>(*p));
        else
            t.raw_set(i, *p);
    return p;
}

template<typename T>
auto write_table(
    nngn::lua::table_view t, lua_Integer i, lua_Integer n, const std::byte *p
) {
    return nngn::as_bytes(write_table(t, i, n, nngn::byte_cast<const T*>(p)));
}

decltype(auto) map(Type t, auto &&sf, auto &&vf, auto &&of) {
    switch(t) {
    case Type::BYTE: return FWD(sf)(std::byte{});
    case Type::INT: return FWD(sf)(i32{});
    case Type::UINT: return FWD(sf)(u32{});
    case Type::FLOAT: return FWD(sf)(float{});
    case Type::BYTEV: return FWD(vf)(std::byte{});
    case Type::INTV: return FWD(vf)(i32{});
    case Type::UINTV: return FWD(vf)(u32{});
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

bool to_bytes(Type type, nngn::lua::table_view t, bvec *v) {
    NNGN_LOG_CONTEXT_F();
    return map_vector(type, false, [&t, v]<typename T>(T) {
        const auto n = t.size();
        return check_size(n, *v)
            && (read_table<T>(t, 1, n, v->data()), true);
    });
}

auto get_limits(Compute &c, nngn::lua::state_view lua) {
    const auto v = c.get_limits();
    const auto n = v.size();
    auto ret = lua.create_table(nngn::narrow<int>(n), 0);
    for(std::size_t i = 0; i != n; ++i)
        ret.raw_set(
            nngn::narrow<lua_Integer>(i + 1),
            nngn::narrow<lua_Integer>(v[i]));
    return ret.release();
}

auto create_vector(lua_Integer n) {
    return bvec(nngn::narrow<std::size_t>(n));
}

auto vector_size(const bvec &v) {
    return nngn::narrow<lua_Integer>(v.size());
}

auto read_vector(
    bvec &v, lua_Integer off, lua_Integer n, Type type,
    nngn::lua::state_view lua)
{
    NNGN_LOG_CONTEXT_F();
    using R = nngn::lua::table_view;
    using O = std::optional<R>;
    return map_vector(type, O{}, [lua, &v, off, n]<typename T>(T) mutable {
        constexpr auto size = sizeof(T);
        if(!n) {
            if(v.size() % size)
                return nngn::Log::l() << "invalid vector size for type\n", O{};
            n = nngn::narrow<lua_Integer>(v.size() / size);
        }
        auto ret = lua.create_table(nngn::narrow<int>(n), 0);
        write_table<T>(
            ret, 1, n, v.data() + nngn::narrow<std::size_t>(off) * size);
        return O{ret.release()};
    });
}

void zero_vector(bvec *v, lua_Integer off, lua_Integer n) {
    std::memset(v->data() + off, 0, nngn::narrow<std::size_t>(n));
}

bool fill_vector(
    bvec *v, lua_Integer off, lua_Integer n,
    Type type, nngn::lua::table_view t
) {
    NNGN_LOG_CONTEXT_F();
    bvec tv(static_cast<std::size_t>(t.size()));
    if(!to_bytes(type, t, &tv))
        return false;
    const auto i = begin(*v) + nngn::narrow<std::ptrdiff_t>(off);
    nngn::fill_with_pattern(
        begin(tv), end(tv), i, i + nngn::narrow<std::ptrdiff_t>(n));
    return true;
}

void copy_vector(
    bvec *dst, const bvec &src,
    lua_Integer dst_off, lua_Integer src_off, lua_Integer n)
{
    NNGN_LOG_CONTEXT_F();
    std::memcpy(
        dst->data() + nngn::narrow<std::ptrdiff_t>(dst_off),
        src.data()  + nngn::narrow<std::ptrdiff_t>(src_off),
        nngn::narrow<std::size_t>(n));
}

template<typename T>
auto write_scalar_type(const auto &t, lua_Integer i, std::byte *p) {
    const T v = t[i];
    std::memcpy(p, &v, sizeof(T));
    return p + sizeof(T);
}

template<typename T>
auto write_vector_type(nngn::lua::table_view t, lua_Integer i, std::byte *p) {
    if constexpr(std::is_same_v<T, std::byte>)
        if(const auto v = t[i].get<std::optional<std::string_view>>()) {
            const auto n = v->size();
            std::memcpy(p, v->data(), n);
            return p + n;
        }
    const nngn::lua::table tt = t[i];
    return read_table<T>(tt, 1, tt.size(), p);
}

bool write_vector(bvec *v, lua_Integer off, nngn::lua::table_view t) {
    NNGN_LOG_CONTEXT_F();
    auto *p = v->data() + off;
    for(lua_Integer i = 1, n = t.size(); i <= n;) {
        const auto type = nngn::chain_cast<Type, lua_Integer>(t[i++]);
        if(type == Type::DATA) {
            const nngn::lua::table tt = t[i++];
            const auto size = tt[1].get<lua_Integer>();
            if(!check_size(p, size, *v))
                return false;
            std::memcpy(p, tt[2], nngn::narrow<std::size_t>(size));
            p += size;
            continue;
        }
        const auto scalar = [v, &t, &p, &i]<typename T>(T) {
            return check_size(p, sizeof(T), *v)
                && (p = write_scalar_type<T>(t, i++, p), true);
        };
        const auto vector = [v, &t, &p, &i]<typename T>(T) {
            const nngn::lua::table tt = t[i++];
            const auto nt = tt.size();
            return check_size(p, nt, *v)
                && (p = read_table<T>(tt, 1, nt, p));
        };
        const auto invalid = [type](auto) { invalid_type(type); return false; };
        if(!map(type, scalar, vector, invalid))
            return false;
    }
    return true;
}

auto read_sizes(nngn::lua::table_view t) {
    std::vector<std::size_t> ret = {};
    ret.reserve(nngn::narrow<std::size_t>(t.size()));
    for(auto [_, x] : ipairs(t))
        ret.push_back(nngn::narrow<std::size_t>(x.get<lua_Integer>()));
    return ret;
}

auto read_events(
    const Compute &c,
    Compute::Events *events,
    std::span<const Compute::Type> types,
    std::optional<nngn::lua::value_view> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    std::pair ret = {
        std::vector<const Compute::Event*>{},
        std::vector<Compute::Event*>{},
    };
    auto &[w, e] = ret;
    if(wait_opt) {
        const auto v = wait_opt->get<std::vector<const void*>>();
        w.resize(v.size());
        std::transform(
            begin(v), end(v), begin(w),
            nngn::to<const Compute::Event*>{});
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
    std::span<const Compute::Event *const> events,
    std::optional<nngn::lua::table_view> events_opt
) {
    if(!events_opt)
        return;
    auto &x = *events_opt;
    const auto e = static_cast<std::size_t>(events.size());
    const auto b = static_cast<std::size_t>(x.size());
    for(std::size_t i = 0; i < e; ++i)
        x.raw_set(
            nngn::narrow<lua_Integer>(b + i) + 1,
            static_cast<const void*>(events[i]));
}

auto opt(const Compute::Handle &h) {
    using O = std::optional<u32>;
    return h.id ? O{h.id} : O{};
}

auto create_buffer(
    Compute &c, Compute::MemFlag flags, Type type, lua_Integer n,
    const bvec *ov)
{
    NNGN_LOG_CONTEXT_F();
    if(!n)
        return ov
            ? opt(c.create_buffer(flags, ov->size(), ov->data()))
            : opt(c.create_buffer(flags, 0, nullptr));
    const auto size = elem_size(type);
    return size
        ? opt(c.create_buffer(
            flags, nngn::narrow<std::size_t>(n * size),
            ov ? ov->data() : nullptr))
        : opt({});
}

bool read_buffer(
    const Compute &c,
    lua_Integer b, Type type, lua_Integer n, bvec *v)
{
    NNGN_LOG_CONTEXT_F();
    const auto size = n * elem_size(type);
    return size
        && check_size(size, *v)
        && c.read_buffer(
            {{nngn::narrow<u32>(b)}},
            0, nngn::narrow<std::size_t>(size), v->data(), {});
}

bool fill_buffer(
    const Compute &c, lua_Integer b, lua_Integer off, lua_Integer n, Type type,
    nngn::lua::table_view pattern_t,
    std::optional<nngn::lua::value_view> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    NNGN_LOG_CONTEXT_F();
    bvec pattern(static_cast<std::size_t>(pattern_t.size()));
    if(!to_bytes(type, pattern_t, &pattern))
        return false;
    const auto size = elem_size(type);
    if(!size)
        return false;
    Compute::Events events = {};
    auto [wait_v, events_v] = read_events(c, &events, {}, wait_opt, events_opt);
    if(!c.fill_buffer(
            {{nngn::narrow<u32>(b)}},
            nngn::narrow<std::size_t>(off * size),
            nngn::narrow<std::size_t>(n * size),
            pattern.size(), pattern.data(), events))
        return false;
    write_events(events_v, events_opt);
    return true;
}

bool write_buffer(
    const Compute &c, lua_Integer b, lua_Integer off, lua_Integer n, Type type,
    const bvec &v)
{
    NNGN_LOG_CONTEXT_F();
    const auto size = elem_size(type);
    return size
        && c.write_buffer(
            {{nngn::narrow<u32>(b)}},
            nngn::narrow<std::size_t>(off * size),
            nngn::narrow<std::size_t>(n * size),
            v.data(), {});
}

auto create_image(
    Compute &c, Type type, lua_Integer w, lua_Integer h,
    Compute::MemFlag flags, const bvec *ov)
{
    NNGN_LOG_CONTEXT_F();
    return opt(c.create_image(
        type, nngn::narrow<std::size_t>(w), nngn::narrow<std::size_t>(h),
        flags, ov ? ov->data() : nullptr));
}

auto read_image(
    const Compute &c,
    lua_Integer i, lua_Integer w, lua_Integer h, Type type, bvec *v)
{
    NNGN_LOG_CONTEXT_F();
    constexpr lua_Integer channels = 4;
    const auto size = w * h * elem_size(type);
    return size
        && check_size(channels * size, *v)
        && c.read_image(
            {{nngn::narrow<u32>(i)}},
            nngn::narrow<std::size_t>(w),
            nngn::narrow<std::size_t>(h),
            v->data(), {});
}

bool fill_image(
    const Compute &c, lua_Integer i, lua_Integer w, lua_Integer h, Type type,
    nngn::lua::table_view t)
{
    NNGN_LOG_CONTEXT_F();
    const auto size = elem_size(type);
    if(!size)
        return false;
    bvec v(nngn::narrow<std::size_t>(size * t.size()));
    return to_bytes(type, t, &v)
        && c.fill_image(
            {{nngn::narrow<u32>(i)}},
            nngn::narrow<std::size_t>(w),
            nngn::narrow<std::size_t>(h),
            v.data(), {});
}

auto create_sampler(Compute &c) {
    return opt(c.create_sampler());
}

auto create_program(Compute &c, std::string_view src, const char *opts) {
    return opt(c.create_program(src, opts));
}

std::optional<nngn::lua::table_view> prof_info(
    const Compute &c, Compute::ProfInfo info,
    std::vector<const void*> events,
    nngn::lua::state_view lua)
{
    NNGN_LOG_CONTEXT_F();
    const auto n = events.size();
    const auto pc = nngn::narrow<std::size_t>(
        std::popcount(nngn::to_underlying(info)));
    std::vector<u64> tmp(n * pc);
    if(!c.prof_info(
        info, n, reinterpret_cast<const Compute::Event**>(events.data()),
        tmp.data()
    ))
        return {};
    std::vector<lua_Integer> ret(tmp.size());
    std::transform(begin(tmp), end(tmp), begin(ret), nngn::to<lua_Integer>{});
    return nngn::lua::table_from_range(lua, std::move(ret)).release();
}

auto wait(const Compute &c, std::vector<const void*> events) {
    return c.wait(
        events.size(),
        reinterpret_cast<const Compute::Event**>(events.data()));
}

template<auto Compute::*f>
bool release(Compute &c, lua_Integer id) {
    return (c.*f)({{nngn::narrow<u32>(id)}});
}

bool release_events(const Compute &c, std::vector<const void*> events) {
    return c.release_events(
        events.size(),
        reinterpret_cast<const Compute::Event**>(events.data()));
}

std::vector<std::size_t> data_size(nngn::lua::table_view t) {
    const auto n = t.size();
    std::vector<std::size_t> ret;
    ret.reserve(nngn::narrow<std::size_t>(n / 2));
    for(lua_Integer i = 1; i <= n;) {
        const auto type = nngn::chain_cast<Type, lua_Integer>(t[i++]);
        if(type == Type::DATA)
            ret.push_back(
                nngn::narrow<std::size_t>(t[i][1].get<lua_Integer>()));
        else if(type == Type::LOCAL || Compute::is_handle_type(type))
            ret.push_back(sizeof(u32));
        else {
            const auto scalar = [&ret]<typename T>(T) {
                ret.push_back(sizeof(T));
            };
            const auto vector = [&ret, &t, i]<typename T>(T) {
                using O = std::optional<std::string_view>;
                if constexpr(std::is_same_v<T, std::byte>)
                    if(const auto v = t[i].get<O>())
                        return ret.push_back(v->size());
                const auto tn = t[i].get<nngn::lua::table>().size();
                ret.push_back(sizeof(T) * nngn::narrow<std::size_t>(tn));
            };
            const auto invalid = [type](auto) { invalid_type(type); };
            map(type, scalar, vector, invalid);
        }
        ++i;
    }
    return ret;
}

auto read_data(nngn::lua::table_view t, auto *data_v) {
    const auto n = t.size();
    std::vector<Type> types;
    std::vector<const std::byte*> ret;
    types.reserve(nngn::narrow<std::size_t>(n / 2));
    ret.reserve(nngn::narrow<std::size_t>(n / 2));
    auto *p = data_v->data();
    for(lua_Integer i = 1, e = n; i <= e;) {
        const auto type = types.emplace_back(
            nngn::chain_cast<Type, lua_Integer>(t[i++]));
        if(type == Type::DATA)
            ret.push_back(nngn::as_bytes(t[i][2].get<const void*>()));
        else if(type == Type::LOCAL || Compute::is_handle_type(type))
            ret.push_back(
                std::exchange(p, write_scalar_type<u32>(t, i, p)));
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
    Compute &c, lua_Integer program, const char *func,
    nngn::lua::table_view data,
    std::optional<nngn::lua::value_view> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    NNGN_LOG_CONTEXT_F();
    const auto size = data_size(data);
    std::vector<std::byte> data_v(
        nngn::narrow<std::size_t>(nngn::reduce(size)));
    const auto [types, data_p] = read_data(data, &data_v);
    Compute::Events events = {};
    const auto [wait_v, events_v] =
        read_events(c, &events, types, wait_opt, events_opt);
    const auto ret = c.create_kernel(
        {{nngn::narrow<u32>(program)}}, func,
        nngn::narrow<std::size_t>(data.size() / 2),
        types.data(), size.data(), data_p.data(), events);
    return ret
        ? (write_events(events_v, events_opt), opt(ret))
        : opt({});
}

bool execute_kernel(
    Compute &c, lua_Integer kernel, Compute::ExecFlag flags,
    nngn::lua::table_view global_size,
    nngn::lua::table_view local_size,
    std::optional<nngn::lua::value_view> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    const auto global_size_v = read_sizes(global_size);
    const auto local_size_v = read_sizes(local_size);
    Compute::Events events = {};
    const auto [wait_v, events_v] =
        read_events(c, &events, {}, wait_opt, events_opt);
    if(!c.execute(
        {{nngn::narrow<u32>(kernel)}}, flags,
        nngn::narrow<u32>(global_size_v.size()),
        global_size_v.data(), local_size_v.data(), events
    ))
        return false;
    write_events(events_v, events_opt);
    return true;
}

bool execute(
    Compute &c, lua_Integer program, const char *func, Compute::ExecFlag flags,
    nngn::lua::table_view global_size,
    nngn::lua::table_view local_size,
    nngn::lua::table_view data,
    std::optional<nngn::lua::value_view> wait_opt,
    std::optional<nngn::lua::table_view> events_opt
) {
    NNGN_LOG_CONTEXT_F();
    const auto global_size_v = read_sizes(global_size);
    const auto local_size_v = read_sizes(local_size);
    const auto size = data_size(data);
    std::vector<std::byte> data_v(nngn::reduce(size));
    const auto [types, data_p] = read_data(data, &data_v);
    Compute::Events events = {};
    const auto [wait_v, events_v] =
        read_events(c, &events, types, wait_opt, events_opt);
    if(!c.execute(
        {{nngn::narrow<u32>(program)}}, func, flags,
        nngn::narrow<u32>(global_size_v.size()),
        global_size_v.data(), local_size_v.data(),
        nngn::narrow<std::size_t>(data.size() / 2),
        types.data(), size.data(), data_p.data(), events
    ))
        return false;
    write_events(events_v, events_opt);
    return true;
}

std::optional<Compute::OpenCLParameters> opencl_params(
    nngn::lua::table_view t)
{
    NNGN_LOG_CONTEXT_F();
    Compute::OpenCLParameters ret = {};
    for(const auto &[k, v] : t) {
        const auto ks = k.get<std::optional<std::string_view>>();
        if(!ks) {
            nngn::Log::l() << "only string keys are allowed\n";
            return {};
        }
        if(*ks == "version") {
            const nngn::lua::table_view tt = v;
            ret.version = {.major = tt[1], .minor = tt[2]};
        } else if(*ks == "debug")
            ret.debug = v.get<bool>();
        else if(*ks == "preferred_device")
            ret.preferred_device = v.get<Compute::DeviceType>();
    }
    return ret;
}

void register_compute(nngn::lua::table_view t) {
    t["SIZEOF_INT"] = nngn::narrow<lua_Integer>(sizeof(i32));
    t["SIZEOF_UINT"] = nngn::narrow<lua_Integer>(sizeof(u32));
    t["SIZEOF_FLOAT"] = nngn::narrow<lua_Integer>(sizeof(float));
    t["SIZEOF_I16"] = nngn::narrow<lua_Integer>(sizeof(i16));
    t["PSEUDOCOMP"] = Compute::Backend::PSEUDOCOMP;
    t["OPENCL_BACKEND"] = Compute::Backend::OPENCL_BACKEND;
    t["DEVICE_TYPE_GPU"] = Compute::DeviceType::GPU;
    t["DEVICE_TYPE_CPU"] = Compute::DeviceType::CPU;
    t["LOCAL"] = Type::LOCAL;
    t["BYTE"] = Type::BYTE;
    t["INT"] = Type::INT;
    t["UINT"] = Type::UINT;
    t["FLOAT"] = Type::FLOAT;
    t["BYTEV"] = Type::BYTEV;
    t["INTV"] = Type::INTV;
    t["UINTV"] = Type::UINTV;
    t["FLOATV"] = Type::FLOATV;
    t["DATA"] = Type::DATA;
    t["BUFFER"] = Type::BUFFER;
    t["IMAGE"] = Type::IMAGE;
    t["SAMPLER"] = Type::SAMPLER;
    t["READ_ONLY"] = Compute::MemFlag::READ_ONLY;
    t["WRITE_ONLY"] = Compute::MemFlag::WRITE_ONLY;
    t["READ_WRITE"] = Compute::MemFlag::READ_WRITE;
    t["BLOCKING"] = Compute::ExecFlag::BLOCKING;
    t["COMPUTE_UNITS"] = Compute::Limit::COMPUTE_UNITS;
    t["WORK_GROUP_SIZE"] = Compute::Limit::WORK_GROUP_SIZE;
    t["LOCAL_MEMORY"] = Compute::Limit::LOCAL_MEMORY;
    t["QUEUED"] = Compute::ProfInfo::QUEUED;
    t["SUBMIT"] = Compute::ProfInfo::SUBMIT;
    t["START"] = Compute::ProfInfo::START;
    t["END"] = Compute::ProfInfo::END;
    t["PROF_INFO_ALL"] = Compute::ProfInfo::PROF_INFO_ALL;
    t["opencl_params"] = opencl_params;
    t["get_limits"] = get_limits;
    t["platform_name"] = &Compute::platform_name;
    t["device_name"] = &Compute::device_name;
    t["create_vector"] = create_vector;
    t["vector_size"] = vector_size;
    t["vector_data"] = [](bvec *v) { return static_cast<void*>(v->data()); };
    t["read_vector"] = read_vector;
    t["zero_vector"] = zero_vector;
    t["fill_vector"] = fill_vector;
    t["copy_vector"] = copy_vector;
    t["write_vector"] = write_vector;
    t["to_bytes"] = to_bytes;
    t["create_buffer"] = create_buffer;
    t["read_buffer"] = read_buffer;
    t["fill_buffer"] = fill_buffer;
    t["write_buffer"] = write_buffer;
    t["release_buffer"] = release<&Compute::release_buffer>;
    t["create_image"] = create_image;
    t["read_image"] = read_image;
    t["fill_image"] = fill_image;
    t["release_image"] = release<&Compute::release_image>;
    t["create_sampler"] = create_sampler;
    t["release_sampler"] = release<&Compute::release_sampler>;
    t["create_program"] = create_program;
    t["release_program"] = release<&Compute::release_program>;
    t["create_kernel"] = create_kernel;
    t["release_kernel"] = release<&Compute::release_kernel>;
    t["prof_info"] = prof_info;
    t["wait"] = wait;
    t["release_events"] = release_events;
    t["execute_kernel"] = execute_kernel;
    t["execute"] = execute;
}

}

NNGN_LUA_DECLARE_USER_TYPE(Compute)
NNGN_LUA_PROXY(Compute, register_compute)
NNGN_LUA_PROXY(Compute::OpenCLParameters)
