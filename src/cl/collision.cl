struct Collision { float4 v; uint i0, i1; };
struct AABBCollider { float2 center, bl, tr; float radius; };
struct BBCollider { float2 center, bl, tr; float radius, cos, sin; };
struct SphereCollider { float3 pos; float mass, radius; };
struct PlaneCollider { float4 abcd; };
struct GravityCollider { float3 pos; float mass, max_distance2; };

float length2(float2 v);
bool check_bb_fast(
    __global const struct AABBCollider *c0,
    __global const struct AABBCollider *c1);
float overlap(float min0, float max0, float min1, float max1);
bool float_eq_zero(float f);
bool float_gt_zero(float f);
float2 rotate(float2 p, float cos, float sin);
void to_edges(float2 bl, float2 tr, float2 out[4]);
bool check_bb_common(float2 bl0, float2 tr0, float2 v1[4], float2 *out);
bool check_bb_sphere_common(
    float2 c0, float2 bl0, float2 tr0, float2 sc, float sr, float2 *out);

// TODO grid / local mem
__kernel void aabb_collision(
        uint n, uint max_collisions, __global uint *counters,
        __global const struct AABBCollider *aabb,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n)
        return;
    __global const struct AABBCollider
        *const c0 = aabb + id,
        *const e = aabb + n,
        *c1 = c0 + 1;
    for(; c1 != e; ++c1) {
        if(!check_bb_fast(c0, c1))
            continue;
        const float xoverlap = overlap(c0->bl.x, c0->tr.x, c1->bl.x, c1->tr.x);
        if(float_eq_zero(xoverlap))
            continue;
        const float yoverlap = overlap(c0->bl.y, c0->tr.y, c1->bl.y, c1->tr.y);
        if(float_eq_zero(yoverlap))
            continue;
        const uint coll_id = atomic_inc(counters);
        if(coll_id >= max_collisions)
            return;
        const float2 v = fabs(xoverlap) <= fabs(yoverlap)
            ? (float2){-xoverlap, 0}
            : (float2){0, -yoverlap};
        out[coll_id] = (struct Collision){(float4)(v, 0, 0), id, c1 - aabb};
    }
}

__kernel void bb_collision(
        uint n, uint max_collisions, __global uint *counters,
        __global const struct BBCollider *bb,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n)
        return;
    __global const struct BBCollider
        *const c0 = bb + id,
        *const e = bb + n,
        *c1 = c0 + 1;
    const float2 rel_bl0 = c0->bl - c0->center, rel_tr0 = c0->tr - c0->center;
    for(; c1 != e; ++c1) {
        if(!check_bb_fast(
                (__global const struct AABBCollider*)c0,
                (__global const struct AABBCollider*)c1))
            continue;
        const float2 rel_bl1 = c1->bl - c1->center;
        const float2 rel_tr1 = c1->tr - c1->center;
        float2 edges[4] = {};
        to_edges(rel_bl0, rel_tr0, edges);
        for(uint i = 0; i < 4; ++i)
            edges[i] = rotate(
                rotate(edges[i], c0->cos, c0->sin) + c0->center - c1->center,
                c1->cos, -c1->sin);
        float2 v0 = {};
        if(!check_bb_common(rel_bl1, rel_tr1, edges, &v0))
            continue;
        to_edges(rel_bl1, rel_tr1, edges);
        for(uint i = 0; i < 4; ++i)
            edges[i] = rotate(
                rotate(edges[i], c1->cos, c1->sin) + c1->center - c0->center,
                c0->cos, -c0->sin);
        float2 v1 = {};
        if(!check_bb_common(rel_bl0, rel_tr0, edges, &v1))
            continue;
        const uint coll_id = atomic_inc(counters + 1);
        if(coll_id >= max_collisions)
            return;
        v0 = length2(v0) <= length2(v1)
            ? -rotate(v0, c1->cos, c1->sin)
            : rotate(v1, c0->cos, c0->sin);
        out[coll_id] = (struct Collision){(float4)(v0, 0, 0), id, c1 - bb};
    }
}

__kernel void sphere_collision(
        uint n, uint max_collisions,
        __global uint *counters, float dt,
        __global const struct SphereCollider *sphere,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n)
        return;
    __global const struct SphereCollider
        *const c0 = sphere + id,
        *const e = sphere + n,
        *c1 = c0 + 1;
    const float3 pos0 = c0->pos;
    const float radius0 = c0->radius;
    for(; c1 != e; ++c1) {
        const float3 d = pos0 - c1->pos;
        const float r = radius0 + c1->radius;
        const float l2 = dot(d, d);
        if(l2 >= r * r || l2 == 0)
            continue;
        const uint coll_id = atomic_inc(counters + 2);
        if(coll_id >= max_collisions)
            return;
        const float l = sqrt(l2);
        const float3 v = (r - l) / l * d;
        out[coll_id] = (struct Collision){(float4)(v, 0), id, c1 - sphere};
    }
}

__kernel void aabb_bb_collision(
        uint n_aabb, uint n_bb, uint max_collisions, __global uint *counters,
        __global const struct AABBCollider *aabb,
        __global const struct BBCollider *bb,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n_bb)
        return;
    __global const struct BBCollider *const c0 = bb + id;
    __global const struct AABBCollider *c1 = aabb, *const e = c1 + n_aabb;
    const float2 rel_bl0 = c0->bl - c0->center, rel_tr0 = c0->tr - c0->center;
    for(; c1 != e; ++c1) {
        if(!check_bb_fast((__global const struct AABBCollider*)c0, c1))
            continue;
        const float2 rel_bl1 = c1->bl - c1->center;
        const float2 rel_tr1 = c1->tr - c1->center;
        float2 edges[4] = {};
        to_edges(rel_bl0, rel_tr0, edges);
        for(uint i = 0; i < 4; ++i)
            edges[i] = rotate(edges[i], c0->cos, c0->sin)
                + c0->center - c1->center;
        float2 v0 = {};
        if(!check_bb_common(rel_bl1, rel_tr1, edges, &v0))
            continue;
        to_edges(c1->bl, c1->tr, edges);
        for(uint i = 0; i < 4; ++i)
            edges[i] = rotate(edges[i] - c0->center, c0->cos, -c0->sin);
        float2 v1 = {};
        if(!check_bb_common(rel_bl0, rel_tr0, edges, &v1))
            continue;
        const uint coll_id = atomic_inc(counters + 3);
        if(coll_id >= max_collisions)
            return;
        v0 = length2(v0) <= length2(v1)
            ? v0 : -rotate(v1, c0->cos, c0->sin);
        out[coll_id] = (struct Collision){(float4)(v0, 0, 0), c1 - aabb, id};
    }
}

__kernel void aabb_sphere_collision(
        uint n_aabb, uint n_sphere,
        uint max_collisions, __global uint *counters,
        __global const struct AABBCollider *aabb,
        __global const struct SphereCollider *sphere,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n_aabb)
        return;
    __global const struct AABBCollider *const c0 = aabb + id;
    __global const struct SphereCollider
        *c1 = sphere,
        *const e = sphere + n_sphere;
    for(; c1 != e; ++c1) {
        float2 v = {};
        if(!check_bb_sphere_common(
                c0->center, c0->bl, c0->tr, c1->pos.xy, c1->radius, &v))
            continue;
        const uint coll_id = atomic_inc(counters + 4);
        if(coll_id >= max_collisions)
            return;
        out[coll_id] = (struct Collision){(float4)(v, 0, 0), id, c1 - sphere};
    }
}

__kernel void bb_sphere_collision(
        uint n_bb, uint n_sphere,
        uint max_collisions, __global uint *counters,
        __global const struct BBCollider *bb,
        __global const struct SphereCollider *sphere,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n_bb)
        return;
    __global const struct BBCollider *c0 = bb + id;
    __global const struct SphereCollider
        *c1 = sphere,
        *const e = c1 + n_sphere;
    for(; c1 != e; ++c1) {
        float2 v = {};
        if(!check_bb_sphere_common(
                c0->center, c0->bl, c0->tr,
                c0->center + rotate(
                    c1->pos.xy - c0->center,
                    c0->cos, -c0->sin),
                c1->radius, &v))
            continue;
        v = rotate(v, c0->cos, c0->sin);
        const uint coll_id = atomic_inc(counters + 5);
        if(coll_id >= max_collisions)
            return;
        out[coll_id] = (struct Collision){(float4)(v, 0, 0), id, c1 - sphere};
    }
}

__kernel void sphere_plane_collision(
        uint n_spheres, uint n_planes,
        uint max_collisions, __global uint *counters,
        __global const struct SphereCollider *sphere,
        __global const struct PlaneCollider *plane,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n_spheres)
        return;
    __global const struct SphereCollider *const c0 = sphere + id;
    __global const struct PlaneCollider
        *c1 = plane,
        *const e = c1 + n_planes;
    const float3 pos = c0->pos.xyz;
    const float radius = c0->radius;
    for(; c1 != e; ++c1) {
        float3 n = c1->abcd.xyz;
        float d = dot(n, pos) + c1->abcd[3] - radius;
        if(float_gt_zero(d))
            continue;
        const uint coll_id = atomic_inc(counters + 6);
        if(coll_id >= max_collisions)
            return;
        float3 v = n * -d;
        out[coll_id] = (struct Collision){(float4)(v, 0), id, c1 - plane};
    }
}

__kernel void sphere_gravity_collision(
        uint n_spheres, uint n_gravity, uint max_collisions,
        __global uint *counters, float g,
        __global const struct SphereCollider *sphere,
        __global const struct GravityCollider *gravity,
        __global struct Collision *out) {
    const uint id = get_global_id(0);
    if(id >= n_spheres)
        return;
    __global const struct SphereCollider *const c0 = sphere + id;
    __global const struct GravityCollider
        *c1 = gravity,
        *const e = c1 + n_gravity;
    const float3 pos = c0->pos;
    const float mass = c0->mass;
    for(; c1 != e; ++c1) {
        const float3 d = c1->pos - pos;
        const float d2 = dot(d, d);
        if(d2 > c1->max_distance2 || !d2)
            continue;
        const uint coll_id = atomic_inc(counters + 7);
        if(coll_id >= max_collisions)
            return;
        float l = g * mass * c1->mass / d2 / sqrt(d2);
        out[coll_id] = (struct Collision){(float4)(d * l, 0), id, c1 - gravity};
    }
}

float length2(float2 v) { return dot(v, v); }

bool check_bb_fast(
        __global const struct AABBCollider *c0,
        __global const struct AABBCollider *c1) {
    return length2(c1->center - c0->center)
        < (c0->radius + c1->radius) * (c0->radius + c1->radius);
}

float overlap(float min0, float max0, float min1, float max1) {
    return (min1 > max0 || max1 < min0) ? 0.0f
        : (max0 > max1) ? (min0 - max1)
        : (max0 - min1);
}

bool float_eq_zero(float f) { return fabs(f) <= FLT_EPSILON * 2; }
bool float_gt_zero(float f) { return f > -FLT_EPSILON * 2; }

float2 rotate(float2 p, float cos, float sin)
    { return (float2){p.x * cos - p.y * sin, p.x * sin + p.y * cos}; }

void to_edges(float2 bl, float2 tr, float2 out[4]) {
    out[0] = (float2){bl.x, bl.y};
    out[1] = (float2){tr.x, bl.y};
    out[2] = (float2){tr.x, tr.y};
    out[3] = (float2){bl.x, tr.y};
}

bool check_bb_common(float2 bl0, float2 tr0, float2 v1[4], float2 *out) {
    float min_x = fmin(v1[0].x, fmin(v1[1].x, fmin(v1[2].x, v1[3].x)));
    float max_x = fmax(v1[0].x, fmax(v1[1].x, fmax(v1[2].x, v1[3].x)));
    const float xoverlap = overlap(bl0.x, tr0.x, min_x, max_x);
    if(float_eq_zero(xoverlap))
        return false;
    float min_y = fmin(v1[0].y, fmin(v1[1].y, fmin(v1[2].y, v1[3].y)));
    float max_y = fmax(v1[0].y, fmax(v1[1].y, fmax(v1[2].y, v1[3].y)));
    const float yoverlap = overlap(bl0.y, tr0.y, min_y, max_y);
    if(float_eq_zero(yoverlap))
        return false;
    *out = fabs(xoverlap) <= fabs(yoverlap)
        ? (float2){-xoverlap, 0} : (float2){0, -yoverlap};
    return true;
}

bool check_bb_sphere_common(
        float2 c0, float2 bl0, float2 tr0, float2 sc, float sr, float2 *out) {
    const float2 nearest = {
        clamp(sc.x, bl0.x, tr0.x),
        clamp(sc.y, bl0.y, tr0.y)};
    const float2 d = sc - nearest;
    const float l2 = length2(d);
    if(l2 != 0) {
        if(l2 > sr * sr)
            return false;
        *out = d * (1 - sr / sqrt(l2));
        return true;
    }
    const float2 rd = tr0 - bl0;
    const float2 rd_2 = rd / 2.0f;
    const float2 v = sc - c0;
    const float a = v.y / v.x;
    const float2 vx = (v.x > 0 ? 1.0f : -1.0f) * (float2){rd_2.x, a * rd_2.x};
    const float2 vy = (v.y > 0 ? 1.0f : -1.0f) * (float2){rd_2.y / a, rd_2.y};
    const float2 proj = c0 + (fabs(vx.y) < fabs(rd.y) ? vx : vy);
    *out = (float2)sc - (float2){c0 + proj * (1 + sr / length(proj))};
    return true;
}
