/**
 * \dir src/compute
 * \brief Compute back ends.
 *
 * Compute back ends for task execution in heterogeneous compute devices.
 *
 * - OpenCLBackend: main/default OpenCL 1/2 back end.
 * - Pseudocomp: fake back end for testing.
 * - An absent back end is also supported, in which case native CPU code is
 *   used.
 *
 * The following area of the screenshots page shows some of the compute
 * capabilities:
 *
 * https://bbguimaraes.com/nngn/screenshots/compute.html
 *
 * Lua
 * ---
 *
 * The compute back end is exposed to Lua via the `nngn.compute` variable.
 * Compiling a program and executing a kernel is done similarly to the C++ API:
 *
 * ```lua
 * prog = nngn.compute:create_prog(io.open("prog.cl"):read("a"))
 * nngn.compute:execute(prog, "fn", Compute.BLOCKING, {256, 256}, {16, 16}, {
 *     Compute.FLOAT, 3.1415,
 *     Compute.BYTEV, {0, 1, 2, 3},
 * })
 * ```
 *
 * `Compute.BLOCKING` makes the execution synchronous.  Kernels can also be
 * executed concurrently (requires a device with an out-of-order queue) and
 * synchronized using events:
 *
 * ```lua
 * events = {}
 * -- Last two arguments are wait list and output events.
 * nngn.compute:execute(prog, "k0", 0, {1}, {1}, {}, nil, events)
 * -- Reuse output.
 * nngn.compute:execute(prog, "k1", 0, {1}, {1}, {}, nil, events)
 * -- Wait for all previous events, reuse output.
 * nngn.compute:execute(prog, "k2", 0, {1}, {1}, {}, events, events)
 * -- Wait for all previous events and release them.
 * nngn.compute:wait(events)
 * nngn.compute:release_events(events)
 * ```
 *
 * Separate tables can be used to construct more complex task graphs.
 *
 * ### Buffers
 *
 * To create and populate a device buffer:
 *
 * ```lua
 * b = assert(nngn.compute:create_buffer(Compute.READ_WRITE, size))
 * v = Compute.create_vector(size)
 * Compute.fill_rnd_vector(v)
 * assert(nngn.compute:write_buffer(b, v))
 * ```
 *
 * Common operations on buffers are creation:
 *
 * ```lua
 * rw = Compute.READ_WRITE
 * assert(nngn.compute:create_buffer(rw, size))
 * assert(nngn.compute:create_buffer(rw, size, Compute.FLOATV))
 * assert(nngn.compute:create_buffer(rw, v))
 * assert(nngn.compute:create_buffer(rw, v, offset))
 * assert(nngn.compute:create_buffer(rw, v, offset, length))
 * ```
 *
 * writes:
 *
 * ```lua
 * assert(nngn.compute:write_buffer(b, v))
 * assert(nngn.compute:write_buffer(b, v, dst_offset))
 * assert(nngn.compute:write_buffer(b, v, dst_offset, src_offset))
 * assert(nngn.compute:write_buffer(b, v, dst_offset, src_offset, length))
 * ```
 *
 * and reads:
 *
 * ```lua
 * assert(nngn.compute:read_buffer(b, v))
 * assert(nngn.compute:read_buffer(b, v, src_offset))
 * assert(nngn.compute:read_buffer(b, v, src_offset, dst_offset))
 * assert(nngn.compute:read_buffer(b, v, src_offset, dst_offset, length))
 * ```
 */
#ifndef NNGN_COMPUTE_COMPUTE_H
#define NNGN_COMPUTE_COMPUTE_H

#include <array>
#include <cassert>
#include <cstring>
#include <memory>
#include <numeric>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "utils/concepts.h"
#include "utils/def.h"
#include "utils/ranges.h"
#include "utils/utils.h"

namespace nngn {

/**
 * Base class for computation back ends, which may be hardware-accelerated.
 */
struct Compute {
    /** Indicates which type of back end to create. */
    enum class Backend : u8 {
        /**
         * No-op back end which can be used in lieu of a real one.
         * As currently implemented, it is almost always better to not create a
         * back end at all.
         */
        PSEUDOCOMP,
        /** OpenCL 1.2 back end. */
        OPENCL_BACKEND,
    };
    enum class DeviceType : u8 {
        CPU = 1u << 0, GPU = 1u << 1
    };
    struct Version { u32 major = {}, minor = {}; };
    struct OpenCLParameters {
        /** Desired OpenCL version. */
        Version version = {};
        /** Enables debugging, also required for profiling information. */
        bool debug = {};
        /** Prefer this device type on initialization. */
        DeviceType preferred_device = DeviceType::CPU;
    };
    /** Supported parameter types for kernel execution. */
    enum class Type : u8 {
        /** Invalid value. */
        NONE,
        /** Device-local memory. */
        LOCAL,
        SCALAR_BEGIN,
        BYTE = SCALAR_BEGIN, INT, UINT, FLOAT,
        VECTOR_BEGIN,
        BYTEV = VECTOR_BEGIN, INTV, UINTV, FLOATV,
        VECTOR_END = FLOATV,
        /** Pointer to raw memory \see DataArg. */
        DATA,
        HANDLE_BEGIN,
        BUFFER = HANDLE_BEGIN, IMAGE, SAMPLER,
        HANDLE_END = SAMPLER,
        N,
    };
    /** Properties of memory blocks. */
    enum MemFlag : u8 {
        READ_WRITE = 1u << 0, WRITE_ONLY = 1u << 1, READ_ONLY = 1u << 2,
    };
    /** Kernel execution flags. */
    enum ExecFlag : u8 {
        /**
         * Block the execution until the operation finishes.
         * Note that if this flag is not set and no event data is provided (see
         * \ref Events), it will not be possible to determine when the operation
         * finishes.
         */
        BLOCKING = 1u << 0,
    };
    /** Indices for the values accessible via \ref get_limits. */
    enum Limit : u8 {
        COMPUTE_UNITS, WORK_GROUP_SIZE, LOCAL_MEMORY, N,
    };
    /** Bit mask indicating which profiling data to collect/return. */
    enum ProfInfo : u8 {
        QUEUED = 1u << 0, SUBMIT = 1u << 1, START = 1u << 2, END = 1u << 3,
        PROF_INFO_MAX = END, PROF_INFO_ALL = QUEUED | SUBMIT | START | END,
    };
    /** Base class for handles to opaque compute objects. */
    struct Handle {
        u32 id = {};
        constexpr explicit operator bool() const { return this->id; }
    };
    struct Buffer : Handle { static constexpr auto type = Type::BUFFER; };
    struct Image : Handle { static constexpr auto type = Type::IMAGE; };
    struct Sampler : Handle { static constexpr auto type = Type::SAMPLER; };
    struct Program : Handle {};
    struct Kernel : Handle {};
    /**
     * Establishes dependencies between operations.
     * Unlike the \ref Handle sub-classes, it is a direct reference to the
     * object and must be destroyed manually via \ref release_events.
     */
    struct Event;
    /** Controls dependencies between operations. */
    struct Events {
        /** Number of elements in \ref wait_list. */
        std::size_t n_wait = {};
        /** Events that must precede the execution of the operation. */
        const Event *const *wait_list = {};
        /**
         * Buffer where the operation's resulting events will be placed.
         * For \ref execute, the required size is dependent on the argument
         * types: it can be obtained via \ref n_events (plus one for the
         * execution itself).  For all other operations, the size is \c 1.
         */
        Event *const *events = {};
    };
    /**
     * Argument type for raw memory passed to the execution kernel "by value".
     * It differs from specifying a range of bytes (e.g.
     * <tt>std::span<const std::byte>></tt> in that the value will be forwarded
     * directly as an argument, instead of being first copied to a temporary
     * buffer.
     */
    struct DataArg {
        std::size_t s = {};
        const std::byte *p = {};
        template<typename T>
        /** Convenience constructor to pass a single object as an argument. */
        explicit constexpr DataArg(const T *t) : s(sizeof(T)), p(as_bytes(t)) {}
        constexpr auto begin() const { return this->p; }
        constexpr auto end() const { return this->p + this->s; }
    };
    /** Maps supported types to the equivalent \ref Type value. */
    template<typename T> static constexpr Type arg_type = Type::NONE;
    /** Determines is \c t is one of the \c *V vector values in \ref Type. */
    static constexpr bool is_vector_type(Type t);
    static constexpr bool is_handle_type(Type t);
    /**
     * Transforms a scalar type into the equivalent vector type.
     * E.g. <tt>to_vector_type<INT> == INTV</tt>.  It is an error to apply this
     * function to a non-scalar type.
     */
    template<Type t> static constexpr Type to_vector_type();
    /**
     * Creates a back end of the specified type.
     * The \c init function must be called (and its return value checked) before
     * it can be used.
     */
    static std::unique_ptr<Compute> create(
        Backend b, const void *params = nullptr);
    NNGN_VIRTUAL(Compute)
    /** Must be called before the back end can be used. */
    virtual bool init() = 0;
    virtual size_t n_platforms() const = 0;
    virtual size_t n_devices() const = 0;
    /** Writes the information described in \ref Limit into \c p. */
    virtual void get_limits(u64 *p) const = 0;
    /** Convenience overload that allocates the required memory. */
    std::vector<u64> get_limits() const;
    virtual std::string platform_name() const = 0;
    virtual std::string device_name() const = 0;
    /**
     * Create a buffer of the specified size.
     * Optionally fill it with \c n bytes starting from \c p, if not null.
     */
    virtual Buffer create_buffer(
        MemFlag flags, std::size_t n, const std::byte *p) = 0;
    virtual bool read_buffer(
        Buffer b, std::size_t off, std::size_t n, std::byte *p,
        Events events) const = 0;
    /** Fills the buffer with \c n copies of \c v. */
    virtual bool fill_buffer(
        Buffer b, std::size_t off, std::size_t n, std::byte v,
        Events events) const = 0;
    /**
     * Fills the buffer with <tt>n / pattern_size</tt> copies of pattern \c p.
     * \c n must be a multiple of \c pattern_size.
     */
    virtual bool fill_buffer(
        Buffer b, std::size_t off, std::size_t n,
        std::size_t pattern_size, const std::byte *p,
        Events events) const = 0;
    virtual bool write_buffer(
        Buffer b, std::size_t off, std::size_t n, const std::byte *p,
        Events events) const = 0;
    /**
     * Writes a rectangular region of a buffer.
     * Arguments are the same as the OpenCL function \c clEnqueueCopyBufferRect.
     */
    virtual bool write_buffer_rect(
        Buffer b,
        std::array<std::size_t, 3> buffer_origin,
        std::array<std::size_t, 3> host_origin,
        std::array<std::size_t, 3> region,
        std::size_t buffer_row_pitch, std::size_t buffer_slice_pitch,
        std::size_t host_row_pitch, std::size_t host_slice_pitch,
        const std::byte *p, Events events) const = 0;
    virtual void *map_buffer(
        Buffer b, MemFlag flags, std::size_t off, std::size_t n,
        Events events) const = 0;
    virtual bool unmap_buffer(Buffer b, void *p, Events events) const = 0;
    /**
     * Writes the variadic arguments sequentially to the buffer.
     * Supported arguments types are scalar and vector types described in \ref
     * execute.
     */
    template<typename ...Ts>
    bool write_struct(Buffer b, Events events, Ts &&...ts) const;
    virtual bool release_buffer(Buffer b) = 0;
    /**
     * Create an image of the specified size.
     * Optionally fill it with <tt>w * h bytes</tt> starting from \c p, if not
     * null.
     */
    virtual Image create_image(
        Type type, std::size_t w, std::size_t h, MemFlag flags,
        const std::byte *p) = 0;
    virtual bool read_image(
        Image i, std::size_t w, std::size_t h, std::byte *p, Events events)
        const = 0;
    /** Fills the image with copies of \c v. */
    virtual bool fill_image(
        Image i, std::size_t w, std::size_t h, const void *v,
        Events events) const = 0;
    virtual bool release_image(Image i) = 0;
    virtual Sampler create_sampler() = 0;
    virtual bool release_sampler(Sampler s) = 0;
    /** Compiles source into a program using compilation options \c opts. */
    virtual Program create_program(std::string_view src, const char *opts) = 0;
    virtual bool release_program(Program p) = 0;
    virtual Kernel create_kernel(
        Program program, const char *func,
        std::size_t len, const Type *types,
        const std::size_t *sizes, const std::byte *const *data,
        Events events) = 0;
    virtual bool release_kernel(Kernel k) = 0;
    /**
     * Number of events generated by \ref execute for arguments \c types.
     * Does not include the event generated by the execution itself.
     */
    virtual std::size_t n_events(std::size_t n, const Type *types) const = 0;
    /**
     * Collect profiling information from \c n \c events.
     * The back end must have been created with debugging information enabled.
     */
    virtual bool prof_info(
        ProfInfo info, std::size_t n, const Event *const *events, u64 *out)
        const = 0;
    /** Block until the given \c events have completed. */
    virtual bool wait(std::size_t n, const Event *const *v) const = 0;
    virtual bool release_events(std::size_t n, const Event *const *v) const = 0;
    virtual bool execute(
        Kernel kernel, ExecFlag flags,
        u32 n_dim, const std::size_t *global_size,
        const std::size_t *local_size, Events events) const = 0;
    /**
     * Execute a program with the given arguments.
     * \param func Name of the function to be used as the entry point.
     * \param n_dim Number of dimensions in \c global_size and \c local_size.
     * \param len Length of \c types, \c sizes, and \c data.
     * \param types Type of each argument in \c data.
     * \param sizes Size of each argument in \c data.
     * \param data
     *     The argument data.  Must have the correct type and size as determined
     *     by the corresponding values in \c types and \c sizes.
     * \param events Dependency information, see \ref Events and \ref n_events.
     */
    virtual bool execute(
        Program program, const std::string &func, ExecFlag flags,
        u32 n_dim, const std::size_t *global_size,
        const std::size_t *local_size, std::size_t len, const Type *types,
        const std::size_t *sizes, const std::byte *const *data,
        Events events) const = 0;
    /** Variadic interface for #create_kernel.  \see #execute */
    template<typename ...Ts>
    Kernel create_kernel(
        Program program, const char *func, Events events, Ts &&...ts);
    /**
     * Variadic interface for #execute.
     * All arguments in common with the other function in the overload set have
     * the same semantics.
     * \param ts
     *     These values are used to create temporary (C) arrays that are used as
     *     the `len`, `types`, `sizes`, and `data` arguments of the original
     *     function.
     *     The values for those arrays are determined via the #arg_type, \ref
     *     detail::arg_size, and \ref detail::arg_ptr functions, respectively,
     *     and must be of supported types (see the #arg_type specializations).
     */
    template<typename ...Ts>
    bool execute(
        Program program, const std::string &func, ExecFlag flags,
        u32 n_dim, const std::size_t *global_size,
        const std::size_t *local_size, Events events, Ts &&...ts);
};

template<Compute::Backend>
std::unique_ptr<Compute> compute_create_backend(const void *params);

template<typename T>
static constexpr Compute::Type arg_type = Compute::Type::NONE;
template<>
constexpr auto Compute::arg_type<std::byte> = Compute::Type::BYTE;
template<>
constexpr auto Compute::arg_type<i32> = Compute::Type::INT;
template<>
constexpr auto Compute::arg_type<u32> = Compute::Type::UINT;
template<>
constexpr auto Compute::arg_type<float> = Compute::Type::FLOAT;

template<std::derived_from<Compute::Handle> T>
constexpr auto Compute::arg_type<T> = T::type;

template<std::ranges::range T>
constexpr auto Compute::arg_type<T> =
    Compute::to_vector_type<Compute::arg_type<std::ranges::range_value_t<T>>>();

inline constexpr bool Compute::is_vector_type(Type t)
    { return Type::VECTOR_BEGIN <= t && t <= Type::VECTOR_END; }
inline constexpr bool Compute::is_handle_type(Type t)
    { return Type::HANDLE_BEGIN <= t && t <= Type::HANDLE_END; }

namespace detail {

inline auto arg_size(const std::byte&) { return sizeof(std::byte); }
inline auto arg_ptr(const std::byte &b) { return as_bytes(&b); }

inline auto arg_size(const Compute::Handle &t) { return sizeof(t.id); }
inline auto arg_ptr(const Compute::Handle &t) { return as_bytes(&t.id); }

template<arithmetic T> auto arg_size(const T&) { return sizeof(T); }
auto arg_ptr(const arithmetic auto &t) { return as_bytes(&t); }

auto arg_size(const std::ranges::sized_range auto &r) {
    // TODO https://bugs.llvm.org/show_bug.cgi?id=39663
    // return std::span{r}.size_bytes();
    auto s = std::span{r};
    return s.size_bytes();
}
auto arg_ptr(const std::ranges::range auto &r)
    { return std::as_bytes(std::span{r}).data(); }

}

template<Compute::Type t>
inline constexpr auto Compute::to_vector_type() -> Type {
    using T = std::underlying_type_t<Compute::Type>;
    if constexpr(Type::SCALAR_BEGIN <= t && t < Type::VECTOR_BEGIN)
        return static_cast<Compute::Type>(
            static_cast<T>(Compute::Type::VECTOR_BEGIN)
            + static_cast<T>(t)
            - static_cast<T>(Compute::Type::BYTE));
    else
        static_assert(nngn::always_false<std::integral_constant<Type, t>>{});
}

inline std::vector<u64> Compute::get_limits() const {
    std::vector<u64> ret(Limit::N);
    this->get_limits(ret.data());
    return ret;
}

template<typename ...Ts>
bool Compute::write_struct(Buffer b, Events events, Ts &&...ts) const {
    const std::array sizes = {detail::arg_size(ts)...};
    std::vector<std::byte> data(std::reduce(cbegin(sizes), cend(sizes)));
    assert(!data.empty());
    auto copy = [p = data.data(), s = sizes.data()](const auto &x) mutable
        { std::memcpy(p, detail::arg_ptr(x), *s); p += *s++; };
    (..., copy(ts));
    return this->write_buffer(b, 0, data.size(), data.data(), events);
}

template<typename ...Ts>
auto Compute::create_kernel(
    Program program, const char *func, Events events, Ts &&...ts
) -> Kernel {
    constexpr auto n = sizeof...(Ts);
    std::array<Type, n> types = {};
    std::array<std::size_t, n> sizes = {};
    std::array<const std::byte*, n> data = {};
    auto f =
        [tp = types.data(), sp = sizes.data(), dp = data.data()]
        <typename T>(const T &t) mutable
    {
        *tp++ = Compute::arg_type<T>;
        *sp++ = detail::arg_size(t);
        *dp++ = as_bytes(detail::arg_ptr(t));
    };
    (..., f(ts));
    return this->create_kernel(
        program, func, n, types.data(), sizes.data(), data.data(), events);
}

template<typename ...Ts>
bool Compute::execute(
        Program program, const std::string &func, ExecFlag flags,
        u32 n_dim, const std::size_t *global_size,
        const std::size_t *local_size, Events events, Ts &&...ts) {
    constexpr auto n = sizeof...(Ts);
    std::array<Type, n> types = {};
    std::array<std::size_t, n> sizes = {};
    std::array<const std::byte*, n> data = {};
    auto f =
        [tp = types.data(), sp = sizes.data(), dp = data.data()]
        <typename T>(const T &t) mutable
    {
        *tp++ = Compute::arg_type<T>;
        *sp++ = detail::arg_size(t);
        *dp++ = as_bytes(detail::arg_ptr(t));
    };
    (..., f(ts));
    return this->execute(
        program, func, flags, n_dim, global_size, local_size,
        n, types.data(), sizes.data(), data.data(), events);
}

}

#endif
