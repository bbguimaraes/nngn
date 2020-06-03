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

__kernel void old_collision(
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
    const uint id = get_global_id(0);
    center_x[id] += dt * (vel_x[id] += dt * force_x[id]);
    center_y[id] += dt * (vel_y[id] += dt * force_y[id]);
    force_x[id] = 0;
    force_y[id] = 0;
}
