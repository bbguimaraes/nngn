typedef struct {
    uint w, h, max_depth, n_spheres;
    struct { float3 pos, eye, up; } camera;
} tracer_conf;

typedef struct { uint4 state; } rnd_gen;

typedef struct {
    float3 origin, lower_left_corner, horizontal, vertical;
} camera;

typedef struct { float3 pos, dir; } ray;

typedef struct {
    float3 pos, normal;
    float t;
    bool front_face;
} hit_record;

typedef struct { float4 c_r; } sphere;

// https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-37-efficient-random-number-generation-and-application
// https://math.stackexchange.com/questions/337782/pseudo-random-number-generation-on-the-gpu
uint taus_step(uint z, int s1, int s2, int s3, uint m) {
    uint b = ((z << s1) ^ z) >> s2;
    return ((z & m) << s3) ^ b;
}

uint lcg_step(uint z, uint a, uint c) { return a * z + c; }

float rnd_gen_next(rnd_gen *b) {
    uint4 state = b->state;
    state.x = taus_step(state.x, 13, 19, 12, 4294967294);
    state.y = taus_step(state.y, 2, 25, 4, 4294967288);
    state.z = taus_step(state.z, 3, 11, 17, 4294967280);
    state.w = lcg_step(state.w, 1664525, 1013904223);
    b->state = state;
    return 2.3283064365387e-10f * (state.x ^ state.y ^ state.z ^ state.w);
}

camera camera_init(
    float3 pos, float3 eye, float3 up,
    float vfov, float aspect
) {
    const float h = tan(vfov / 2.0f);
    const float viewport_height = 2.0f * h;
    const float viewport_width = aspect * viewport_height;
    const float3 w = normalize(eye - pos);
    const float3 u = normalize(cross(up, w));
    const float3 v = cross(w, u);
    const float3 horizontal = viewport_width * u;
    const float3 vertical = viewport_height * v;
    return (camera){
        .origin = eye,
        .horizontal = horizontal,
        .vertical = vertical,
        .lower_left_corner = eye - horizontal / 2.0f - vertical / 2.0f - w};
}

ray camera_ray(const camera *c, float s, float t) {
    float3 dir = c->lower_left_corner;
    dir += s * c->horizontal;
    dir += t * c->vertical;
    dir -= c->origin;
    return (ray){.pos = c->origin, .dir = dir};
}

float3 ray_at(const ray *r, float t) { return r->pos + t * r->dir; }

float3 sphere_center(__global const sphere *s) { return s->c_r.xyz; }
float sphere_radius(__global const sphere *s) { return s->c_r[3]; }

void sphere_hit_record(
    __global const sphere *s, float t, const ray *r, hit_record *rec
) {
    const float3 c = sphere_center(s);
    const float3 p = ray_at(r, t);
    const float3 n = (p - c) / sphere_radius(s);
    const bool front_face = dot(r->dir, n) < 0.0f;
    rec->t = t;
    rec->pos = p;
    rec->normal = front_face ? n : -n;
    rec->front_face = front_face;
}

bool sphere_hit(
    __global const sphere *s, float t_max, const ray *r,
    hit_record *rec
) {
    const float3 center = sphere_center(s);
    const float radius = sphere_radius(s);
    const float3 dir = r->pos - center;
    const float a = dot(r->dir, r->dir);
    const float half_b = dot(dir, r->dir);
    const float c = dot(dir, dir) - radius * radius;
    const float disc = half_b * half_b - a * c;
    if(disc <= 0)
        return false;
    const float root = sqrt(disc);
    const float neg_t = (-half_b - root) / a;
    if(T_MIN < neg_t && neg_t < t_max)
        return sphere_hit_record(s, neg_t, r, rec), true;
    const float pos_t = (-half_b + root) / a;
    if(T_MIN < pos_t && pos_t < t_max)
        return sphere_hit_record(s, pos_t, r, rec), true;
    return false;
}

bool world_hit(
    float t_max, uint n_spheres, __global const sphere *spheres,
    const ray *r, hit_record *rec
) {
    __global const sphere *const e = spheres + n_spheres;
    hit_record tmp_rec = {};
    bool hit_anything = false;
    for(; spheres < e; ++spheres)
        if(sphere_hit(spheres, t_max, r, &tmp_rec)) {
            hit_anything = true;
            t_max = tmp_rec.t;
            *rec = tmp_rec;
        }
    return hit_anything;
}

float3 color(uint n_spheres, __global const sphere *spheres, ray r) {
    hit_record rec = {};
    if(world_hit(T_MAX, n_spheres, spheres, &r, &rec))
        return 0.5f * (rec.normal + 1.0f);
    const float3 unit_direction = normalize(r.dir);
    const float t = 0.5f * (unit_direction.y + 1.0f);
    return (1 - t) * (float3){1, 1, 1} + t * (float3){0.5f, 0.7f, 1.0f};
}

void merge_pixel(uint i, float3 c, __global float3 *tex) {
    c += *tex * (float)i;
    c /= (float)i + 1.0f;
    *tex = c;
}

float3 trace_pixel(
    tracer_conf *conf, const camera *c, rnd_gen *rnd,
    __global const sphere *spheres,
    uint x, uint y
) {
    const float u = ((float)x + rnd_gen_next(rnd)) / (float)(conf->w - 1);
    const float v = ((float)y + rnd_gen_next(rnd)) / (float)(conf->h - 1);
    return color(conf->n_spheres, spheres, camera_ray(c, u, v));
}

__kernel void trace(
    tracer_conf conf, uint i_samples,
    __global const rnd_gen *rnd_p,
    __global const sphere *spheres,
    __global float3 *tex
) {
    const camera c = camera_init(
        conf.camera.pos, conf.camera.eye, conf.camera.up,
        PI / 2.0f, (float)conf.w / (float)conf.h);
    const uint id0 = get_global_id(0), id1 = get_global_id(1);
    const uint id = get_global_size(0) * id1 + id0;
    rnd_gen rnd = rnd_p[id];
    for(uint y = BLOCK_SIZE * id0, ye = y + BLOCK_SIZE; y < ye; ++y)
        for(uint x = BLOCK_SIZE * id1, xe = x + BLOCK_SIZE; x < xe; ++x) {
            const float3 pixel = trace_pixel(&conf, &c, &rnd, spheres, x, y);
            merge_pixel(i_samples, pixel, tex + conf.w * (conf.w - y - 1) + x);
        }
}

__kernel void write_tex(
    uint w, __global const float3 *in, __global uchar4 *out
) {
    const uint y = get_global_id(0);
    in += w * y;
    out += w * y;
    const float m = 256.0f - FLT_EPSILON;
    for(__global const float3 *const e = in + w; in != e; ++in) {
        const float3 c = *in * m;
        *out++ = (uchar4){c[0], c[1], c[2], 255};
    }
}
