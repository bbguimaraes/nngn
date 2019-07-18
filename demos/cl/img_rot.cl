__constant sampler_t sampler =
    CLK_NORMALIZED_COORDS_FALSE
    | CLK_FILTER_LINEAR
    | CLK_ADDRESS_CLAMP;

static inline int2 rotate_common(
    uint w, uint h, uint x, uint y, float cos, float sin
) {
    const int x0 = w / 2, y0 = h / 2;
    const int x1 = (int)x - x0, y1 = (int)y - y0;
    return (int2){
        x0 + cos * x1 - sin * y1,
        y0 + sin * x1 + cos * y1,
    };
}

__kernel void rotate0(
    uint w, uint h, float cos, float sin,
    __global const uchar *src, __global uchar *dst
) {
    const uint id = get_global_id(0);
    const int2 r = rotate_common(w, h, id / 4 % w, id / 4 / w, cos, sin);
    if(0 <= r.x && r.x < w && 0 <= r.y && r.y < h)
        dst[id] = src[4 * (w * r.y + r.x) + id % 4];
    else
        dst[id] = 0;
}

__kernel void rotate1(
    uint w, uint h, float cos, float sin,
    __global const uchar4 *src, __global uchar4 *dst
) {
    const uint id = get_global_id(0);
    const int2 r = rotate_common(w, h, id % w, id / w, cos, sin);
    if(0 <= r.x && r.x < w && 0 <= r.y && r.y < h)
        dst[id] = src[r.y * w + r.x];
    else
        dst[id] = 0;
}

__kernel void rotate2(
    uint w, uint h, float cos, float sin,
    __read_only image2d_t src, __write_only image2d_t dst
) {
    const int2 i = {get_global_id(0), get_global_id(1)};
    const int2 r = rotate_common(w, h, i.x, i.y, cos, sin);
    write_imageui(dst, i, read_imageui(src, sampler, (int2)(r.x, r.y)));
}
