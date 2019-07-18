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

static float4 collision_grid(
    uint max_idx, float radius, float elast, float dt,
    __global const uint *grid, __global const uint *grid_count,
    __global const float4 *pos, __global const float4 *vel,
    float4 pos0, float4 vel0, uint grid_idx
) {
    float4 ret = {};
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
    const uint id = get_global_id(0);
    pos[id] += dt * (vel[id] += dt * forces[id]);
    forces[id] = 0;
}
