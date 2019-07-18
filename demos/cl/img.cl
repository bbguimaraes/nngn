__kernel void brighten(
    float f, __read_only image2d_t src, __write_only image2d_t dst
) {
    const int2 i = {get_global_id(0), get_global_id(1)};
    const uint4 v = read_imageui(src, i);
    write_imageui(dst, i, convert_uint4(f * convert_float4(v)));
}
