__kernel void rotate_img(
        uint w, uint h, float cos_theta, float sin_theta,
        __global const uchar *src, __global uchar *dst) {
    const uint x0 = w / 2, y0 = h / 2;
    const uint id = get_global_id(0);
    const uint div_4 = id / 4, mod_4 = id % 4;
    const int y = div_4 / w - y0, x = div_4 % w - x0;
    const int rx =  cos_theta * x + sin_theta * y + x0;
    const int ry = -sin_theta * x + cos_theta * y + y0;
    if(0 <= rx && rx < w && 0 <= ry && ry < h)
        dst[id] = src[4 * (ry * w + rx) + mod_4];
    else
        dst[id] = 0;
}

__kernel void brighten(
        float f, sampler_t sampler,
        __read_only image2d_t src, __write_only image2d_t dst) {
    const int2 i = {get_global_id(0), get_global_id(1)};
    const uint4 v = read_imageui(src, i);
    write_imageui(dst, i, convert_uint4(f * convert_float4(v)));
}

__kernel void convolution0(
        uint w, uint h,
        __constant float *filter, uint radius,
        __read_only image2d_t src, sampler_t sampler,
        __write_only image2d_t dst) {
    const int x = get_global_id(0), y = get_global_id(1);
    const int r = radius;
    int fi = 0;
    float4 v = {};
    for(int fy = y - r, ey = y + r; fy <= ey; ++fy)
        for(int fx = x - r, ex = x + r; fx <= ex; ++fx)
            v += (float4)filter[fi++]
                * convert_float4(read_imageui(src, sampler, (int2){fx, fy}));
    write_imageui(dst, (int2){x, y}, convert_uint4(v));
}

__kernel void convolution1(
        uint w, uint h,
        __constant float *filter, uint radius,
        __read_only image2d_t src, sampler_t sampler,
        __local float4 *l, __write_only image2d_t dst) {
    const uint lsx = get_local_size(0), lsy = get_local_size(1);
    const uint csx = lsx + 2 * radius, csy = lsy + 2 * radius;
    const uint lx = get_local_id(0), ly = get_local_id(1);
    const int wx = get_group_id(0) * (int)lsx - (int)radius;
    const int wy = get_group_id(1) * (int)lsy - (int)radius;
    for(int y = ly, ye = csy; y < ye; y += lsy)
        for(int x = lx, xe = csx; x < xe; x += lsx)
            l[y * csx + x] = convert_float4(
                read_imageui(src, sampler, (int2){wx + x, wy + y}));
    barrier(CLK_LOCAL_MEM_FENCE);
    float4 v = {};
    for(int fy = 0, e = 2 * radius + 1; fy < e; ++fy)
        for(int fx = 0; fx < e; ++fx)
            v += (float4)(*filter++) * l[(ly + fy) * csx + lx + fx];
    write_imageui(
        dst, (int2){get_global_id(0), get_global_id(1)}, convert_uint4(v));
}

__kernel void convolution2(
        uint w, uint h,
        __constant float *filter, uint radius,
        __global uchar4 *src,
        __local float4 *l, __global uchar4 *dst) {
    const uint lsx = get_local_size(0), lsy = get_local_size(1);
    const uint csx = lsx + 2 * radius, csy = lsy + 2 * radius;
    const uint lx = get_local_id(0), ly = get_local_id(1);
    const int wx = get_group_id(0) * (int)lsx - (int)radius;
    const int wy = get_group_id(1) * (int)lsy - (int)radius;
    for(int y = ly, ye = csy; y < ye; y += lsy)
        for(int x = lx, xe = csx; x < xe; x += lsx)
            l[y * csx + x] = convert_float4(
                src[w * clamp(wy + y, 0, (int)h) + clamp(wx + x, 0, (int)w)]);
    barrier(CLK_LOCAL_MEM_FENCE);
    float4 v = {};
    for(int fy = 0, e = 2 * radius + 1; fy < e; ++fy)
        for(int fx = 0; fx < e; ++fx)
            v += (float4)(*filter++) * l[(ly + fy) * csx + lx + fx];
    dst[w * get_global_id(1) + get_global_id(0)] = convert_uchar4(v);
}
