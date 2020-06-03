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

static uint sphere_grid_idx(struct sphere_params *params, float4 pos) {
    const float s = params->size;
    float2 gpos = pos.xy;
    if(pos.x < 0)
        pos.x = s - pos.x;
    if(pos.y < 0)
        pos.y = s - pos.y;
    gpos = fmod(fabs(gpos) / s, params->n_cells);
    return params->n_cells * gpos.y + gpos.x;
}

__kernel void sphere_gen_grid(
        struct sphere_params params, uint n,
        __global const float4 *pos,
        __global uint *grid, __global uint *grid_count) {
    const uint id = get_global_id(0);
    if(id >= n)
        return;
    const uint gi = sphere_grid_idx(&params, pos[id]);
    const uint i = atomic_inc(grid_count + gi);
    if(i < params.max_idx)
        grid[params.max_idx * gi + i] = id;
}

__kernel void sphere_collision(
        struct sphere_params params, uint n, uint max_collisions,
        __global uint *counters, float radius, float dt,
        __global const uint *grid, __global const uint *grid_count,
        __global const float4 *pos, __global const float4 *vel,
        __global struct collision *out) {
    const uint id = get_global_id(0);
    if(id >= n)
        return;
    const float dt2 = dt * dt;
    const float4 ipos = pos[id], ivel = vel[id];
    const uint gi = sphere_grid_idx(&params, ipos);
    const uint4 n_cells = params.n_cells;
    const uint max_idx = params.max_idx;
//    float4 f = out[id];
    for(int gny = -1; gny <= 1; ++gny)
        for(int gnx = -1; gnx <= 1; ++gnx) {
            const int gn = n_cells[0] * gny + gnx + gi;
            if(gn < 0 || n_cells[0] * n_cells[1] <= gn)
                continue;
            uint i = gn * max_idx, e = i + min(max_idx, grid_count[gn]);
            for(; i < e; ++i) {
                const uint oid = grid[i];
                if(id >= oid)
                    continue;
                const float4 opos = pos[oid];
                float4 d = ipos - opos;
                const float l2 = dot(d, d);
                if(l2 > 4 * radius * radius || !l2)
                    continue;
                const uint coll_id = atomic_inc(counters + 2);
                if(coll_id >= max_collisions)
                    return;
                const float l = sqrt(l2);
                const float4 ovel = vel[oid];
//                f += (2 * radius - l) / l * d/* / dt2*/;
//                f += (ovel - ivel) / dt;
                const float4 f = (2 * radius - l) / l * d/* / dt2*/;
                out[coll_id] = (struct collision){f, id, oid};
            }
        }
//    if(ipos.x < params.min[0])
//        f.x += (params.min[0] - ipos.x) - ivel.x * dt;
//    if(params.max[0] < ipos.x)
//        f.x += (params.max[0] - ipos.x) - ivel.x * dt;
//    if(ipos.y < params.min[1])
//        f.y += (params.min[1] - ipos.y) - ivel.y * dt;
//    if(params.max[1] < ipos.y)
//        f.y += (params.max[1] - ipos.y) - ivel.y * dt;
//    f.y -= params.g;
//    out[id] = f;
}

void check_sphere(
        uint n, uint max_collisions, __global uint *counters,
        __global const float4 *center_x_v, __global const float4 *center_y_v,
        __global const float4 *radius_v, __global const float4 *mass_v,
        __global struct collision *out) {
    const uint id = get_global_id(0);
    const float4 inf = INFINITY;
    int4 ids = {id, id + 1, id + 2, id + 3};
    const float4 center_x = center_x_v[id];
    const float4 center_y = center_y_v[id];
    const float4 radius = radius_v[id];
    const float4 mass = mass_v[id];
    for(size_t i = id + 1; i < n; ++i) {
        // XXX
        const float4 distance_x = center_x - center_x_v[i / 4][i % 4];
        const float4 distance_y = center_y - center_y_v[i / 4][i % 4];
        const float4 distance2_x = distance_x * distance_x;
        const float4 distance2_y = distance_y * distance_y;
        const float4 length2 = distance2_x + distance2_y;
        const float4 radii = radius + radius_v[i / 4][i % 4];
        const int4 dist_check = (length2 != 0) & (length2 < (radii * radii));
        const int4 mass_check = (mass == inf) & (mass_v[i / 4][i % 4] == inf);
        const int4 id_check = ids < (int)i;
        const int4 check = (!mass_check & dist_check) & id_check;
//        int mask = _mm_movemask_epi8(check);
        // XXX
//        if(check == (int4)0) // mask)
//            continue;
        float4 length = sqrt(length2);
        length = (radii - length) / length;
        length = select(length, (float4)0, check);
        const float4 coll_x = distance_x * length;
        const float4 coll_y = distance_y * length;
        for(size_t j = 0; j < 4; ++j) {
            if(!check[j])
                continue;
            const uint coll_id = atomic_inc(counters + 3);
            if(coll_id >= max_collisions)
                return;
            out[coll_id] =
                (struct collision){(float4)(coll_x[j], coll_y[j], 0, 0), id};
        }
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
