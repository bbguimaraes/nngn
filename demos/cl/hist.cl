static void hist_write_global(__global uint *dst, uchar4 v) {
    atomic_inc(dst + 0 * 256 + v.x);
    atomic_inc(dst + 1 * 256 + v.y);
    atomic_inc(dst + 2 * 256 + v.z);
    atomic_inc(dst + 3 * 256 + v.w);
}

static void hist_write_local(__local uint *dst, uchar4 v) {
    atomic_inc(dst + 0 * 256 + v.x);
    atomic_inc(dst + 1 * 256 + v.y);
    atomic_inc(dst + 2 * 256 + v.z);
    atomic_inc(dst + 3 * 256 + v.w);
}

__kernel void hist0(
    __read_only image2d_t src, sampler_t sampler, __global uint *dst
) {
    const int2 i = {get_global_id(0), get_global_id(1)};
    const uint4 c = read_imageui(src, sampler, i);
    hist_write_global(dst, convert_uchar4(c));
}

__kernel void hist1(
    uint w, __global const uchar4 *src, __global uint *dst
) {
    hist_write_global(dst, src[w * get_global_id(0) + get_global_id(1)]);
}

__kernel void hist2(uint n, __global const uchar4 *src, __global uint *dst) {
    const uint ls = get_local_size(0);
    const uint gs = n / get_num_groups(0);
    const uint gid = get_group_id(0), lid = get_local_id(0);
    for(uint i = gs * gid + lid, e = i + gs; i < e; i += ls)
        hist_write_global(dst, src[i]);
}

__kernel void hist3(uint n, __global const uchar4 *src, __global uint *dst) {
    const uint ls = get_local_size(0);
    const uint gs = n / get_num_groups(0), ni = gs / ls;
    const uint gid = get_group_id(0), lid = get_local_id(0);
    for(uint i = gs * gid + ni * lid, e = i + ni; i < e; ++i)
        hist_write_global(dst, src[i]);
}

__kernel void hist4(
    uint n, __local uint *l,
    __global const uchar4 *src, __global uint *dst
) {
    const uint ls = get_local_size(0);
    const uint gs = n / get_num_groups(0);
    const uint gid = get_group_id(0), lid = get_local_id(0);
    for(uint i = lid; i < 1024; i += ls)
        l[i] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    for(uint i = gs * gid + lid, e = i + gs; i < e; i += ls)
        hist_write_local(l, src[i]);
    barrier(CLK_LOCAL_MEM_FENCE);
    for(uint i = lid; i < 1024; i += ls)
        atomic_add(dst + i, l[i]);
}

#define NBANKS 16
__kernel void hist5(
        uint n, __local uint *l,
        __global const uchar4 *src, __global uint *dst) {
    const uint ls = get_local_size(0);
    const uint gs = n / get_num_groups(0);
    const uint gid = get_group_id(0), lid = get_local_id(0);
    for(uint i = lid; i < NBANKS * 1024; i += ls)
        l[i] = 0;
    barrier(CLK_LOCAL_MEM_FENCE);
    __local uint *l_off = l + 1024 * (lid % NBANKS);
    for(uint i = gs * gid + lid, e = i + gs; i < e; i += ls)
        hist_write_local(l_off, src[i]);
    barrier(CLK_LOCAL_MEM_FENCE);
    for(uint i = lid; i < 1024; i += ls) {
        uint sum = 0;
        for(uint b = 0; b < NBANKS; ++b)
            sum += l[1024 * b + i];
        atomic_add(dst + i, sum);
    }
}

__kernel void hist_to_tex(
        uint ch, uint max, uchar mask,
        __global uint *src, __write_only image2d_t dst) {
    const uint4 c =
        255 * (uint4)((mask & 4) >> 2, (mask & 2) >> 1, (mask & 1), 1);
    const uint id = get_global_id(0);
    const uint hs = get_global_size(0);
    const uint s = 2 * hs, col = ch % 2, row = ch / 2;
    int2 b = {hs * col + id, hs * (2 - row)};
    const uint v = hs * src[256 * ch + id] / max;
    for(uint e = b[1] - v; b[1] > e; --b[1])
        write_imageui(dst, b, c);
}
