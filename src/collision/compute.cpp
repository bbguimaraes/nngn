#include "collision.h"

#include <cmath>
#include <filesystem>

#include "compute/compute.h"
#include "os/platform.h"
#include "timing/timing.h"
#include "utils/log.h"
#include "utils/scoped.h"

namespace {

constexpr auto
    CONTERS_SIZE =
        static_cast<std::size_t>(nngn::Collider::Type::N_TYPES - 1)
            * static_cast<std::size_t>(nngn::Collider::Type::N_TYPES - 2),
    COUNTERS_BYTES = CONTERS_SIZE * sizeof(uint32_t);

struct Collision {
    std::array<float, 4> v;
    uint32_t i0, i1;
    std::array<char, 4 * sizeof(float) - 2 * sizeof(uint32_t)> padding;
};

struct AABBCollider {
    std::array<float, 2> center, bl, tr;
    float radius, padding;
};

struct BBCollider {
    std::array<float, 2> center, bl, tr;
    float radius, cos, sin, padding;
};

struct SphereCollider {
    std::array<float, 4> pos;
    float mass, radius;
    std::array<float, 2> padding;
};

struct PlaneCollider { std::array<float, 4> abcd; };

struct GravityCollider {
    std::array<float, 4> pos;
    float mass, max_distance2;
    std::array<float, 2> padding;
};

static_assert(offsetof(Collision, i0) == 4 * sizeof(float));
static_assert(offsetof(Collision, i1) == 4 * sizeof(float) + sizeof(uint32_t));
static_assert(sizeof(Collision) == 8 * sizeof(float));

static_assert(offsetof(AABBCollider, bl) == 2 * sizeof(float));
static_assert(offsetof(AABBCollider, tr) == 4 * sizeof(float));
static_assert(offsetof(AABBCollider, radius) == 6 * sizeof(float));
static_assert(sizeof(AABBCollider) == 8 * sizeof(float));

static_assert(offsetof(BBCollider, bl) == 2 * sizeof(float));
static_assert(offsetof(BBCollider, tr) == 4 * sizeof(float));
static_assert(offsetof(BBCollider, radius) == 6 * sizeof(float));
static_assert(offsetof(BBCollider, cos) == 7 * sizeof(float));
static_assert(offsetof(BBCollider, sin) == 8 * sizeof(float));
static_assert(sizeof(BBCollider) == 10 * sizeof(float));

static_assert(offsetof(SphereCollider, mass) == 4 * sizeof(float));
static_assert(offsetof(SphereCollider, radius) == 5 * sizeof(float));
static_assert(sizeof(SphereCollider) == 8 * sizeof(float));

static_assert(sizeof(PlaneCollider) == 4 * sizeof(float));

static_assert(offsetof(GravityCollider, mass) == 4 * sizeof(float));
static_assert(offsetof(GravityCollider, max_distance2) == 5 * sizeof(float));
static_assert(sizeof(GravityCollider) == 8 * sizeof(float));

struct Events {
    using Event = nngn::Compute::Event;
    Event counters;
    struct { Event copy, exec_barrier, exec; } aabb;
    struct { Event copy, exec_barrier, exec; } bb;
    struct {
        Event
            pos, vel, mass, radius, grid_count, grid_barrier, exec_grid,
            exec_barrier, exec;
    } sphere;
    struct { Event copy; } plane;
    struct { Event pos, mass, max_distance2; } gravity;
    struct { Event exec_barrier, exec; }
        aabb_bb, aabb_sphere, bb_sphere, sphere_gravity, sphere_plane;
    static constexpr size_t n() { return sizeof(Events) / sizeof(Event); }
    constexpr Event *begin() { return &this->counters; }
    constexpr Event *end() { return this->begin() + Events::n(); }
    constexpr const Event *begin() const { return &this->counters; }
    constexpr const Event *end() const { return this->begin() + Events::n(); }
};

class ComputeBackend final : public nngn::Colliders::Backend {
    std::size_t max_colliders = {}, max_collisions = {}, collision_bytes = {};
    uint64_t max_wg_size = {};
    nngn::Compute::Program prog = {};
    nngn::Compute::Buffer
        aabb_buffer = {}, bb_buffer = {}, sphere_buffer = {}, plane_buffer = {},
        gravity_buffer = {}, counters_buffer = {},
        aabb_coll_buffer = {}, bb_coll_buffer = {}, sphere_coll_buffer = {},
        gravity_coll_buffer = {},
        aabb_bb_coll_buffer = {}, aabb_sphere_coll_buffer = {},
        bb_sphere_coll_buffer = {}, sphere_plane_coll_buffer = {},
        sphere_gravity_coll_buffer = {};
    nngn::Compute *compute = {};
    bool init() final;
    bool read_prog(const std::filesystem::path &path);
    bool destroy();
    bool set_max_colliders(std::size_t n) final;
    bool set_max_collisions(std::size_t n) final;
    bool check(const nngn::Timing &t, const Input &input, Output *output) final;
    template<std::size_t to_off, typename To, typename From, typename F>
    bool copy_member(
        nngn::Compute::Buffer b, const std::vector<From> &v, const F *f,
        nngn::Compute::Event *e);
    bool copy_aabb(Events *events, const std::vector<nngn::AABBCollider> &v);
    bool copy_bb(Events *events, const std::vector<nngn::BBCollider> &v);
    bool copy_sphere(
        Events *events, const std::vector<nngn::SphereCollider> &v);
    bool copy_plane(Events *events, const std::vector<nngn::PlaneCollider> &v);
    bool copy_gravity(
        Events *events, const std::vector<nngn::GravityCollider> &v);
    bool check_aabb(Events *events, const std::vector<nngn::AABBCollider> &v);
    bool check_bb(Events *events, const std::vector<nngn::BBCollider> &v);
    bool check_sphere(
        const nngn::Timing &t, Events *events,
        const std::vector<nngn::SphereCollider> &v);
    bool check_aabb_bb(
        Events *events,
        const std::vector<nngn::AABBCollider> &aabb,
        const std::vector<nngn::BBCollider> &bb);
    bool check_aabb_sphere(
        Events *events,
        const std::vector<nngn::AABBCollider> &aabb,
        const std::vector<nngn::SphereCollider> &sphere);
    bool check_bb_sphere(
        Events *events,
        const std::vector<nngn::BBCollider> &aabb,
        const std::vector<nngn::SphereCollider> &sphere);
    bool check_sphere_plane(
        Events *events,
        const std::vector<nngn::SphereCollider> &sphere,
        const std::vector<nngn::PlaneCollider> &plane);
    bool check_sphere_gravity(
        Events *events,
        const std::vector<nngn::SphereCollider> &sphere,
        const std::vector<nngn::GravityCollider> &gravity);
    std::array<size_t, 2> work_size_for_n(size_t n) const;
    std::tuple<Collision*, uint32_t> map_collision_buffer(
        nngn::Compute::Buffer b,
        size_t i, nngn::Compute::Event wait, uint32_t max) const;
    template<typename T, typename U>
    bool read_collision_buffer(
        nngn::Compute::Buffer b, size_t counter_idx,
        const std::vector<T> &v0, const std::vector<U> &v1,
        const nngn::Compute::Event &wait,
        std::vector<nngn::Collision> *out);
    bool write_stats(const Events &events);
public:
    NNGN_MOVE_ONLY(ComputeBackend)
    ComputeBackend(nngn::Compute *c) : compute(c) {}
    ~ComputeBackend(void) override;
};

ComputeBackend::~ComputeBackend() {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    this->destroy();
}

bool ComputeBackend::init() {
    NNGN_LOG_CONTEXT_CF(ComputeBackend)
    std::array<uint64_t, nngn::Compute::Limit::N> limits = {};
    this->compute->get_limits(limits.data());
    this->max_wg_size = limits[nngn::Compute::Limit::WORK_GROUP_SIZE];
    return this->read_prog(nngn::Platform::src_dir / "src/cl/collision.cl");
}

bool ComputeBackend::read_prog(const std::filesystem::path &path) {
    std::string s;
    return nngn::read_file(path.string(), &s)
        && (this->prog = this->compute->create_program(s.data(), "-Werror"));
}

bool ComputeBackend::destroy() {
    if(this->prog)
        if(!this->compute->release_program(std::exchange(this->prog, {})))
            return false;
    const auto f = [this](auto *b)
        { return !*b || this->compute->release_buffer(std::exchange(*b, {})); };
    return f(&this->aabb_buffer)
        && f(&this->bb_buffer)
        && f(&this->sphere_buffer)
        && f(&this->plane_buffer)
        && f(&this->gravity_buffer)
        && f(&this->counters_buffer)
        && f(&this->aabb_coll_buffer)
        && f(&this->bb_coll_buffer)
        && f(&this->sphere_coll_buffer)
        && f(&this->gravity_coll_buffer)
        && f(&this->aabb_bb_coll_buffer)
        && f(&this->sphere_gravity_coll_buffer);
}

bool ComputeBackend::set_max_colliders(std::size_t n) {
    this->max_colliders = n;
    const auto f = [this](auto *b, auto s) {
        return (*b = this->compute->create_buffer(
            nngn::Compute::MemFlag::READ_WRITE, s, nullptr));
    };
    return f(&this->aabb_buffer, n * sizeof(AABBCollider))
        && f(&this->bb_buffer, n * sizeof(BBCollider))
        && f(&this->sphere_buffer, n * sizeof(SphereCollider))
        && f(&this->plane_buffer, n * sizeof(PlaneCollider))
        && f(&this->gravity_buffer, n * sizeof(GravityCollider))
        && f(&this->counters_buffer, COUNTERS_BYTES);
}

bool ComputeBackend::set_max_collisions(std::size_t n) {
    this->max_collisions = n;
    this->collision_bytes = n * sizeof(Collision);
    const auto f = [this](auto *b) {
        return (*b = this->compute->create_buffer(
            nngn::Compute::MemFlag::READ_WRITE,
            this->collision_bytes, nullptr));
    };
    return f(&this->aabb_coll_buffer)
        && f(&this->bb_coll_buffer)
        && f(&this->sphere_coll_buffer)
        && f(&this->gravity_coll_buffer)
        && f(&this->aabb_bb_coll_buffer)
        && f(&this->aabb_sphere_coll_buffer)
        && f(&this->bb_sphere_coll_buffer)
        && f(&this->sphere_plane_coll_buffer)
        && f(&this->sphere_gravity_coll_buffer);
}

bool ComputeBackend::check(
    const nngn::Timing &t, const Input &input, Output *output
) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    auto events = nngn::scoped(
        Events{},
        [this](auto &v) {
            const auto b = v.begin();
            const auto e = std::partition(
                b, v.end(), [](const auto &x) { return x.p; });
            if(const auto n = static_cast<size_t>(e - b))
                this->compute->release_events(n, b);
        });
    return this->compute->fill_buffer(
            this->counters_buffer, 0, COUNTERS_BYTES, {},
            {0, nullptr, &events->counters})
        && this->copy_aabb(events.get(), input.aabb)
        && this->copy_bb(events.get(), input.bb)
        && this->copy_sphere(events.get(), input.sphere)
        && this->copy_plane(events.get(), input.plane)
        && this->copy_gravity(events.get(), input.gravity)
        && this->check_aabb(events.get(), input.aabb)
        && this->check_bb(events.get(), input.bb)
        && this->check_sphere(t, events.get(), input.sphere)
        && this->check_aabb_bb(events.get(), input.aabb, input.bb)
        && this->check_aabb_sphere(events.get(), input.aabb, input.sphere)
        && this->check_bb_sphere(events.get(), input.bb, input.sphere)
        && this->check_sphere_plane(events.get(), input.sphere, input.plane)
        && this->check_sphere_gravity(events.get(), input.sphere, input.gravity)
        && this->read_collision_buffer(
            this->aabb_coll_buffer, 0,
            input.aabb, input.aabb, events->aabb.exec, &output->collisions)
        && this->read_collision_buffer(
            this->bb_coll_buffer, 1,
            input.bb, input.bb, events->bb.exec, &output->collisions)
        && this->read_collision_buffer(
            this->sphere_coll_buffer, 2,
            input.sphere, input.sphere, events->sphere.exec,
            &output->collisions)
        && this->read_collision_buffer(
            this->aabb_bb_coll_buffer, 3,
            input.aabb, input.bb, events->aabb_bb.exec, &output->collisions)
        && this->read_collision_buffer(
            this->aabb_sphere_coll_buffer, 4,
            input.aabb, input.sphere, events->aabb_bb.exec, &output->collisions)
        && this->read_collision_buffer(
            this->bb_sphere_coll_buffer, 5,
            input.bb, input.sphere, events->bb_sphere.exec, &output->collisions)
        && this->read_collision_buffer(
            this->sphere_plane_coll_buffer, 6,
            input.sphere, input.plane, events->sphere_plane.exec,
            &output->collisions)
        && this->read_collision_buffer(
            this->sphere_gravity_coll_buffer, 7,
            input.sphere, input.gravity, events->sphere_gravity.exec,
            &output->collisions)
        && this->write_stats(*events);
}

template<std::size_t to_off, typename To, typename From, typename F>
bool ComputeBackend::copy_member(
    nngn::Compute::Buffer b, const std::vector<From> &v, const F *f,
    nngn::Compute::Event *e
) {
    return this->compute->write_buffer_rect(
        b, {to_off, 0, 0}, {0, 0, 0}, {sizeof(*f), v.size(), 1},
        sizeof(To), 0, sizeof(From), 0,
        static_cast<const std::byte*>(static_cast<const void*>(f)),
        {0, nullptr, e});
}

template<typename To, typename From, typename F>
bool copy_type(
    nngn::Compute *c, nngn::Compute::Buffer b, const std::vector<From> &v, F *f,
    nngn::Compute::Event *e
) {
    return c->write_buffer_rect(
        b, {0, 0, 0}, {0, 0, 0}, {sizeof(To), v.size(), 1},
        sizeof(To), 0, sizeof(From), 0,
        static_cast<const std::byte*>(static_cast<const void*>(f)),
        {0, nullptr, e});
}

bool ComputeBackend::copy_aabb(
        Events *events, const std::vector<nngn::AABBCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    return v.empty() || copy_type<AABBCollider>(
        this->compute, this->aabb_buffer, v, &v[0].center, &events->aabb.copy);
}

bool ComputeBackend::copy_bb(
        Events *events, const std::vector<nngn::BBCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    return v.empty() || copy_type<BBCollider>(
        this->compute, this->bb_buffer, v, &v[0].center, &events->bb.copy);
}

bool ComputeBackend::copy_sphere(
        Events *events, const std::vector<nngn::SphereCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    using T = SphereCollider;
    const auto b = this->sphere_buffer;
    auto &e = events->sphere;
    return v.empty() || (
        this->copy_member<offsetof(T, pos), T>(b, v, &v[0].pos, &e.pos)
        && this->copy_member<offsetof(T, radius), T>(b, v, &v[0].r, &e.radius)
        && this->copy_member<offsetof(T, mass), T>(b, v, &v[0].m, &e.mass));
}

bool ComputeBackend::copy_plane(
        Events *events, const std::vector<nngn::PlaneCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    using T = PlaneCollider;
    const auto b = this->plane_buffer;
    auto &e = events->plane.copy;
    return v.empty() ||
        this->copy_member<offsetof(T, abcd), T>(b, v, &v[0].abcd, &e);
}

bool ComputeBackend::copy_gravity(
        Events *events, const std::vector<nngn::GravityCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    using T = GravityCollider;
    const auto b = this->gravity_buffer;
    auto &e = events->gravity;
    return v.empty() || (
        this->copy_member<offsetof(T, pos), T>(b, v, &v[0].pos, &e.pos)
        && this->copy_member<offsetof(T, mass), T>(b, v, &v[0].m, &e.mass)
        && this->copy_member<offsetof(T, max_distance2), T>(
            b, v, &v[0].max_distance2, &e.max_distance2));
}

bool ComputeBackend::check_aabb(
        Events *events, const std::vector<nngn::AABBCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n = v.size();
    if(!n)
        return true;
    const auto ws = work_size_for_n(n);
    const std::array wait = {events->counters, events->aabb.copy};
    return this->compute->execute(
        this->prog, "aabb_collision", {}, 1, &ws[0], &ws[1],
        {wait.size(), wait.data(), &events->aabb.exec_barrier},
        static_cast<uint32_t>(n),
        static_cast<uint32_t>(this->max_collisions),
        this->counters_buffer,
        this->aabb_buffer,
        this->aabb_coll_buffer);
}

bool ComputeBackend::check_bb(
        Events *events, const std::vector<nngn::BBCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n = v.size();
    if(!n)
        return true;
    const auto ws = work_size_for_n(n);
    const std::array wait = {events->counters, events->bb.copy};
    return this->compute->execute(
        this->prog, "bb_collision", {}, 1, &ws[0], &ws[1],
        {wait.size(), wait.data(), &events->bb.exec_barrier},
        static_cast<uint32_t>(n),
        static_cast<uint32_t>(this->max_collisions),
        this->counters_buffer,
        this->bb_buffer,
        this->bb_coll_buffer);
}

bool ComputeBackend::check_sphere(
        const nngn::Timing &t, Events *events,
        const std::vector<nngn::SphereCollider> &v) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n = v.size();
    if(!n)
        return true;
    const auto ws = work_size_for_n(n);
    const std::array wait =
        {events->counters, events->sphere.pos, events->sphere.radius};
    return this->compute->execute(
            this->prog, "sphere_collision", {}, 1, &ws[0], &ws[1],
            {wait.size(), wait.data(), &events->sphere.exec_barrier},
            static_cast<uint32_t>(n),
            static_cast<uint32_t>(this->max_collisions),
            this->counters_buffer,
            t.fdt_s(),
            this->sphere_buffer,
            this->sphere_coll_buffer);
}

bool ComputeBackend::check_aabb_bb(
        Events *events,
        const std::vector<nngn::AABBCollider> &aabb,
        const std::vector<nngn::BBCollider> &bb) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n_aabb = aabb.size(), n_bb = bb.size();
    if(!n_aabb || !n_bb)
        return true;
    const auto ws = work_size_for_n(n_bb);
    std::array wait = {events->counters, events->aabb.copy, events->bb.copy};
    return this->compute->execute(
        this->prog, "aabb_bb_collision", {}, 1, &ws[0], &ws[1],
        {wait.size(), wait.data(), &events->aabb_bb.exec_barrier},
        static_cast<uint32_t>(n_aabb),
        static_cast<uint32_t>(n_bb),
        static_cast<uint32_t>(this->max_collisions),
        this->counters_buffer,
        this->aabb_buffer,
        this->bb_buffer,
        this->aabb_bb_coll_buffer);
}

bool ComputeBackend::check_aabb_sphere(
        Events *events,
        const std::vector<nngn::AABBCollider> &aabb,
        const std::vector<nngn::SphereCollider> &sphere) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n_aabb = aabb.size(), n_sphere = sphere.size();
    if(!n_aabb || !n_sphere)
        return true;
    const auto ws = work_size_for_n(n_aabb);
    std::array wait = {
        events->counters, events->aabb.copy,
        events->sphere.pos, events->sphere.radius};
    return this->compute->execute(
        this->prog, "aabb_sphere_collision", {}, 1, &ws[0], &ws[1],
        {wait.size(), wait.data(), &events->aabb_sphere.exec_barrier},
        static_cast<uint32_t>(n_aabb),
        static_cast<uint32_t>(n_sphere),
        static_cast<uint32_t>(this->max_collisions),
        this->counters_buffer,
        this->aabb_buffer,
        this->sphere_buffer,
        this->aabb_sphere_coll_buffer);
}

bool ComputeBackend::check_bb_sphere(
        Events *events,
        const std::vector<nngn::BBCollider> &bb,
        const std::vector<nngn::SphereCollider> &sphere) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n_bb = bb.size(), n_sphere = sphere.size();
    if(!n_bb || !n_sphere)
        return true;
    const auto ws = work_size_for_n(n_bb);
    std::array wait = {
        events->counters, events->bb.copy,
        events->sphere.pos, events->sphere.radius};
    return this->compute->execute(
        this->prog, "bb_sphere_collision", {}, 1, &ws[0], &ws[1],
        {wait.size(), wait.data(), &events->bb_sphere.exec_barrier},
        static_cast<uint32_t>(n_bb),
        static_cast<uint32_t>(n_sphere),
        static_cast<uint32_t>(this->max_collisions),
        this->counters_buffer,
        this->bb_buffer,
        this->sphere_buffer,
        this->bb_sphere_coll_buffer);
}

bool ComputeBackend::check_sphere_plane(
        Events *events,
        const std::vector<nngn::SphereCollider> &sphere,
        const std::vector<nngn::PlaneCollider> &plane) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n_spheres = sphere.size(), n_plane = plane.size();
    if(!n_spheres || !n_plane)
        return true;
    const auto ws = work_size_for_n(n_spheres);
    std::array wait = {
        events->counters,
        events->sphere.pos, events->sphere.radius, events->plane.copy};
    return this->compute->execute(
        this->prog, "sphere_plane_collision", {}, 1, &ws[0], &ws[1],
        {wait.size(), wait.data(), &events->sphere_plane.exec_barrier},
        static_cast<uint32_t>(n_spheres),
        static_cast<uint32_t>(n_plane),
        static_cast<uint32_t>(this->max_collisions),
        this->counters_buffer,
        this->sphere_buffer,
        this->plane_buffer,
        this->sphere_plane_coll_buffer);
}

bool ComputeBackend::check_sphere_gravity(
        Events *events,
        const std::vector<nngn::SphereCollider> &sphere,
        const std::vector<nngn::GravityCollider> &gravity) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    const auto n_spheres = sphere.size(), n_gravity = gravity.size();
    if(!n_spheres || !n_gravity)
        return true;
    const auto ws = work_size_for_n(n_spheres);
    std::array wait = {
        events->counters,
        events->sphere.pos, events->sphere.mass,
        events->gravity.pos, events->gravity.mass,
        events->gravity.max_distance2};
    return this->compute->execute(
        this->prog, "sphere_gravity_collision", {}, 1, &ws[0], &ws[1],
        {wait.size(), wait.data(), &events->sphere_gravity.exec_barrier},
        static_cast<uint32_t>(n_spheres),
        static_cast<uint32_t>(n_gravity),
        static_cast<uint32_t>(this->max_collisions),
        this->counters_buffer,
        nngn::GravityCollider::G,
        this->sphere_buffer,
        this->gravity_buffer,
        this->sphere_gravity_coll_buffer);
}

std::array<size_t, 2> ComputeBackend::work_size_for_n(size_t n) const {
    const auto max = static_cast<size_t>(this->max_wg_size);
    return n < max ? std::array{n, n} : std::array{(n / max) * max, max};
}

std::tuple<Collision*, uint32_t> ComputeBackend::map_collision_buffer(
    nngn::Compute::Buffer b, size_t i, nngn::Compute::Event wait, uint32_t max
) const {
    uint32_t n = {};
    if(!this->compute->read_buffer(
            this->counters_buffer, i * sizeof(n), sizeof(n),
            static_cast<std::byte*>(static_cast<void*>(&n)),
            {1, &wait, nullptr}))
        return {};
    if(!n)
        return {nullptr, 0};
    if(max < n) {
        nngn::Log::l() << "too many collisions: " << n << '\n';
        n = max;
    }
    auto *p = static_cast<Collision*>(
        this->compute->map_buffer(
            b, nngn::Compute::READ_ONLY, 0, n * sizeof(Collision), {}));
    if(!p)
        return {};
    return {p, n};
}

template<typename T, typename U>
bool ComputeBackend::read_collision_buffer(
        nngn::Compute::Buffer b, size_t counter_idx,
        const std::vector<T> &v0, const std::vector<U> &v1,
        const nngn::Compute::Event &wait,
        std::vector<nngn::Collision> *out) {
    NNGN_LOG_CONTEXT_CF(ComputeBackend);
    if(v0.empty() || v1.empty())
        return true;
    const auto [p, n] = this->map_collision_buffer(
        b, counter_idx, wait,
        static_cast<uint32_t>(out->capacity() - out->size()));
    if(!n)
        return true;
    if(!p)
        return false;
    for(size_t i = 0; i < n; ++i) {
        const auto &ci = p[i];
        assert(ci.i0 < v0.size());
        assert(ci.i1 < v1.size());
        const auto &c0 = v0[ci.i0];
        const auto &c1 = v1[ci.i1];
        if(std::isinf(c0.m) && std::isinf(c1.m))
            continue;
        out->push_back(nngn::Collision{
            c0.entity, c1.entity, c0.m, c1.m, c0.flags, c1.flags,
            {ci.v[0], ci.v[1], ci.v[2]}});
    }
    return this->compute->unmap_buffer(b, p, {});
}

bool ComputeBackend::write_stats(const Events &events) {
    static_assert(Events::n() == nngn::CollisionStats::names.size());
    constexpr auto stats_idx = nngn::Colliders::STATS_IDX;
    constexpr auto info = static_cast<nngn::Compute::ProfInfo>(
        nngn::Compute::ProfInfo::QUEUED
            | nngn::Compute::ProfInfo::SUBMIT
            | nngn::Compute::ProfInfo::START
            | nngn::Compute::ProfInfo::END);
    if(!nngn::Stats::active(stats_idx) || !Events::n())
        return true;
    std::vector<nngn::Compute::Event> valid = {};
    valid.reserve(Events::n());
    std::copy_if(
        events.begin(), events.end(), std::back_inserter(valid),
        [](const auto &x) { return x.p; });
    std::array<uint64_t, nngn::CollisionStats::size()> tmp = {};
    auto tmp_p = tmp.data();
    if(!this->compute->prof_info(info, valid.size(), valid.data(), tmp_p))
        return false;
    const auto min =
        std::min_element(tmp.cbegin(), tmp.cbegin() + 4 * valid.size());
    auto p = static_cast<nngn::CollisionStats*>(nngn::Stats::data(stats_idx))
        ->to_u64();
    for(size_t i = 0; i < Events::n(); ++i, p += 4)
        if(!events.begin()[i].p)
            std::fill(p, p + 4, *min);
        else
            std::copy_n(std::exchange(tmp_p, tmp_p + 4), 4, p);
    return true;
}

}

namespace nngn {

auto Colliders::compute_backend(Compute *c) -> std::unique_ptr<Backend>
    { return std::make_unique<ComputeBackend>(c); }

}
