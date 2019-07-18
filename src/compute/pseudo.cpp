#include "compute.h"

#include "utils/log.h"

using nngn::u32, nngn::u64;

namespace {

struct Pseudocomp : public nngn::Compute {
    Pseudocomp() = default;
    bool init() override { return true; }
    size_t n_platforms() const override { return 0; }
    size_t n_devices() const override { return 0; }
    void get_limits(u64 *p) const override;
    std::string platform_name() const override { return {}; }
    std::string device_name() const override { return {}; }
    Buffer create_buffer(MemFlag, size_t, const std::byte*) override
        { return {{1}}; }
    bool read_buffer(
            Buffer, std::size_t, std::size_t, std::byte*, Events) const override
        { return true; }
    bool fill_buffer(
            Buffer, std::size_t, std::size_t, std::byte, Events) const override
        { return true; }
    bool fill_buffer(
            Buffer, std::size_t, std::size_t, std::size_t, const std::byte*,
            Events) const override
        { return true; }
    bool write_buffer(
            Buffer, std::size_t, std::size_t, const std::byte*, Events)
            const override
        { return true; }
    bool write_buffer_rect(
            Buffer, std::array<std::size_t, 3>,
            std::array<std::size_t, 3>, std::array<std::size_t, 3>,
            std::size_t, std::size_t, std::size_t, std::size_t,
            const std::byte*, Events) const override
        { return true; }
    void *map_buffer(
            Buffer, MemFlag, std::size_t, std::size_t, Events) const override
        { return nullptr; }
    bool unmap_buffer(Buffer, void*, Events) const override
        { return true; }
    bool release_buffer(Buffer) override { return true; }
    Image create_image(
            Type, std::size_t, std::size_t, MemFlag, const std::byte*) override
        { return {{1}}; }
    bool read_image(
            Image, std::size_t, std::size_t, std::byte*, Events)
            const override
        { return true; }
    bool fill_image(
            Image, std::size_t, std::size_t, const void*,
            Events) const override
        { return true; }
    bool release_image(Image) override { return true; }
    Sampler create_sampler() override { return {{1}}; }
    bool release_sampler(Sampler) override { return true; }
    Program create_program(std::string_view, const char*) override
        { return {{1}}; }
    bool release_program(Program) override { return true; }
    Kernel create_kernel(
            Program, const char*,
            std::size_t, const Type*, const std::size_t*,
            const std::byte *const*, Events) override
        { return {{1}}; }
    bool release_kernel(Kernel) override { return true; }
    std::size_t n_events(std::size_t, const Type*) const override
        { return true; }
    bool prof_info(ProfInfo, std::size_t, const Event *const*, u64*)
        const override
        { return true; }
    bool wait(std::size_t, const Event *const*) const override { return true; }
    bool release_events(std::size_t, const Event *const*) const override
        { return true; }
    bool execute(
            Kernel, ExecFlag, u32, const std::size_t*, const std::size_t*,
            Events) const override
        { return true; }
    bool execute(
            Program, const std::string&, ExecFlag,
            u32, const std::size_t*, const std::size_t*,
            std::size_t, const Type*, const std::size_t*,
            const std::byte *const*, Events) const override
        { return true; }
};

void Pseudocomp::get_limits(u64 *p) const {
    p[Compute::Limit::COMPUTE_UNITS] = 1;
    p[Compute::Limit::WORK_GROUP_SIZE] = 1;
    p[Compute::Limit::LOCAL_MEMORY] = UINT64_MAX;
}

}

namespace nngn {

template<> std::unique_ptr<Compute> compute_create_backend
        <Compute::Backend::PSEUDOCOMP>
        (const void *params) {
    if(params) {
        Log::l() << "no parameters allowed\n";
        return {};
    }
    return std::make_unique<Pseudocomp>();
}

}
