struct rnd_gen { uint4 state; };
struct ray { float3 o, d; };
struct lambertian { float3 albedo; };
struct metal { float4 albedo_fuzz; };
struct dielectric { float n1_n0; };
struct aabb { float3 bl, tr; };
struct bvh { struct aabb aabb; };
struct sphere { float4 c_r; uint material; };

struct hit {
    float3 p, n;
    float t;
    uint material;
    bool front_face;
};

struct camera {
    float3 o, bl, hor, ver, u, v, w;
    float lens_radius;
};

struct tracer_conf {
    uint w, h, max_depth, n_spheres;
    float t_min, t_max;
    struct {
        float3 pos, eye, up;
        uint2 screen;
        float fov_y, lens_radius, focus_dist;
    } camera_input;
    struct camera camera;
};

#if __OPENCL_VERSION__ < 200
uint ctz(uint n) { return popcount(n ^ (n + 1)) - 1; }
#endif

float3 vec_reflect(float3 v, float3 n) { return v - 2 * n * dot(v, n); }

float3 vec_refract(float3 v, float3 n, float n1_n0) {
    v = normalize(v);
    const float d = dot(v, n);
    const float disc = 1 - n1_n0 * n1_n0 * (1 - d * d);
    return disc <= 0 ? (float3)0 : n1_n0 * (v - (n * d)) - n * sqrt(disc);
}

float schlick(float cos, float n1_n0) {
    float r0 = (1.0f - n1_n0) / (1.0f + n1_n0);
    r0 *= r0;
    return r0 + (1.0f - r0) * pow(1.0f - cos, 5);
}

float3 sky(struct ray r) {
    return mix(SKY_TOP, SKY_BOTTOM, 0.5f * (normalize(r.d).y + 1.0f));
}

// https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-37-efficient-random-number-generation-and-application
// https://math.stackexchange.com/questions/337782/pseudo-random-number-generation-on-the-gpu
uint taus_step(uint z, int s1, int s2, int s3, uint m) {
    uint b = ((z << s1) ^ z) >> s2;
    return ((z & m) << s3) ^ b;
}

uint lcg_step(uint z, uint a, uint c) { return a * z + c; }

float rnd_gen_next(struct rnd_gen *rnd) {
    uint4 state = rnd->state;
    state.x = taus_step(state.x, 13, 19, 12, 4294967294);
    state.y = taus_step(state.y, 2, 25, 4, 4294967288);
    state.z = taus_step(state.z, 3, 11, 17, 4294967280);
    state.w = lcg_step(state.w, 1664525, 1013904223);
    rnd->state = state;
    return 2.3283064365387e-10f * (state.x ^ state.y ^ state.z ^ state.w);
}

float3 rnd_in_unit_sphere(struct rnd_gen *rnd) {
    return normalize((float3){
        rnd_gen_next(rnd) * 2.0f - 1.0f,
        rnd_gen_next(rnd) * 2.0f - 1.0f,
        rnd_gen_next(rnd) * 2.0f - 1.0f,
    });
}

float3 rnd_in_unit_disk(struct rnd_gen *rnd) {
    for(;;) {
        const float3 p = {
            rnd_gen_next(rnd) * 2.0f - 1.0f,
            rnd_gen_next(rnd) * 2.0f - 1.0f,
            0,
        };
        if(dot(p, p) < 1)
            return p;
    }
}

float3 rnd_in_hemisphere(float3 n, struct rnd_gen *rnd) {
    const float3 v = rnd_in_unit_sphere(rnd);
    return dot(v, n) > 0.0f ? v : -v;
}

float3 ray_at(struct ray r, float t) { return r.o + t * r.d; }

bool lambertian_apply(
    global const struct lambertian *l, const struct hit *hit,
    struct rnd_gen *rnd, struct ray *r, float3 *attenuation
) {
    *r = (struct ray){.o = hit->p, .d = rnd_in_hemisphere(hit->n, rnd)};
    *attenuation *= l->albedo;
    return true;
}

float3 metal_albedo(global const struct metal *m) { return m->albedo_fuzz.xyz; }
float3 metal_fuzz(global const struct metal *m) { return m->albedo_fuzz[3]; }

bool metal_apply(
    global const struct metal *m, const struct hit *hit,
    struct rnd_gen *rnd, struct ray *r, float3 *attenuation
) {
    const float3 refl =
        vec_reflect(normalize(r->d), hit->n)
        + metal_fuzz(m) * rnd_in_unit_sphere(rnd);
    if(dot(refl, hit->n) < 0)
        return false;
    *r = (struct ray){.o = hit->p, .d = refl};
    *attenuation *= metal_albedo(m);
    return true;
}

bool dielectric_apply(
    global const struct dielectric *d, const struct hit *hit,
    struct rnd_gen *rnd, struct ray *r, float3 *attenuation
) {
    const float3 n = hit->n;
    const float3 dir = normalize(r->d);
    const float n1_n0 = hit->front_face ? 1.0f / d->n1_n0 : d->n1_n0;
    const float cos = fmin(dot(-dir, n), 1.0f);
    const float sin = sqrt(1.0f - cos * cos);
    if(n1_n0 * sin > 1.0f || rnd_gen_next(rnd) < schlick(cos, n1_n0))
        *r = (struct ray){.o = hit->p, .d = vec_reflect(dir, n)};
    else
        *r = (struct ray){.o = hit->p, .d = vec_refract(dir, n, n1_n0)};
    return true;
}

bool material_apply(
    global const struct lambertian *lambertians,
    global const struct metal *metals,
    global const struct dielectric *dielectrics,
    const struct hit *hit,
    struct rnd_gen *rnd, struct ray *r, float3 *attenuation
) {
    const uint material = hit->material;
    const int shift = 32 - MATERIAL_TYPE_BITS;
    const uint type = material >> shift;
    const uint i = material & ~(UINT_MAX << shift);
    switch(type) {
    case MATERIAL_TYPE_LAMBERTIAN:
        return lambertian_apply(lambertians + i, hit, rnd, r, attenuation);
    case MATERIAL_TYPE_METAL:
        return metal_apply(metals + i, hit, rnd, r, attenuation);
    case MATERIAL_TYPE_DIELECTRIC:
        return dielectric_apply(dielectrics + i, hit, rnd, r, attenuation);
    }
    return false;
}

uint bvh_level(uint i) { return 32 - clz(i) - 1; }
uint bvh_levels(uint i) { return bvh_level(i - 1) + 1; }
uint bvh_level_base(uint i) { return 1 << i; }
uint bvh_next_right(uint i) { i >>= ctz(~i); return i + !!i; }

uint bvh_tree_size(uint n_levels, uint level) {
    return bvh_level_base(n_levels - level) - 1;
}

uint bvh_next_down(uint n_levels, uint i) {
    return select(i << 1, bvh_next_right(i), bvh_level(i) == n_levels - 1);
}

bool bvh_hit(
    global const struct bvh *bvh, float t_min, float t_max, struct ray r
) {
    for(uint a = 0; a < 3; a++) {
        float inv_d = 1.0f / r.d[a];
        float t0 = (bvh->aabb.bl[a] - r.o[a]) * inv_d;
        float t1 = (bvh->aabb.tr[a] - r.o[a]) * inv_d;
        if(inv_d < 0.0f) {
            const float tmp = t0;
            t0 = t1, t1 = tmp;
        }
        t_min = t0 > t_min ? t0 : t_min;
        t_max = t1 < t_max ? t1 : t_max;
        if(t_max <= t_min)
            return false;
    }
    return true;
}

float3 sphere_center(global const struct sphere *s) { return s->c_r.xyz; }
float sphere_radius(global const struct sphere *s) { return s->c_r[3]; }

struct aabb sphere_aabb(global const struct sphere *s) {
    return (struct aabb) {
        .bl = sphere_center(s) - sphere_radius(s),
        .tr = sphere_center(s) + sphere_radius(s),
    };
}

struct aabb sphere_aabb_join(
    global const struct sphere *b, global const struct sphere *e
) {
    struct aabb ret = sphere_aabb(b);
    for(b++; b < e; ++b) {
        const struct aabb bi = sphere_aabb(b);
        ret.bl = (float3){
            min(ret.bl.x, bi.bl.x),
            min(ret.bl.y, bi.bl.y),
            min(ret.bl.z, bi.bl.z),
        };
        ret.tr = (float3){
            max(ret.tr.x, bi.tr.x),
            max(ret.tr.y, bi.tr.y),
            max(ret.tr.z, bi.tr.z),
        };
    }
    return ret;
}

void sphere_sort(
    global struct sphere *b, global struct sphere *e, uint axis
) {
    for(; b != e; ++b) {
        global struct sphere *s = b;
        float min_bl = sphere_aabb(s).bl[axis];
        for(global struct sphere *i = b + 1; i < e; ++i) {
            const float bl = sphere_aabb(i).bl[axis];
            if(bl < min_bl)
                s = i, min_bl = bl;
        }
        struct sphere tmp = *b;
        *b = *s;
        *s = tmp;
    }
}

struct hit sphere_gen_hit(
    global const struct sphere *s, float t, struct ray r
) {
    const float3 c = sphere_center(s);
    const float3 p = ray_at(r, t);
    const float3 n = (p - c) / sphere_radius(s);
    const float proj = dot(r.d, n);
    return (struct hit){
        .p = p,
        .n = -sign(proj) * n,
        .t = t,
        .material = s->material,
        .front_face = signbit(proj),
    };
}

bool sphere_hit(
    global const struct sphere *s, float t_min, float t_max, struct ray r,
    struct hit *hit
) {
    const float3 center = sphere_center(s);
    const float radius = sphere_radius(s);
    const float3 dir = r.o - center;
    const float a = dot(r.d, r.d);
    const float half_b = dot(dir, r.d);
    const float c = dot(dir, dir) - radius * radius;
    const float disc = half_b * half_b - a * c;
    if(disc <= 0)
        return false;
    const float root = sqrt(disc);
    const float neg_t = (-half_b - root) / a;
    if(t_min < neg_t && neg_t < t_max) {
        *hit = sphere_gen_hit(s, neg_t, r);
        return true;
    }
    const float pos_t = (-half_b + root) / a;
    if(t_min < pos_t && pos_t < t_max) {
        *hit = sphere_gen_hit(s, pos_t, r);
        return true;
    }
    return false;
}

struct camera camera_init(global const struct tracer_conf *conf) {
    const float3 eye = conf->camera_input.eye;
    const float3 pos = conf->camera_input.pos;
    const float3 up = conf->camera_input.up;
    const uint2 screen = conf->camera_input.screen;
    const float focus_dist = conf->camera_input.focus_dist;
    const float aspect = (float)screen.x / (float)screen.y;
    const float half_h = tan(conf->camera_input.fov_y / 2.0f);
    const float half_w = aspect * half_h;
    const float3 w = normalize(eye - pos);
    const float3 u = normalize(cross(up, w));
    const float3 v = cross(w, u);
    const float3 hor = u * (2 * half_w * focus_dist);
    const float3 ver = v * (2 * half_h * focus_dist);
    return (struct camera){
        .o = eye,
        .hor = hor,
        .ver = ver,
        .bl = eye - hor / 2 - ver / 2 - w * focus_dist,
        .u = u,
        .v = v,
        .w = w,
        .lens_radius = conf->camera_input.lens_radius,
    };
}

struct ray camera_ray(
    global const struct camera *c, struct rnd_gen *rnd, float s, float t
) {
    const float3 rd = c->lens_radius * rnd_in_unit_disk(rnd);
    const float3 offset = c->u * rd.x + c->v * rd.y;
    const float3 o = c->o + offset;
    const float3 dir = c->bl + s * c->hor + t * c->ver;
    return (struct ray){.o = o, .d = dir - o};
}

bool world_hit_linear(
    float t_min, float t_max,
    uint n_spheres, global const struct sphere *spheres,
    struct ray r, struct hit *hit
) {
    global const struct sphere *const e = spheres + n_spheres;
    bool ret = false;
    for(; spheres != e; ++spheres) {
        if(!sphere_hit(spheres, t_min, t_max, r, hit))
            continue;
        ret = true;
        t_max = hit->t;
    }
    return ret;
}

bool world_hit_bvh(
    float t_min, float t_max,
    global const struct bvh *bvh,
    uint n_spheres, global const struct sphere *spheres,
    struct ray r, struct hit *hit
) {
    const uint n_levels = bvh_levels(n_spheres);
    bool ret = false;
    for(uint i = 1; i;) {
        const uint level = bvh_level(i);
        if(!bvh_hit(bvh + i, t_min, t_max, r)) {
            i = bvh_next_right(i);
            continue;
        }
        if(level == n_levels - 1) {
            global const struct sphere *s =
                spheres + 2 * (i - bvh_level_base(level));
            if(sphere_hit(s, t_min, t_max, r, hit)) {
                ret = true;
                t_max = hit->t;
            }
            if(sphere_hit(s + 1, t_min, t_max, r, hit)) {
                ret = true;
                t_max = hit->t;
            }
        }
        i = bvh_next_down(n_levels, i);
    }
    return ret;
}

bool world_hit(
    float t_min, float t_max,
    global const struct bvh *bvh,
    uint n_spheres, global const struct sphere *spheres,
    struct ray r, struct hit *hit
) {
#if BVH_ENABLED
    return world_hit_bvh(t_min, t_max, bvh, n_spheres, spheres, r, hit);
#else
    return world_hit_linear(t_min, t_max, n_spheres, spheres, r, hit);
#endif
}

float3 color(
    uint max_depth, float t_min, float t_max,
    struct rnd_gen *rnd,
    global const struct lambertian *lambertians,
    global const struct metal *metals,
    global const struct dielectric *dielectrics,
    global const struct bvh *bvh,
    uint n_spheres, global const struct sphere *spheres,
    struct ray r
) {
    float3 att = {1, 1, 1};
    struct hit hit = {0};
    while(max_depth--) {
        if(!world_hit(t_min, t_max, bvh, n_spheres, spheres, r, &hit))
            return att * sky(r);
        if(!material_apply(
            lambertians, metals, dielectrics, &hit, rnd, &r, &att)
        )
            break;
    }
    return (float3)0;
}

float3 trace_pixel(
    uint w, uint h, uint max_depth, float t_min, float t_max,
    struct rnd_gen *rnd, global const struct camera *camera,
    global const struct lambertian *lambertians,
    global const struct metal *metals,
    global const struct dielectric *dielectrics,
    global const struct bvh *bvh,
    uint n_spheres, global const struct sphere *spheres,
    uint x, uint y
) {
    const float u = ((float)x + rnd_gen_next(rnd)) / (float)(w - 1);
    const float v = ((float)y + rnd_gen_next(rnd)) / (float)(h - 1);
    return color(
        max_depth, t_min, t_max, rnd,
        lambertians, metals, dielectrics, bvh, n_spheres, spheres,
        camera_ray(camera, rnd, u, v));
}

void merge_pixel(uint i, float3 c, global float3 *tex) {
    c += *tex * (float)i;
    c /= (float)i + 1.0f;
    *tex = c;
}

kernel void camera(global struct tracer_conf *conf) {
    conf->camera = camera_init(conf);
}

kernel void bvh(
    global struct rnd_gen *rnd_p,
    uint n_spheres, global struct sphere *spheres,
    global struct bvh *bvh
) {
    struct rnd_gen rnd = *rnd_p;
    const uint n_levels = bvh_levels(n_spheres);
//    printf("%u %u\n", n_spheres, n_levels);
    uint n_elems = 1 << n_levels;
    uint n_nodes = 1;
    for(uint level = 0; level != n_levels; ++level) {
        for(uint node = 0; node != n_nodes; ++node) {
            global struct sphere
                *const b = spheres + min(n_spheres, n_elems * node),
                *const e = spheres + min(n_spheres, n_elems * (node + 1));
            const uint axis = fmod(rnd_gen_next(&rnd) * 3, 3);
            sphere_sort(b, e, axis);
            *++bvh = (struct bvh){.aabb = sphere_aabb_join(b, e)};
        }
        n_elems >>= 1;
        n_nodes <<= 1;
//        print && printf("\n");
    }
//    print && printf("---\n");
//    bvh[1].aabb = (struct aabb) {.bl = {-8, -1, -1}, .tr = { 8, 1, 1}};
//    bvh[2].aabb = (struct aabb) {.bl = {-8, -1, -1}, .tr = { 0, 1, 1}};
//    bvh[3].aabb = (struct aabb) {.bl = { 0, -1, -1}, .tr = { 8, 1, 1}};
//    bvh[4].aabb = (struct aabb) {.bl = {-8, -1, -1}, .tr = {-4, 1, 1}};
//    bvh[5].aabb = (struct aabb) {.bl = {-4, -1, -1}, .tr = { 0, 1, 1}};
//    bvh[6].aabb = (struct aabb) {.bl = { 0, -1, -1}, .tr = { 4, 1, 1}};
//    bvh[7].aabb = (struct aabb) {.bl = { 4, -1, -1}, .tr = { 8, 1, 1}};
//    bvh[0] = (struct bvh) {{.bl = {-8, -1, -1}, .tr = { 8, 1, 1}}, .i = 0};
//    bvh[1] = (struct bvh) {{.bl = {-8, -1, -1}, .tr = { 0, 1, 1}}, .i = 0};
//    bvh[2] = (struct bvh) {{.bl = {-8, -1, -1}, .tr = {-4, 1, 1}}, .i = 0};
//    bvh[3] = (struct bvh) {{.bl = {-4, -1, -1}, .tr = { 0, 1, 1}}, .i = 2};
//    bvh[4] = (struct bvh) {{.bl = { 0, -1, -1}, .tr = { 8, 1, 1}}, .i = 0};
//    bvh[5] = (struct bvh) {{.bl = { 0, -1, -1}, .tr = { 4, 1, 1}}, .i = 4};
//    bvh[6] = (struct bvh) {{.bl = { 4, -1, -1}, .tr = { 8, 1, 1}}, .i = 6};
//    for(uint i = 0; i < bvh_tree_size(n_levels, n_spheres); ++i)
//        printf(
//            "%u: {%2.2f, %2.2f, %2.2f}, {%2.2f, %2.2f, %2.2f} %u\n",
//            i,
//            bvh[i].aabb.bl[0], bvh[i].aabb.bl[1], bvh[i].aabb.bl[2],
//            bvh[i].aabb.tr[0], bvh[i].aabb.tr[1], bvh[i].aabb.tr[2],
//            bvh[i].i);
//    print && printf("\n");
//    for(uint i = 0; i < n_spheres; ++i) {
//        const float3 c = sphere_center(spheres + i);
//        print && printf("{%2.2f, %2.2f, %2.2f}\n", c.x, c.y, c.z);
//    }
//    for(uint i = 1; i;)
//        switch(i) {
//        case 6:
//        case 2:
//            i = bvh_next_right(i); continue;
//        default:
//            printf("%x\n", i);
//            if(bvh_level(i) == n_levels - 1)
//                printf("  %u %u\n", bvh[i].i, bvh[i].i + 1);
//            i = bvh_next_down(n_levels, i);
//            break;
//        }
    *rnd_p = rnd;
}

kernel void trace(
    global struct tracer_conf *conf, uint i_samples,
    global struct rnd_gen *rnd_v,
    global const struct lambertian *lambertians,
    global const struct metal *metals,
    global const struct dielectric *dielectrics,
    global const struct bvh *bvh,
    global const struct sphere *spheres,
    global float3 *tex
) {
    const uint n_samples = 1;
    const uint x = get_global_id(0), y = get_global_id(1);
    const uint i = get_global_size(0) * y + x;
    struct rnd_gen rnd = rnd_v[i];
    float3 c = 0;
    for(uint i_sample = 0; i_sample < n_samples; ++i_sample)
        c += trace_pixel(
            conf->w, conf->h, conf->max_depth, conf->t_min, conf->t_max,
            &rnd, &conf->camera, lambertians, metals, dielectrics, bvh,
            conf->n_spheres, spheres, x, y);
    c /= n_samples;
    merge_pixel(i_samples, c, tex + conf->w * (conf->h - y - 1) + x);
    rnd_v[i] = rnd;
}

kernel void write_tex(uint w, global const float3 *src, global uchar4 *dst) {
    const uint y = get_global_id(0);
    src += w * y;
    dst += w * y;
    for(global const float3 *const e = src + w; src != e; ++src, ++dst)
        *dst = (uchar4){convert_uchar3(255.99f * sqrt(*src)), 255};
}
