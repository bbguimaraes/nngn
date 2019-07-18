struct __attribute__((packed)) sphere_params {
    float size;
    uint n_cells;
    uint max_idx;
    float g;
};

static uint grid_idx(struct sphere_params *params, float4 pos) {
    const float s = params->size;
    float2 gpos = pos.xy;
    if(pos.x < 0)
        pos.x = s - pos.x;
    if(pos.y < 0)
        pos.y = s - pos.y;
    gpos = fmod(fabs(gpos) / s, params->n_cells);
    return params->n_cells * gpos.y + gpos.x;
}

__kernel void grid(
        struct sphere_params params,
        __global const float4 *pos,
        __global uint *grid, __global uint *grid_count) {
    const uint id = get_global_id(0);
    const uint gi = grid_idx(&params, pos[id]);
    const uint i = atomic_inc(grid_count + gi);
    if(i < params.max_idx)
        grid[params.max_idx * gi + i] = id;
    else
        printf("bucket %d full\n", gi);
}

__kernel void collision(
        struct sphere_params params, float radius, float dt,
        __global const uint *grid, __global const uint *grid_count,
        __global const float4 *pos, __global const float4 *vel,
        __global float4 *forces) {
    const uint id = get_global_id(0);
    const float dt2 = dt * dt;
    const float elast = 0.15f;
    const float4 ipos = pos[id], ivel = vel[id];
    const uint gi = grid_idx(&params, ipos);
    const uint4 n_cells = params.n_cells;
    const uint max_idx = params.max_idx;
    float4 f = forces[id];
    for(int gny = -1; gny <= 1; ++gny)
        for(int gnx = -1; gnx <= 1; ++gnx) {
            const int gn = n_cells[0] * gny + gnx + gi;
            if(gn < 0 || n_cells[0] * n_cells[1] <= gn)
                continue;
            uint i = gn * max_idx, e = i + min(max_idx, grid_count[gn]);
            for(; i < e; ++i) {
                const uint oid = grid[i];
                if(id == oid)
                    continue;
                const float4 opos = pos[oid];
                float4 d = ipos - opos;
                const float l2 = dot(d, d);
                if(l2 > 4 * radius * radius || !l2)
                    continue;
                const float l = sqrt(l2);
                const float4 ovel = vel[oid];
                f += (2 * radius - l) / l * d / dt2 * 0.5f;
                f += (ovel - ivel) / dt * elast;
            }
        }
    float d = ipos.x - radius - -params.size;
    if(d < 0)
        f.x += (-d / dt2 - ivel.x / dt * elast);
    d = ipos.x + radius - params.size;
    if(d > 0)
        f.x += (-d / dt2 - ivel.x / dt * elast);
    d = ipos.y - radius - -params.size;
    if(d < 0)
        f.y += (-d / dt2 - ivel.y / dt * elast);
    d = ipos.y + radius - params.size;
    if(d > 0)
        f.y += (-d / dt2 - ivel.y / dt * elast);
    f.y -= params.g;
    forces[id] = f;
}

__kernel void integrate(
        float dt,
        __global float4 *forces, __global float4 *vel, __global float4 *pos) {
    const uint id = get_global_id(0);
    pos[id] += dt * (vel[id] += dt * forces[id]);
    forces[id] = 0;
}
