__constant sampler_t sampler =
    CLK_NORMALIZED_COORDS_FALSE
    | CLK_FILTER_LINEAR
    | CLK_ADDRESS_CLAMP_TO_EDGE;

__kernel void convolution0(
    __constant float *filter, uint radius,
    __read_only image2d_t src,
    __write_only image2d_t dst
) {
    const int x = get_global_id(0), y = get_global_id(1);
    const int r = radius;
    int fi = 0;
    float4 v = {};
    for(int fy = y - r, ey = y + r; fy <= ey; ++fy)
        for(int fx = x - r, ex = x + r; fx <= ex; ++fx) {
            const uint4 p = read_imageui(src, sampler, (int2){fx, fy});
            v += (float4)filter[fi++] * convert_float4(p);
        }
    write_imageui(dst, (int2){x, y}, convert_uint4(v));
}

__kernel void convolution1(
    uint w, uint h,
    __constant float *filter, uint radius,
    __read_only image2d_t src,
    __local uchar4 *l, __write_only image2d_t dst
) {
    const uint lsx = get_local_size(0), lsy = get_local_size(1);
    const uint csx = lsx + 2 * radius, csy = lsy + 2 * radius;
    const uint lx = get_local_id(0), ly = get_local_id(1);
    const int wx = get_group_id(0) * (int)lsx - (int)radius;
    const int wy = get_group_id(1) * (int)lsy - (int)radius;
    for(int y = ly, ye = csy; y < ye; y += lsy)
        for(int x = lx, xe = csx; x < xe; x += lsx)
            l[y * csx + x] = convert_uchar4(
                read_imageui(src, sampler, (int2){wx + x, wy + y}));
    barrier(CLK_LOCAL_MEM_FENCE);
    float4 v = {};
    for(int fy = 0, e = 2 * radius + 1; fy < e; ++fy)
        for(int fx = 0; fx < e; ++fx)
            v += (float4)*filter++
                * convert_float4(l[(ly + fy) * csx + lx + fx]);
    write_imageui(
        dst, (int2){get_global_id(0), get_global_id(1)}, convert_uint4(v));
}

__kernel void convolution2(
    uint w, uint h,
    __constant float *filter, uint radius,
    __global uchar4 *src,
    __local uchar4 *l, __global uchar4 *dst
) {
    const uint lsx = get_local_size(0), lsy = get_local_size(1);
    const uint csx = lsx + 2 * radius, csy = lsy + 2 * radius;
    const uint lx = get_local_id(0), ly = get_local_id(1);
    const int wx = get_group_id(0) * (int)lsx - (int)radius;
    const int wy = get_group_id(1) * (int)lsy - (int)radius;
    for(int y = ly, ye = csy; y < ye; y += lsy)
        for(int x = lx, xe = csx; x < xe; x += lsx)
            l[y * csx + x] = convert_uchar4(
                src[w * clamp(wy + y, 0, (int)h) + clamp(wx + x, 0, (int)w)]);
    barrier(CLK_LOCAL_MEM_FENCE);
    float4 v = {};
    for(int fy = 0, e = 2 * radius + 1; fy < e; ++fy)
        for(int fx = 0; fx < e; ++fx)
            v += (float4)*filter++
                * convert_float4(l[(ly + fy) * csx + lx + fx]);
    dst[w * get_global_id(1) + get_global_id(0)] = convert_uchar4(v);
}

__kernel void blur(
    uint x_radius, uint y_radius, __constant float *filter,
    __read_only image2d_t src, __write_only image2d_t dst
) {
    const uint x = get_global_id(0), y = get_global_id(1);
    const uint radius = x_radius + y_radius;
    float4 v = {};
    for(int fy = y - y_radius, ey = y + y_radius; fy <= ey; ++fy)
        for(int fx = x - x_radius, ex = x + x_radius; fx <= ex; ++fx) {
            const uint4 p = read_imageui(src, sampler, (int2){fx, fy});
            v += (float4)*filter++ * convert_float4(p);
        }
    write_imageui(dst, (int2){x, y}, convert_uint4(v));
}
