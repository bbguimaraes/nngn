#ifndef NNGN_COLLISION_H
#define NNGN_COLLISION_H

#include <memory>
#include <vector>

#include "timing/stats.h"
#include "utils/utils.h"

#include "colliders.h"

struct lua_State;
struct Entity;

namespace nngn {

struct Compute;
struct Timing;

struct Collision {
    Entity *entity0 = nullptr, *entity1 = nullptr;
    float mass0 = 0, mass1 = 0;
    Flags<Collider::Flag> flags0 = {}, flags1 = {};
    vec3 normal = {};
    float length = {};
};

struct CollisionStats : StatsBase<CollisionStats, 4> {
    std::array<uint64_t, 4>
        counters,
        aabb_copy, aabb_exec_barrier, aabb_exec,
        bb_copy, bb_exec_barrier, bb_exec,
        sphere_pos, sphere_vel, sphere_mass, sphere_radius,
        sphere_grid_count, sphere_exec_grid_barrier,
        sphere_exec_grid, sphere_exec_barrier, sphere_exec,
        plane,
        gravity_pos, gravity_mass, gravity_max_distance2,
        aabb_bb_exec_barrier, aabb_bb_exec,
        aabb_sphere_exec_barrier, aabb_sphere_exec,
        bb_sphere_exec_barrier, bb_sphere_exec,
        sphere_plane_exec_barrier, sphere_plane_exec,
        sphere_gravity_exec_barrier, sphere_gravity_exec;
    static constexpr std::array names = {
        "counters",
        "aabb_copy", "aabb_exec_barrier", "aabb_exec",
        "bb_copy", "bb_exec_barrier", "bb_exec",
        "sphere_pos", "sphere_vel", "sphere_mass", "sphere_radius",
        "sphere_grid_count", "sphere_exec_grid_barrier",
        "sphere_exec_grid", "sphere_exec_barrier", "sphere_exec",
        "plane",
        "gravity_pos", "gravity_mass", "gravity_max_distance2",
        "aabb_bb_exec_barrier", "aabb_bb_exec",
        "aabb_sphere_exec_barrier", "aabb_sphere_exec",
        "bb_sphere_exec_barrier", "bb_sphere_exec",
        "sphere_plane_exec_barrier", "sphere_plane_exec",
        "sphere_gravity_exec_barrier", "sphere_gravity_exec"};
    const uint64_t *to_u64() const { return this->counters.data(); }
    uint64_t *to_u64() { return this->counters.data(); }
};

struct Colliders {
    struct Backend {
        struct Input {
            std::vector<AABBCollider> aabb = {};
            std::vector<BBCollider> bb = {};
            std::vector<SphereCollider> sphere = {};
            std::vector<PlaneCollider> plane = {};
            std::vector<GravityCollider> gravity = {};
        };
        struct Output {
            CollisionStats stats = {};
            std::vector<Collision> collisions = {};
        };
        NNGN_NO_COPY(Backend)
        Backend() = default;
        virtual ~Backend() = default;
        virtual bool init() { return true; }
        virtual bool set_max_colliders(size_t) { return true; }
        virtual bool set_max_collisions(size_t) { return true; }
        virtual bool check(const Timing&, const Input&, Output*)
            { return true; }
    };
private:
    enum Flag : uint8_t {
        CHECK = 1u << 0, RESOLVE = 1u << 1,
        MAX_COLLIDERS_UPDATED = 1u << 2, MAX_COLLISIONS_UPDATED = 1u << 3,
    };
    Flags<Flag> m_flags = {static_cast<Flag>(Flag::CHECK | Flag::RESOLVE)};
    size_t m_max_colliders = 0;
    Backend::Input input = {};
    Backend::Output output = {};
    std::unique_ptr<Backend> backend = {};
public:
    using Stats = CollisionStats;
    static constexpr size_t STATS_IDX = 1;
    static std::unique_ptr<Backend> native_backend();
    static std::unique_ptr<Backend> compute_backend(Compute *c);
    NNGN_NO_COPY(Colliders)
    Colliders();
    ~Colliders();
    auto &aabb() const { return this->input.aabb; }
    auto &bb() const { return this->input.bb; }
    auto &sphere() const { return this->input.sphere; }
    auto &plane() const { return this->input.plane; }
    auto &gravity() const { return this->input.gravity; }
    size_t max_colliders() const { return this->m_max_colliders; }
    size_t max_collisions() const { return this->output.collisions.capacity(); }
    auto &collisions() const { return this->output.collisions; }
    bool check() const { return this->m_flags.is_set(Flag::CHECK); }
    bool resolve() const { return this->m_flags.is_set(Flag::RESOLVE); }
    bool has_backend() const { return static_cast<bool>(this->backend); }
    void set_check(bool b) { this->m_flags.set(Flag::CHECK, b); }
    void set_resolve(bool b) { this->m_flags.set(Flag::RESOLVE, b); }
    bool set_max_colliders(size_t n);
    bool set_max_collisions(size_t n);
    bool set_backend(std::unique_ptr<Backend> p);
    AABBCollider *add(const AABBCollider &c);
    BBCollider *add(const BBCollider &c);
    SphereCollider *add(const SphereCollider &c);
    PlaneCollider *add(const PlaneCollider &c);
    GravityCollider *add(const GravityCollider &c);
    Collider *load(const sol::stack_table &t);
    void remove(Collider *p);
    void clear();
    bool check_collisions(const Timing &t);
    void resolve_collisions() const;
    void lua_on_collision(lua_State *L);
};

}

#endif
