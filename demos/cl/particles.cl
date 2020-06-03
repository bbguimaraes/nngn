struct params {
    uint max_idx, n_cells;
    float size, radius, elast, g;
};

static uint grid_idx(uint n_cells, float size, float4 pos) {
    const float s = size;
    float2 gpos = pos.xy;
    if(pos.x < 0)
        pos.x = s - pos.x;
    if(pos.y < 0)
        pos.y = s - pos.y;
    gpos = fmod(fabs(gpos) / s, n_cells);
    return n_cells * gpos.y + gpos.x;
}

__kernel void grid(
    __constant struct params *params, __global const float4 *pos,
    __global uint *grid, __global uint *grid_count
) {
    const uint id = get_global_id(0);
    const uint gi = grid_idx(params->n_cells, params->size, pos[id]);
    const uint i = atomic_inc(grid_count + gi);
    if(i < params->max_idx)
        grid[params->max_idx * gi + i] = id;
    else
        printf("bucket %d full\n", gi);
}

<<<<<<< HEAD
static float4 collision_grid(
    uint max_idx, float radius, float elast, float dt,
    __global const uint *grid, __global const uint *grid_count,
    __global const float4 *pos, __global const float4 *vel,
    float4 pos0, float4 vel0, uint grid_idx
) {
    float4 ret = {};
=======
__kernel void old_collision(
        struct sphere_params params, float radius, float dt,
        __global const uint *grid, __global const uint *grid_count,
        __global const float4 *pos, __global const float4 *vel,
        __global float4 *forces) {
    const uint id = get_global_id(0);
>>>>>>> 23ad430a5 (wip simd)
    const float dt2 = dt * dt;
    uint i = grid_idx * max_idx;
    const uint e = i + min(max_idx, grid_count[grid_idx]);
    for(; i < e; ++i) {
        const uint id1 = grid[i];
        const float4 pos1 = pos[id1];
        const float4 vel1 = vel[id1];
        float4 d = pos0 - pos1;
        const float l2 = dot(d, d);
        if(!l2 || l2 > 4 * radius * radius)
            continue;
        const float l = sqrt(l2);
        ret += (2 * radius - l) / l * d / dt2 * 0.5f;
        ret += (vel1 - vel0) / dt * elast;
    }
    return ret;
}

void collision_grids(
    uint max_idx, uint n_cells, float radius, float elast, float dt,
    __global const uint *grid, __global const uint *grid_count,
    __global const float4 *pos, __global const float4 *vel,
    __global float4 *forces, uint grid_idx, uint id
) {
    if(id >= grid_count[grid_idx])
        return;
    const uint gi = grid[grid_idx * max_idx + id];
    const float4 pos0 = pos[gi];
    const float4 vel0 = vel[gi];
    const uint xc = grid_idx % n_cells;
    const uint yc = grid_idx / n_cells;
    const uint x0 = sub_sat(xc, 1u);
    const uint y0 = sub_sat(yc, 1u);
    const uint x1 = min(n_cells - 1, xc + 1);
    const uint y1 = min(n_cells - 1, yc + 1);
    float4 f = {};
    for(uint y = y0; y <= y1; ++y)
        for(uint x = x0; x <= x1; ++x)
            f += collision_grid(
                max_idx, radius, elast, dt, grid, grid_count, pos, vel,
                pos0, vel0, n_cells * y + x);
    forces[gi] += f;
}

static float4 collision_planes(
    float size, float radius, float elast, float g, float dt,
    __global const float4 *pos, __global const float4 *vel, uint id
) {
    const float dt2 = dt * dt;
    const float4 ipos = pos[id];
    const float4 ivel = vel[id];
    float4 f = {};
    float d = 0;
    if((d = ipos.x - radius - -size) < 0)
        f.x += (-d / dt2 - ivel.x / dt * elast);
    if((d = ipos.x + radius - size) > 0)
        f.x += (-d / dt2 - ivel.x / dt * elast);
    if((d = ipos.y - radius - -size) < 0)
        f.y += (-d / dt2 - ivel.y / dt * elast);
    if((d = ipos.y + radius - size) > 0)
        f.y += (-d / dt2 - ivel.y / dt * elast);
    f.y -= g;
    return f;
}

__kernel void collision_by_particle(
    __constant struct params *params, float dt,
    __global const uint *grid, __global const uint *grid_count,
    __global const float4 *pos, __global const float4 *vel,
    __global float4 *forces
) {
    const uint id = get_global_id(0);
    const float4 ipos = pos[id];
    const float4 ivel = vel[id];
    const uint gi = grid_idx(params->n_cells, params->size, ipos);
    const uint4 n_cells = params->n_cells;
    const uint max_idx = params->max_idx;
    const uint xc = gi % n_cells[0];
    const uint yc = gi / n_cells[0];
    const uint x0 = sub_sat(xc, 1u);
    const uint y0 = sub_sat(yc, 1u);
    const uint x1 = min(n_cells[0] - 1, xc + 1);
    const uint y1 = min(n_cells[1] - 1, yc + 1);
    float4 f = forces[id];
    for(uint y = y0; y <= y1; ++y)
        for(uint x = x0; x <= x1; ++x)
            f += collision_grid(
                max_idx, params->radius, params->elast,
                dt, grid, grid_count, pos, vel,
                ipos, ivel, n_cells[0] * y + x);
    f += collision_planes(
        params->size, params->radius, params->elast, params->g,
        dt, pos, vel, id);
    forces[id] = f;
}

<<<<<<< HEAD
__kernel void collision_by_grid(
    __constant struct params *params, float dt,
    __global const uint *grid, __global const uint *grid_count,
    __global const float4 *pos, __global const float4 *vel,
    __global float4 *forces
) {
    const uint id = get_global_id(0);
    const uint n = params->n_cells * params->n_cells;
    for(uint i = 0; i < n; ++i)
        collision_grids(
            params->max_idx, params->n_cells, params->radius, params->elast, dt,
            grid, grid_count, pos, vel, forces, i, id);
    forces[id] += collision_planes(
        params->size, params->radius, params->elast, params->g,
        dt, pos, vel, id);
}

__kernel void integrate(
    float dt,
    __global float4 *forces, __global float4 *vel, __global float4 *pos
) {
=======
__kernel void collision0(
        struct sphere_params params, uint n, float dt,
        __global const uint *grid, __global const uint *grid_count,
        __global const float4 *center_x_v, __global const float4 *center_y_v,
        __global const float4 *radius_v, __global const float4 *mass_v,
        __global const float4 *vel_x_v, __global const float4 *vel_y_v,
        __global float4 *out_x, __global float4 *out_y) {
    const uint id = get_global_id(0);
    const float dt2 = dt * dt;
    const float elast = 0.15f;
    const float4 inf = INFINITY;
    const int4 ids = {id, id + 1, id + 2, id + 3};
    const float4 center_x = center_x_v[id];
    const float4 center_y = center_y_v[id];
    const float4 radius = radius_v[id];
    const float4 mass = mass_v[id];
    const float4 vel_x = vel_x_v[id];
    const float4 vel_y = vel_y_v[id];
    float4 force_x = out_x[id], force_y = out_y[id];
    for(size_t i = 0; i < n; ++i) {
        const float4 distance_x = center_x - center_x_v[i / 4][i % 4];
        const float4 distance_y = center_y - center_y_v[i / 4][i % 4];
        const float4 distance2_x = distance_x * distance_x;
        const float4 distance2_y = distance_y * distance_y;
        const float4 length2 = distance2_x + distance2_y;
        const float4 radii = radius + radius_v[i / 4][i % 4];
        const int4 dist_check = (length2 != 0) & (length2 < (radii * radii));
        const int4 mass_check = (mass == inf) & (mass_v[i / 4][i % 4] == inf);
        const int4 id_check = ids != (int)i; //< (int)i;
        const int4 check = !mass_check & dist_check & id_check;
        float4 length = sqrt(length2);
        length = (radii - length) / length;
        const float4 ovel_x = vel_x_v[i / 4][i % 4];
        const float4 ovel_y = vel_y_v[i / 4][i % 4];
        force_x += select((float4)0, length * distance_x / dt2 * 0.5f, check);
        force_y += select((float4)0, length * distance_y / dt2 * 0.5f, check);
        force_x += select((float4)0, (ovel_x - vel_x) / dt * elast, check);
        force_y += select((float4)0, (ovel_y - vel_y) / dt * elast, check);
    }
    for(uint i = 0; i < 4; ++i) {
        float d = center_x[i] - radius[i] - -params.size;
        if(d < 0)
            force_x[i] += (-d / dt2 - vel_x[i] / dt * elast);
        d = center_x[i] + radius[i] - params.size;
        if(d > 0)
            force_x[i] += (-d / dt2 - vel_x[i] / dt * elast);
        d = center_y[i] - radius[i] - -params.size;
        if(d < 0)
            force_y[i] += (-d / dt2 - vel_y[i] / dt * elast);
        d = center_y[i] + radius[i] - params.size;
        if(d > 0)
            force_y[i] += (-d / dt2 - vel_y[i] / dt * elast);
    }
    force_y -= 32.0f * 9.8f;
    out_x[id] = force_x;
    out_y[id] = force_y;
}

__kernel void collision1(
        struct sphere_params params, uint n, float dt,
        __global const uint *grid, __global const uint *grid_count,
        __global const float *center_x_v, __global const float *center_y_v,
        __global const float *radius_v, __global const float *mass_v,
        __global const float *vel_x_v, __global const float *vel_y_v,
        __global float *out_x, __global float *out_y) {
    const uint id = get_global_id(0);
    const float dt2 = dt * dt;
    const float elast = 0.15f;
    const float4 inf = INFINITY;
    const int4 ids = {id, id + 1, id + 2, id + 3};
    const float center_x = center_x_v[id];
    const float center_y = center_y_v[id];
    const float radius = radius_v[id];
    const float mass = mass_v[id];
    const float vel_x = vel_x_v[id];
    const float vel_y = vel_y_v[id];
    float force_x = out_x[id], force_y = out_y[id];
    const uint gi = grid_idx(&params, (float4)(center_x, center_y, 0, 0));
    const uint4 n_cells = params.n_cells;
    const uint max_idx = params.max_idx;
    for(int gny = -1; gny <= 1; ++gny)
        for(int gnx = -1; gnx <= 1; ++gnx) {
            const int gn = n_cells[0] * gny + gnx + gi;
            if(gn < 0 || n_cells[0] * n_cells[1] <= gn)
                continue;
            uint i = gn * max_idx, e = i + min(max_idx, grid_count[gn]);
            while(i < e) {
        float4 distance_x, distance_y, radii, i_mass;
        for(uint j = 0; j < 4; ++j) {
            const uint oid = grid[i++];
            distance_x[j] = center_x_v[oid];
            distance_y[j] = center_y_v[oid];
            radii[j] = radius_v[oid];
            i_mass[j] = mass_v[oid];
        }
        distance_x = center_x - distance_x;
        distance_y = center_y - distance_x;
        radii += radius;
        const float4 distance2_x = distance_x * distance_x;
        const float4 distance2_y = distance_y * distance_y;
        const float4 length2 = distance2_x + distance2_y;
        const int4 dist_check = (length2 != 0) & (length2 < (radii * radii));
        const int4 mass_check = (mass == inf) & (mass_v[i] == inf);
        const int4 id_check = ids != (int)i; //< (int)i;
        const int4 check = !mass_check & dist_check & id_check;
        float4 length = sqrt(length2);
        length = (radii - length) / length;
        const float4 ovel_x = vel_x_v[i];
        const float4 ovel_y = vel_y_v[i];
        float4 f_x = {}, f_y = {};
        f_x += select((float4)0, length * distance_x / dt2 * 0.5f, check);
        f_y += select((float4)0, length * distance_y / dt2 * 0.5f, check);
        f_x += select((float4)0, (ovel_x - vel_x) / dt * elast, check);
        f_y += select((float4)0, (ovel_y - vel_y) / dt * elast, check);
        force_x += f_x[0] + f_x[1] + f_x[2] + f_x[3];
        force_y += f_y[0] + f_y[1] + f_y[2] + f_y[3];
        }
    }
    float d = center_x - radius - -params.size;
    if(d < 0)
        force_x += (-d / dt2 - vel_x / dt * elast);
    d = center_x + radius - params.size;
    if(d > 0)
        force_x += (-d / dt2 - vel_x / dt * elast);
    d = center_y - radius - -params.size;
    if(d < 0)
        force_y += (-d / dt2 - vel_y / dt * elast);
    d = center_y + radius - params.size;
    if(d > 0)
        force_y += (-d / dt2 - vel_y / dt * elast);
    force_y -= 32.0f * 9.8f;
    out_x[id] = force_x;
    out_y[id] = force_y;
}

__kernel void integrate(
        float dt,
        __global float4 *force_x,
        __global float4 *force_y,
        __global float4 *vel_x,
        __global float4 *vel_y,
        __global float4 *center_x,
        __global float4 *center_y) {
>>>>>>> 23ad430a5 (wip simd)
    const uint id = get_global_id(0);
    center_x[id] += dt * (vel_x[id] += dt * force_x[id]);
    center_y[id] += dt * (vel_y[id] += dt * force_y[id]);
    force_x[id] = 0;
    force_y[id] = 0;
}
