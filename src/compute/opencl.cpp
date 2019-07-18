#include "os/platform.h"
#include "utils/flags.h"
#include "utils/log.h"

#include "compute.h"

static constexpr auto backend = nngn::Compute::Backend::OPENCL_BACKEND;

#ifndef NNGN_PLATFORM_HAS_OPENCL

namespace nngn {

template<>
std::unique_ptr<Compute> compute_create_backend<backend>(const void*) {
    NNGN_LOG_CONTEXT_F();
    Log::l() << "compiled without OpenCL support\n";
    return nullptr;
}

}

#else

#include <algorithm>

#include "utils/log.h"
#include "utils/scoped.h"

#include "opencl.h"
// This code supports OpenCL 1.2 (e.g. POCL), which had many of its functions
// deprecated.  See CL_TARGET_OPENCL_VERSION preprocessor checks below.
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

namespace {

using namespace std::string_view_literals;
using nngn::u32, nngn::u64;

using TemporaryBuffers = nngn::scoped<
    std::vector<cl_mem>,
    decltype([](const auto &v)
        { for(auto x : v) if(x) clReleaseMemObject(x); })>;

using TemporaryKernel = nngn::scoped<
    cl_kernel,
    decltype([](auto x) { if(x) clReleaseKernel(x); })>;

using TemporaryEvents = nngn::scoped<
    std::vector<cl_event>,
    decltype([](const auto &v) { for(auto x : v) if(x) clReleaseEvent(x); })>;

static_assert(nngn::Compute::MemFlag::READ_ONLY == CL_MEM_READ_ONLY);
static_assert(nngn::Compute::MemFlag::WRITE_ONLY == CL_MEM_WRITE_ONLY);
static_assert(nngn::Compute::MemFlag::READ_WRITE == CL_MEM_READ_WRITE);

class Device {
public:
    using Version = nngn::Compute::Version;
    cl_device_id id() const { return this->m_id; }
    Version version() const { return this->m_version; }
    bool init(cl_device_id id, std::string *tmp);
private:
    bool get_version_info(std::string *tmp);
    cl_device_id m_id = {};
    Version m_version = {};
};

class Platform {
public:
    using Version = nngn::Compute::Version;
    using DeviceType = nngn::Compute::DeviceType;
    cl_platform_id id() const { return this->m_id; }
    bool init(cl_platform_id id, std::string *tmp);
    cl_device_id find_device(
        Version min_version, DeviceType preferred, Version *version) const;
    std::size_t n_devices() const;
private:
    bool get_version_info(std::string *tmp);
    bool get_device_info();
    cl_platform_id m_id = {};
    Version version = {};
    std::vector<Device> gpus = {}, cpus = {};
};

class OpenCLBackend final : public nngn::Compute {
public:
    NNGN_MOVE_ONLY(OpenCLBackend)
    explicit OpenCLBackend(OpenCLParameters p);
    ~OpenCLBackend(void) final;
    bool init() final;
    std::size_t n_platforms() const final;
    std::size_t n_devices() const final;
    void get_limits(u64 *p) const final
        { std::memcpy(p, &this->limits, sizeof(this->limits)); }
    std::string platform_name() const final;
    std::string device_name() const final;
    Buffer create_buffer(
        MemFlag flags, std::size_t n, const std::byte *p) final;
    bool read_buffer(
        Buffer b, std::size_t off, std::size_t n, std::byte *p,
        Events events) const final;
    bool fill_buffer(
        Buffer b, std::size_t off, std::size_t n, std::byte v,
        Events events) const final;
    bool fill_buffer(
        Buffer b, std::size_t off, std::size_t n,
        std::size_t pattern_size, const std::byte *p,
        Events events) const final;
    bool write_buffer(
        Buffer b, std::size_t off, std::size_t n, const std::byte *p,
        Events events) const final;
    bool write_buffer_rect(
        Buffer b,
        std::array<std::size_t, 3> buffer_origin,
        std::array<std::size_t, 3> host_origin,
        std::array<std::size_t, 3> region,
        std::size_t buffer_row_pitch, std::size_t buffer_slice_pitch,
        std::size_t host_row_pitch, std::size_t host_slice_pitch,
        const std::byte *p, Events events) const final;
    void *map_buffer(
        Buffer b, MemFlag flags, std::size_t off, std::size_t n,
        Events events) const final;
    bool unmap_buffer(Buffer b, void *p, Events events) const final;
    bool release_buffer(Buffer b) final;
    Image create_image(
        Type type, std::size_t w, std::size_t h, MemFlag flags,
        const std::byte *p) final;
    bool read_image(
        Image i, std::size_t w, std::size_t h, std::byte *p,
        Events events) const final;
    bool fill_image(
        Image i, std::size_t w, std::size_t h, const void*v,
        Events events) const override;
    bool release_image(Image i) final;
    Sampler create_sampler() final;
    bool release_sampler(Sampler s) final;
    Program create_program(std::string_view src, const char *opts) final;
    bool release_program(Program p) final;
    Kernel create_kernel(
        Program program, const char *func,
        std::size_t len, const Type *types,
        const std::size_t *sizes, const std::byte *const *data,
        Events events) final;
    bool release_kernel(Kernel k) final;
    std::size_t n_events(std::size_t n, const Type *types) const final;
    bool prof_info(
        ProfInfo info, std::size_t n, const Event *events,
        u64 *out) const final;
    bool wait(std::size_t n, const Event *e) const final;
    bool release_events(std::size_t n, const Event *v) const final;
    bool execute(
        Kernel kernel, ExecFlag flags,
        u32 n_dim, const std::size_t *global_size,
        const std::size_t *local_size, Events events) const final;
    bool execute(
        Program program, const std::string &func, ExecFlag flags,
        u32 n_dim, const std::size_t *global_size,
        const std::size_t *local_size,
        std::size_t len, const Type *types,
        const std::size_t *sizes, const std::byte *const *data,
        Events events) const final;
private:
    enum Flag {
        DEBUG = 1u << 0,
    };
    Version version = {};
    nngn::Flags<Flag> flags = {};
    DeviceType preferred_device = {};
    std::vector<Platform> platforms = {};
    cl_platform_id platform = {};
    cl_device_id device = {};
    cl_context context = {};
    std::vector<cl_mem> buffers = {};
    std::vector<cl_mem> images = {};
    std::vector<cl_sampler> samplers = {};
    std::vector<cl_program> programs = {};
    std::vector<cl_kernel> kernels = {};
    cl_command_queue queue = {};
    struct limits {
        u64 compute_units = 0, work_group_size = 0, local_memory = 0;
    } limits = {};
    bool set_kernel_args(
        cl_kernel k, std::size_t len, const Type *types,
        const std::size_t *sizes, const std::byte *const *data,
        cl_event *events, std::vector<cl_mem> *tmp_buffers = nullptr) const;
    bool execute_(
        cl_kernel kernel, ExecFlag flags,
        u32 n_dim, const std::size_t *global_size,
        const std::size_t *local_size,
        std::size_t n_events, Events events) const;
};

auto cl_strerror(cl_int e) {
    switch(e) {
#define C(V) case V: return #V;
    C(CL_SUCCESS)
    C(CL_DEVICE_NOT_FOUND)
    C(CL_DEVICE_NOT_AVAILABLE)
    C(CL_COMPILER_NOT_AVAILABLE)
    C(CL_MEM_OBJECT_ALLOCATION_FAILURE)
    C(CL_OUT_OF_RESOURCES)
    C(CL_OUT_OF_HOST_MEMORY)
    C(CL_PROFILING_INFO_NOT_AVAILABLE)
    C(CL_MEM_COPY_OVERLAP)
    C(CL_IMAGE_FORMAT_MISMATCH)
    C(CL_IMAGE_FORMAT_NOT_SUPPORTED)
    C(CL_BUILD_PROGRAM_FAILURE)
    C(CL_MAP_FAILURE)
    C(CL_MISALIGNED_SUB_BUFFER_OFFSET)
    C(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST)
    C(CL_COMPILE_PROGRAM_FAILURE)
    C(CL_LINKER_NOT_AVAILABLE)
    C(CL_LINK_PROGRAM_FAILURE)
    C(CL_DEVICE_PARTITION_FAILED)
    C(CL_KERNEL_ARG_INFO_NOT_AVAILABLE)
    C(CL_INVALID_VALUE)
    C(CL_INVALID_DEVICE_TYPE)
    C(CL_INVALID_PLATFORM)
    C(CL_INVALID_DEVICE)
    C(CL_INVALID_CONTEXT)
    C(CL_INVALID_QUEUE_PROPERTIES)
    C(CL_INVALID_COMMAND_QUEUE)
    C(CL_INVALID_HOST_PTR)
    C(CL_INVALID_MEM_OBJECT)
    C(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR)
    C(CL_INVALID_IMAGE_SIZE)
    C(CL_INVALID_SAMPLER)
    C(CL_INVALID_BINARY)
    C(CL_INVALID_BUILD_OPTIONS)
    C(CL_INVALID_PROGRAM)
    C(CL_INVALID_PROGRAM_EXECUTABLE)
    C(CL_INVALID_KERNEL_NAME)
    C(CL_INVALID_KERNEL_DEFINITION)
    C(CL_INVALID_KERNEL)
    C(CL_INVALID_ARG_INDEX)
    C(CL_INVALID_ARG_VALUE)
    C(CL_INVALID_ARG_SIZE)
    C(CL_INVALID_KERNEL_ARGS)
    C(CL_INVALID_WORK_DIMENSION)
    C(CL_INVALID_WORK_GROUP_SIZE)
    C(CL_INVALID_WORK_ITEM_SIZE)
    C(CL_INVALID_GLOBAL_OFFSET)
    C(CL_INVALID_EVENT_WAIT_LIST)
    C(CL_INVALID_EVENT)
    C(CL_INVALID_OPERATION)
    C(CL_INVALID_GL_OBJECT)
    C(CL_INVALID_BUFFER_SIZE)
    C(CL_INVALID_MIP_LEVEL)
    C(CL_INVALID_GLOBAL_WORK_SIZE)
    C(CL_INVALID_PROPERTY)
    C(CL_INVALID_IMAGE_DESCRIPTOR)
    C(CL_INVALID_COMPILER_OPTIONS)
    C(CL_INVALID_LINKER_OPTIONS)
    C(CL_INVALID_DEVICE_PARTITION_COUNT)
    default: return "unknown";
#undef C
    }
}

cl_int check_result(const char *func_name, cl_int result) {
    if(result != CL_SUCCESS)
        nngn::Log::l() << func_name << ": " << cl_strerror(result) << '\n';
    return result;
}
#define LOG_RESULT(f, ...) (check_result(#f, f(__VA_ARGS__)) == CL_SUCCESS)
#define CHECK_RESULT(f, ...) if(!LOG_RESULT(f, __VA_ARGS__)) return false;
#define EVENT_PARAMS(e) \
    static_cast<cl_uint>(e.n_wait), \
    to_cl_event(e.wait_list), to_cl_event(e.events)

static_assert(sizeof(nngn::Compute::Event) == sizeof(cl_event));

cl_event *to_cl_event(nngn::Compute::Event *e) {
    return reinterpret_cast<cl_event*>(e);
}

const cl_event *to_cl_event(const nngn::Compute::Event *e) {
    return reinterpret_cast<const cl_event*>(e);
}

nngn::Compute::Event *from_cl_event(cl_event *e) {
    return reinterpret_cast<nngn::Compute::Event*>(e);
}

std::optional<nngn::Compute::Version> parse_version(std::string_view s) {
    if(!s.starts_with("OpenCL ")) {
        nngn::Log::l() << "invalid version format: " << s << '\n';
        return {};
    }
    std::stringstream ss = {};
    ss << s.substr(sizeof("OpenCL ") - 1);
    nngn::Compute::Version ret = {};
    char c = {};
    if(!(ss >> ret.major >> c >> ret.minor) || c != '.') {
        nngn::Log::l() << "invalid version format: " << s << '\n';
        return {};
    }
    return ret;
}

bool version_ge(nngn::Compute::Version lhs, nngn::Compute::Version rhs) {
    return lhs.major > rhs.major
        || (lhs.major == rhs.major && lhs.minor >= rhs.minor);
}

template<typename T>
typename T::const_pointer get_obj(const T &v, const nngn::Compute::Handle &h) {
    if constexpr(nngn::Platform::debug)
        if(!h || h.id >= v.size()) {
            nngn::Log::l() << "invalid id: " << h.id << '\n';
            return nullptr;
        }
    auto *ret = &v[h.id];
    if constexpr(nngn::Platform::debug)
        if(!*ret) {
            nngn::Log::l() << "invalid object: " << ret << '\n';
            return nullptr;
        }
    return ret;
}

template<typename T>
typename T::pointer get_obj(T &v, const nngn::Compute::Handle &h) {
    return const_cast<typename T::pointer>(
        get_obj(static_cast<const T&>(v), h));
}

template<typename T, typename F, typename... P>
auto enumerate(const char *fn, F f, T *v, const P &...args) {
    cl_uint n = 0;
    if(const auto ret = check_result(fn, f(args..., n, nullptr, &n));
            ret != CL_SUCCESS)
        return ret;
    v->resize(n);
    return check_result(fn, f(args..., n, v->data(), nullptr));
}
#define ENUMERATE(f, ...) enumerate(#f, f, __VA_ARGS__)

template<typename T>
u32 insert_at_first_free(std::vector<T> *v, T t) {
    const auto b = v->begin(), e = v->end();
    if(auto const it = std::find(b + 1, e, T{}); it != e) {
        *it = t;
        return static_cast<u32>(std::distance(b, it));
    }
    v->emplace_back(t);
    return static_cast<u32>(v->size() - 1);
}

bool Device::init(cl_device_id id, std::string *tmp) {
    NNGN_LOG_CONTEXT_CF(Device);
    this->m_id = id;
    return this->get_version_info(tmp);
}

bool Device::get_version_info(std::string *tmp) {
    NNGN_LOG_CONTEXT_CF(Device);
    std::size_t n = 0;
    if(!LOG_RESULT(clGetDeviceInfo,
            this->m_id, CL_DEVICE_VERSION, 0, nullptr, &n))
        return false;
    tmp->resize(n);
    if(!LOG_RESULT(clGetDeviceInfo,
            this->m_id, CL_DEVICE_VERSION, n, tmp->data(), nullptr))
        return false;
    const auto v = parse_version(std::move(*tmp));
    if(v)
        this->m_version = *v;
    return v.has_value();
}

bool Platform::init(cl_platform_id id, std::string *tmp) {
    NNGN_LOG_CONTEXT_CF(Platform);
    this->m_id = id;
    return this->get_version_info(tmp)
        && this->get_device_info();
}

bool Platform::get_version_info(std::string *tmp) {
    NNGN_LOG_CONTEXT_CF(Platform);
    std::size_t n = 0;
    if(!LOG_RESULT(clGetPlatformInfo,
            this->m_id, CL_PLATFORM_VERSION, 0, nullptr, &n))
        return false;
    tmp->resize(n);
    if(!LOG_RESULT(clGetPlatformInfo,
            this->m_id, CL_PLATFORM_VERSION, n, tmp->data(), nullptr))
        return false;
    const auto v = parse_version(std::move(*tmp));
    if(v)
        this->version = *v;
    return v.has_value();
}

bool Platform::get_device_info() {
    NNGN_LOG_CONTEXT_CF(Platform);
    const auto f = [this](auto type, auto *tmp_v, auto *tmp_s, auto *v) {
        cl_uint n = 0;
        const auto t = static_cast<cl_device_type>(type);
        switch(const auto err = clGetDeviceIDs(this->m_id, t, 0, nullptr, &n)) {
        case CL_SUCCESS: break;
        case CL_DEVICE_NOT_FOUND: return true;
        default: check_result("clGetDeviceIDs", err); return false;
        };
        tmp_v->resize(n);
        CHECK_RESULT(clGetDeviceIDs, this->m_id, t, n, tmp_v->data(), nullptr);
        v->reserve(n);
        for(auto x : *tmp_v)
            if(!v->emplace_back().init(x, tmp_s))
                return false;
        return true;
    };
    std::vector<cl_device_id> tmp_v = {};
    std::string tmp_s = {};
    return f(CL_DEVICE_TYPE_GPU, &tmp_v, &tmp_s, &this->gpus)
        && f(CL_DEVICE_TYPE_CPU, &tmp_v, &tmp_s, &this->cpus);
}

cl_device_id Platform::find_device(
    Version min_version, DeviceType type, Version *dev_version
) const {
    if(!version_ge(this->version, min_version))
        return nullptr;
    const auto f = [min_version, dev_version](const auto &v, auto *out) {
        const auto i = std::ranges::find_if(v, [min_version](auto &d) {
            return version_ge(d.version(), min_version);
        });
        if(i == end(v))
            return;
        *out = i->id();
        *dev_version = i->version();
    };
    cl_device_id ret = nullptr;
    switch(type) {
    case DeviceType::GPU: f(this->gpus, &ret); break;
    case DeviceType::CPU: f(this->cpus, &ret); break;
    default: break;
    }
    return ret;
}

std::size_t Platform::n_devices() const {
    return this->gpus.size() + this->cpus.size();
}

OpenCLBackend::OpenCLBackend(OpenCLParameters p) :
    version{p.version},
    flags{p.debug ? Flag::DEBUG : Flag{}},
    preferred_device{p.preferred_device} {}

OpenCLBackend::~OpenCLBackend() {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend)
    for(auto x : this->programs) if(x) LOG_RESULT(clReleaseProgram, x);
    for(auto x : this->kernels) if(x) LOG_RESULT(clReleaseKernel, x);
    for(auto x : this->samplers) if(x) LOG_RESULT(clReleaseSampler, x);
    for(auto x : this->images) if(x) LOG_RESULT(clReleaseMemObject, x);
    for(auto x : this->buffers) if(x) LOG_RESULT(clReleaseMemObject, x);
    if(this->queue)
        LOG_RESULT(clReleaseCommandQueue, this->queue);
    if(this->context)
        LOG_RESULT(clReleaseContext, this->context);
}

bool OpenCLBackend::init() {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend)
    constexpr auto get_platforms = [](auto *v) {
        std::vector<cl_platform_id> ids = {};
        if(ENUMERATE(clGetPlatformIDs, &ids) != CL_SUCCESS)
            return false;
        if(ids.empty())
            return nngn::Log::l() << "no available platforms\n", false;
        const auto n = ids.size();
        v->reserve(n);
        std::string tmp = {};
        for(auto x : ids)
            if(!v->emplace_back().init(x, &tmp))
                return false;
        return true;
    };
    constexpr auto select_device = [](
        const auto &ps, Version v, DeviceType t,
        cl_platform_id *p, cl_device_id *d, Version *dv
    ) {
        const auto f = [&ps, v, p, d, dv](DeviceType t_) {
            const auto i = std::ranges::find_if(ps, [v, t_, d, dv](auto &x) {
                const auto ret = x.find_device(v, t_, dv);
                return ret && (*d = ret);
            });
            if(i == end(ps))
                return false;
            *p = i->id();
            return true;
        };
        if(f(t))
            return;
        switch(t) {
        case DeviceType::GPU: f(DeviceType::CPU); break;
        case DeviceType::CPU: f(DeviceType::GPU); break;
        }
    };
    constexpr auto get_device_info = [](auto d, auto *l) {
        using T = std::tuple<cl_device_info, u64*>;
        for(auto [i, p] : {
            T{CL_DEVICE_MAX_COMPUTE_UNITS, &l->compute_units},
            T{CL_DEVICE_MAX_WORK_GROUP_SIZE, &l->work_group_size},
            T{CL_DEVICE_LOCAL_MEM_SIZE, &l->local_memory},
        })
            CHECK_RESULT(clGetDeviceInfo, d, i, sizeof(*p), p, nullptr);
        return true;
    };
    const auto create_context = [this](auto *p) {
        const auto cb = [](
            const char *errinfo, const void*, std::size_t, void*
        ) {
            NNGN_LOG_CONTEXT("opencl_debug_callback");
            nngn::Log::l() << errinfo << '\n';
        };
        cl_int err = CL_SUCCESS;
        *p = clCreateContext(nullptr, 1, &this->device, cb, nullptr, &err);
        return check_result("clCreateContext", err) == CL_SUCCESS;
    };
    const auto create_queue = [this](auto *p) {
        cl_int err = CL_SUCCESS;
        cl_command_queue_properties props =
            CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE;
        if(this->flags.is_set(Flag::DEBUG))
            props |= CL_QUEUE_PROFILING_ENABLE;
        if(this->version.major >= 2) {
            *p = clCreateCommandQueueWithProperties(
                this->context, this->device,
                std::array<cl_command_queue_properties, 3>{
                    CL_QUEUE_PROPERTIES, props,
                }.data(), &err);
            return check_result("clCreateCommandQueueWithProperties", err)
                == CL_SUCCESS;
        } else {
            *p = clCreateCommandQueue(this->context, this->device, props, &err);
            return check_result("clCreateCommandQueue", err) == CL_SUCCESS;
        }
    };
    if(!get_platforms(&this->platforms))
        return false;
    select_device(
        this->platforms, this->version, this->preferred_device,
        &this->platform, &this->device, &this->version);
    if(!this->device)
        return nngn::Log::l() << "no available devices\n", false;
    if(!get_device_info(this->device, &this->limits))
        return false;
    if(!create_context(&this->context))
        return false;
    if(!create_queue(&this->queue))
        return false;
    this->programs.push_back(nullptr);
    this->kernels.push_back(nullptr);
    this->buffers.push_back(nullptr);
    this->images.push_back(nullptr);
    this->samplers.push_back(nullptr);
    return true;
}

std::size_t OpenCLBackend::n_platforms() const {
    return this->platforms.size();
}

std::size_t OpenCLBackend::n_devices() const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return std::accumulate(
        begin(this->platforms), end(this->platforms), std::size_t{},
        [](auto a, const auto &x) { return a + x.n_devices(); });
}

std::string OpenCLBackend::platform_name() const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    constexpr auto i = CL_PLATFORM_NAME;
    std::string ret;
    std::size_t n = 0;
    if(LOG_RESULT(clGetPlatformInfo, this->platform, i, 0, nullptr, &n)) {
        ret.resize(n);
        LOG_RESULT(clGetPlatformInfo,
            this->platform, i, n, ret.data(), nullptr);
    }
    return ret;
}

std::string OpenCLBackend::device_name() const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    constexpr auto i = CL_DEVICE_NAME;
    std::string ret;
    std::size_t n = 0;
    if(LOG_RESULT(clGetDeviceInfo, this->device, i, 0, nullptr, &n)) {
        ret.resize(n);
        LOG_RESULT(clGetDeviceInfo, this->device, i, n, ret.data(), nullptr);
    }
    return ret;
}

auto OpenCLBackend::create_buffer(
    MemFlag mem_flags, std::size_t n, const std::byte *p
) -> Buffer {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    cl_int err = CL_SUCCESS;
    cl_mem_flags f = mem_flags
        & (MemFlag::READ_ONLY | MemFlag::WRITE_ONLY | MemFlag::READ_WRITE);
    if(p)
        f |= CL_MEM_COPY_HOST_PTR;
    const auto b = clCreateBuffer(
        this->context, f, n, const_cast<std::byte*>(p), &err);
    if(check_result("clCreateBuffer", err) != CL_SUCCESS)
        return {};
    return {{insert_at_first_free(&this->buffers, b)}};
}

bool OpenCLBackend::read_buffer(
    Buffer b, std::size_t off, std::size_t n, std::byte *p, Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clEnqueueReadBuffer,
        this->queue, *get_obj(this->buffers, b), !events.events, off, n, p,
        EVENT_PARAMS(events));
}

bool OpenCLBackend::fill_buffer(
    Buffer b, std::size_t off, std::size_t n, std::byte v, Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return this->fill_buffer(b, off, n, 1, &v, events);
}

bool OpenCLBackend::fill_buffer(
    Buffer b, std::size_t off, std::size_t n,
    std::size_t pattern_size, const std::byte *p, Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clEnqueueFillBuffer,
        this->queue, *get_obj(this->buffers, b), p, pattern_size, off, n,
        EVENT_PARAMS(events));
}

bool OpenCLBackend::write_buffer(
    Buffer b, std::size_t off, std::size_t n, const std::byte *p,
    Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clEnqueueWriteBuffer,
        this->queue, *get_obj(this->buffers, b), !events.events, off, n, p,
        EVENT_PARAMS(events));
}

bool OpenCLBackend::write_buffer_rect(
    Buffer b,
    std::array<std::size_t, 3> buffer_origin,
    std::array<std::size_t, 3> host_origin,
    std::array<std::size_t, 3> region,
    std::size_t buffer_row_pitch, std::size_t buffer_slice_pitch,
    std::size_t host_row_pitch, std::size_t host_slice_pitch,
    const std::byte *p,
    Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clEnqueueWriteBufferRect,
        this->queue, *get_obj(this->buffers, b), !events.events,
        buffer_origin.data(), host_origin.data(), region.data(),
        buffer_row_pitch, buffer_slice_pitch,
        host_row_pitch, host_slice_pitch, p,
        EVENT_PARAMS(events));
}

void *OpenCLBackend::map_buffer(
    Buffer b, MemFlag mem_flags, std::size_t off, std::size_t n,
    Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    cl_map_flags map_flags = 0;
    if(mem_flags & MemFlag::READ_ONLY)
        map_flags |= CL_MAP_READ;
    if(mem_flags & MemFlag::WRITE_ONLY)
        map_flags |= CL_MAP_WRITE_INVALIDATE_REGION;
    cl_int err = CL_SUCCESS;
    void *ret = clEnqueueMapBuffer(
        this->queue, *get_obj(this->buffers, b), !events.events,
        map_flags, off, n, EVENT_PARAMS(events), &err);
    if(check_result("clEnqueueMapBuffer", err) != CL_SUCCESS)
        return nullptr;
    return ret;
}

bool OpenCLBackend::unmap_buffer(Buffer b, void *p, Events events) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clEnqueueUnmapMemObject,
        this->queue, *get_obj(this->buffers, b), p,
        EVENT_PARAMS(events));
}

bool OpenCLBackend::release_buffer(Buffer b) {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clReleaseMemObject,
        std::exchange(*get_obj(this->buffers, b), {}));
}

auto OpenCLBackend::create_image(
    Type type, std::size_t w, std::size_t h, MemFlag mem_flags,
    const std::byte *p
) -> Image {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    cl_image_format fmt = {};
    fmt.image_channel_order = CL_RGBA;
    switch(type) {
    case Type::BYTEV: fmt.image_channel_data_type = CL_UNSIGNED_INT8; break;
    case Type::FLOATV: fmt.image_channel_data_type = CL_FLOAT; break;
    case Type::NONE:
    case Type::LOCAL:
    case Type::BYTE:
    case Type::INT:
    case Type::UINT:
    case Type::FLOAT:
    case Type::INTV:
    case Type::UINTV:
    case Type::DATA:
    case Type::BUFFER:
    case Type::IMAGE:
    case Type::SAMPLER:
    case Type::N:
    default:
        nngn::Log::l()
            << "invalid image type: "
            << static_cast<int>(type) << '\n';
        return {};
    }
    cl_image_desc desc = {};
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = w;
    desc.image_height = h;
    cl_int err = CL_SUCCESS;
    cl_mem_flags f = mem_flags
        & (MemFlag::READ_ONLY | MemFlag::WRITE_ONLY);
    if(p)
        f |= CL_MEM_COPY_HOST_PTR;
    const auto i = clCreateImage(
        this->context, f, &fmt, &desc,
        const_cast<std::byte*>(p), &err);
    if(check_result("clCreateImage", err) != CL_SUCCESS)
        return {};
    return {{insert_at_first_free(&this->images, i)}};
}

bool OpenCLBackend::read_image(
    Image i, std::size_t w, std::size_t h, std::byte *p,
    Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    constexpr std::array<std::size_t, 3> origin = {};
    const std::array<std::size_t, 3> region = {w, h, 1};
    return LOG_RESULT(clEnqueueReadImage,
        this->queue, *get_obj(this->images, i),
        !events.events, origin.data(), region.data(), 0, 0, p,
        EVENT_PARAMS(events));
}

bool OpenCLBackend::fill_image(
    Image i, std::size_t w, std::size_t h, const void *v,
    Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    constexpr std::array<std::size_t, 3> origin = {};
    const std::array<std::size_t, 3> region = {w, h, 1};
    return LOG_RESULT(clEnqueueFillImage,
        this->queue, *get_obj(this->images, i), v,
        origin.data(), region.data(),
        EVENT_PARAMS(events));
}

bool OpenCLBackend::release_image(Image i) {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clReleaseMemObject,
        std::exchange(*get_obj(this->images, i), {}));
}

auto OpenCLBackend::create_sampler() -> Sampler {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    cl_int err = CL_SUCCESS;
    constexpr cl_bool norm = CL_FALSE;
    constexpr cl_addressing_mode addr = CL_ADDRESS_CLAMP_TO_EDGE;
    constexpr cl_filter_mode filter = CL_FILTER_LINEAR;
    cl_sampler s = nullptr;
#if CL_TARGET_OPENCL_VERSION >= 200
    if(this->version.major >= 2)
        s = clCreateSamplerWithProperties(
            this->context,
            std::array<cl_sampler_properties, 7>{
                CL_SAMPLER_NORMALIZED_COORDS, norm,
                CL_SAMPLER_ADDRESSING_MODE, addr,
                CL_SAMPLER_FILTER_MODE, filter,
            }.data(), &err);
    else
#endif
        s = clCreateSampler(this->context, norm, addr, filter, &err);
    if(check_result("clCreateSampler", err) != CL_SUCCESS)
        return {};
    return {{insert_at_first_free(&this->samplers, s)}};
}

bool OpenCLBackend::release_sampler(Sampler s) {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clReleaseSampler,
        std::exchange(*get_obj(this->samplers, s), {}));
}

auto OpenCLBackend::create_program(
    std::string_view src, const char *opts
) -> Program {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    auto src_p = src.data();
    const auto size_p = src.size();
    cl_int err = CL_SUCCESS;
    auto p = clCreateProgramWithSource(
        this->context, 1, &src_p, &size_p, &err);
    if(check_result("clCreateProgramWithSource", err) != CL_SUCCESS)
        return {};
    if(LOG_RESULT(clBuildProgram, p, 0, nullptr, opts, nullptr, nullptr))
        return {{insert_at_first_free(&this->programs, p)}};
    std::size_t log_size = 0;
    clGetProgramBuildInfo(
        p, this->device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
    std::string log(log_size, 0);
    clGetProgramBuildInfo(
        p, this->device, CL_PROGRAM_BUILD_LOG,
        log_size, log.data(), nullptr);
    nngn::Log::l() << "build failed, log:\n" << log << '\n';
    return {};
}

bool OpenCLBackend::release_program(Program p) {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clReleaseProgram,
        std::exchange(*get_obj(this->programs, p), {}));
}

auto OpenCLBackend::create_kernel(
    Program program, const char *func,
    std::size_t len, const Type *types,
    const std::size_t *sizes, const std::byte *const *data,
    Events events
) -> Kernel {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    if(this->n_events(len, types)) {
        nngn::Log::l() << "cannot use temporary buffers\n";
        return {};
    }
    cl_int err = CL_SUCCESS;
    auto k = clCreateKernel(*get_obj(this->programs, program), func, &err);
    if(check_result("clCreateKernel", err) != CL_SUCCESS)
        return {};
    if(!this->set_kernel_args(
            k, len, types, sizes, data, to_cl_event(events.events))) {
        check_result("clReleaseKernel", clReleaseKernel(k));
        return {};
    }
    return {{insert_at_first_free(&this->kernels, k)}};
}

bool OpenCLBackend::release_kernel(Kernel k) {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clReleaseKernel,
        std::exchange(*get_obj(this->kernels, k), {}));
}

bool OpenCLBackend::set_kernel_args(
    cl_kernel k, std::size_t len, const Type *types,
    const std::size_t *sizes, const std::byte *const *data,
    cl_event *events, std::vector<cl_mem> *tmp_buffers
) const {
    NNGN_LOG_CONTEXT_F();
    cl_uint max = 0;
    CHECK_RESULT(clGetKernelInfo,
        k, CL_KERNEL_NUM_ARGS, sizeof(max), &max, nullptr);
    if(len > max) {
        nngn::Log::l()
            << "invalid number of arguments"
            " (" << len << " > " << max << ")\n";
        return false;
    }
    for(std::size_t i = 0; i < len; ++i) {
        const auto set = []<typename T>(
            const auto *name,
            const auto &v, const auto *p, auto k_, auto ui, T t
        ) {
            NNGN_LOG_CONTEXT(name);
            t.id = *static_cast<const u32*>(static_cast<const void*>(p));
            const auto *x = get_obj(v, t);
            return x && LOG_RESULT(clSetKernelArg, k_, ui, sizeof(*x), x);
        };
        const auto ui = static_cast<cl_uint>(i);
        const auto *d = data[i];
        switch(types[i]) {
        case Type::LOCAL: {
            const auto s = *static_cast<const cl_uint*>(
                static_cast<const void*>(d));
            assert(s <= this->limits.local_memory);
            CHECK_RESULT(clSetKernelArg, k, ui, s, nullptr);
            break;
        }
        case Type::BYTE:
            CHECK_RESULT(clSetKernelArg, k, ui, sizeof(cl_uchar), d);
            break;
        case Type::INT:
            CHECK_RESULT(clSetKernelArg, k, ui, sizeof(cl_int), d);
            break;
        case Type::UINT:
            CHECK_RESULT(clSetKernelArg, k, ui, sizeof(cl_uint), d);
            break;
        case Type::FLOAT:
            CHECK_RESULT(clSetKernelArg, k, ui, sizeof(cl_float), d);
            break;
        case Type::BYTEV:
        case Type::INTV:
        case Type::UINTV:
        case Type::FLOATV: {
            cl_int err = CL_SUCCESS;
            const auto buffer = tmp_buffers->emplace_back(clCreateBuffer(
                this->context, CL_MEM_READ_ONLY, sizes[i], nullptr, &err));
            if(check_result("clCreateBuffer", err) != CL_SUCCESS)
                return false;
            CHECK_RESULT(clEnqueueWriteBuffer,
                this->queue, buffer, CL_FALSE, 0, sizes[i], d,
                0, nullptr, events++);
            CHECK_RESULT(clSetKernelArg,
                k, static_cast<cl_uint>(i), sizeof(cl_mem), &buffer);
            break;
        }
        case nngn::Compute::Type::DATA:
            CHECK_RESULT(clSetKernelArg, k, ui, sizes[i], d);
            break;
        case nngn::Compute::Type::BUFFER:
            if(!set("buffer", this->buffers, d, k, ui, Buffer{}))
                return false;
            break;
        case nngn::Compute::Type::IMAGE:
            if(!set("image", this->images, d, k, ui, Image{}))
                return false;
            break;
        case nngn::Compute::Type::SAMPLER:
            if(!set("sampler", this->samplers, d, k, ui, Sampler{}))
                return false;
            break;
        case nngn::Compute::Type::NONE:
        case nngn::Compute::Type::N:
        default:
            nngn::Log::l()
                << "invalid argument type: "
                << static_cast<int>(types[i]) << '\n';
            break;
        }
    }
    return true;
}

std::size_t OpenCLBackend::n_events(std::size_t n, const Type *v) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return static_cast<std::size_t>(std::count_if(v, v + n, is_vector_type));
}

bool OpenCLBackend::prof_info(
    ProfInfo info, std::size_t n, const Event *events, u64 *out
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    const auto f = [info, &out](auto i, auto e, auto v) {
        if(!(info & i))
            return true;
        cl_int err = {};
        CHECK_RESULT(clGetEventInfo,
            *to_cl_event(&e), CL_EVENT_COMMAND_EXECUTION_STATUS,
            sizeof(err), &err, nullptr);
        if(err < 0) {
            nngn::Log::l()
                << "event " << e.p
                << ": " << cl_strerror(err) << '\n';
            return false;
        }
        CHECK_RESULT(clGetEventProfilingInfo,
            *to_cl_event(&e), static_cast<cl_profiling_info>(v),
            sizeof(cl_ulong), out++, nullptr);
        return true;
    };
    return std::all_of(events, events + n, [f](auto x) {
        return f(ProfInfo::QUEUED, x, CL_PROFILING_COMMAND_QUEUED)
            && f(ProfInfo::SUBMIT, x, CL_PROFILING_COMMAND_SUBMIT)
            && f(ProfInfo::START, x, CL_PROFILING_COMMAND_START)
            && f(ProfInfo::END, x, CL_PROFILING_COMMAND_END);
    });
}

bool OpenCLBackend::wait(std::size_t n, const Event *v) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return LOG_RESULT(clWaitForEvents,
        static_cast<cl_uint>(n), to_cl_event(v));
}

bool OpenCLBackend::release_events(std::size_t n, const Event *v) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    return std::all_of(v, v + n, [](auto &x)
        { return LOG_RESULT(clReleaseEvent, *to_cl_event(&x)); });
}

bool OpenCLBackend::execute_(
    cl_kernel kernel, ExecFlag exec_flags,
    u32 n_dim, const std::size_t *global_size,
    const std::size_t *local_size,
    std::size_t n_events, Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    auto *event = to_cl_event(events.events) + n_events - 1;
    cl_event *barrier = nullptr;
    if(events.n_wait) {
        barrier = event - 1;
        CHECK_RESULT(clEnqueueBarrierWithWaitList,
            this->queue, static_cast<cl_uint>(events.n_wait),
            to_cl_event(events.wait_list), barrier);
    }
    if(const auto n = static_cast<cl_uint>(n_events - 1)) {
        CHECK_RESULT(clEnqueueNDRangeKernel,
            this->queue, kernel, n_dim, nullptr, global_size, local_size,
            n, to_cl_event(events.events), event);
    } else
        CHECK_RESULT(clEnqueueNDRangeKernel,
            this->queue, kernel, n_dim, nullptr, global_size, local_size,
            0, nullptr, event);
    if(exec_flags & ExecFlag::BLOCKING)
        CHECK_RESULT(clWaitForEvents, 1, event);
    return true;
}

bool OpenCLBackend::execute(
    Kernel kernel, ExecFlag exec_flags,
    u32 n_dim, const std::size_t *global_size,
    const std::size_t *local_size, Events events
) const {
    const std::size_t n_events = 1 + !!events.n_wait;
    TemporaryEvents tmp_events = {};
    if(!events.events) {
        tmp_events->resize(n_events);
        events.events = from_cl_event(tmp_events->data());
    }
    return this->execute_(
        *get_obj(this->kernels, kernel), exec_flags,
        n_dim, global_size, local_size, n_events, events);
}

bool OpenCLBackend::execute(
    Program program, const std::string &func, ExecFlag exec_flags,
    u32 n_dim, const std::size_t *global_size, const std::size_t *local_size,
    std::size_t len, const Type *types,
    const std::size_t *sizes, const std::byte *const *data,
    Events events
) const {
    NNGN_LOG_CONTEXT_CF(OpenCLBackend);
    NNGN_LOG_CONTEXT(func.c_str());
    if(~exec_flags & ExecFlag::BLOCKING
            && std::any_of(types, types + len, Compute::is_vector_type)) {
        nngn::Log::l() << "cannot use temporary buffers in non-blocking call\n";
        return false;
    }
    cl_int err = CL_SUCCESS;
    const auto kernel = TemporaryKernel{
        clCreateKernel(*get_obj(this->programs, program), func.c_str(), &err)
    };
    if(check_result("clCreateKernel", err) != CL_SUCCESS)
        return false;
    TemporaryBuffers tmp_buffers = {};
    TemporaryEvents tmp_events = {};
    const auto n_events = 1 + !!events.n_wait + this->n_events(len, types);
    if(!events.events) {
        tmp_events->resize(n_events);
        events.events = from_cl_event(tmp_events->data());
    }
    return set_kernel_args(
            *kernel, len, types, sizes, data, to_cl_event(events.events),
            &*tmp_buffers)
        && this->execute_(
            *kernel, exec_flags, n_dim, global_size, local_size,
            n_events, events);
}

}

namespace nngn {

template<>
std::unique_ptr<Compute> compute_create_backend<backend>(const void *params) {
    return std::make_unique<OpenCLBackend>(
        params
            ? *static_cast<const Compute::OpenCLParameters*>(params)
            : Compute::OpenCLParameters{});
}

}

#endif
