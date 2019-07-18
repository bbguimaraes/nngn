void reduce(__local float *in, __global float *out) {
    if(!get_local_id(0))
        return;
    float sum = 0;
    for(uint i = 0, n = get_local_size(0); i < n; ++i)
        sum += in[i];
    out[get_group_id(0)] = sum;
}

__kernel void add_numbers(
        __global float4 *data,
        __local float *local_result,
        __global float *group_result) {
    const uint global_addr = get_global_id(0) * 2;
    const float4 v_sum = data[global_addr] + data[global_addr + 1];
    const uint l_id = get_local_id(0);
    local_result[l_id] = v_sum.s0 + v_sum.s1 + v_sum.s2 + v_sum.s3;
    barrier(CLK_LOCAL_MEM_FENCE);
    if(l_id != 0)
        return;
    float sum = 0.0f;
    for(int i = 0, n = get_local_size(0); i < n; ++i)
        sum += local_result[i];
    group_result[get_group_id(0)] = sum;
}

__kernel void add_vector(
        __global const float *v0,
        __global const float *v1,
        __global float *v2) {
    const uint i = get_global_id(0);
    v2[i] = v0[i] + v1[i];
}

__kernel void mul_vector(
        __global const float *v0,
        __global const float *v1,
        __global float *v2) {
    const uint i = get_global_id(0);
    v2[i] = v0[i] * v1[i];
}

__kernel void mul_mat0(
        __constant float *restrict fn,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __global float *restrict m2) {
    const uint y = get_global_id(0), x = get_global_id(1);
    const uint n = *fn;
    __global float *p = m2 + y * n + x;
    *p = 0;
    for(uint i = 0; i < n; ++i)
        *p += m0[y * n + i] * m1[i * n + x];
}

__kernel void mul_mat1(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __global float *restrict m2) {
    const uint y = get_global_id(0), x = get_global_id(1);
    __global float *p = m2 + y * n + x;
    *p = 0;
    for(uint i = 0; i < n; ++i)
        *p += m0[y * n + i] * m1[i * n + x];
}

__kernel void mul_mat2(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __global float *restrict m2) {
    const uint y = get_global_id(0), x = get_global_id(1);
    float v = 0;
    for(uint i = 0; i < n; ++i)
        v += m0[y * n + i] * m1[i * n + x];
    m2[y * n + x] = v;
}

__kernel void mul_mat3(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __global float *restrict m2) {
    const uint y = get_global_id(0);
    for(uint i = 0; i < n; ++i) {
        float v = 0;
        for(uint j = 0; j < n; ++j)
            v += m0[y * n + j] * m1[j * n + i];
        m2[y * n + i] = v;
    }
}

__kernel void mul_mat4(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __global float *restrict m2) {
    const uint y = get_global_id(0);
    float row[1024];
    for(uint i = 0; i < n; ++i)
        row[i] = m0[y * n + i];
    for(uint i = 0; i < n; ++i) {
        float v = 0;
        for(uint j = 0; j < n; ++j)
            v += row[j] * m1[j * n + i];
        m2[y * n + i] = v;
    }
}

__kernel void mul_mat5(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __local float *restrict m1_local,
        __global float *restrict m2) {
    const uint y = get_global_id(0);
    const uint ln = get_local_size(0), ly = get_local_id(0);
    float row[1024];
    for(uint i = 0; i < n; ++i)
        row[i] = m0[y * n + i];
    for(uint i = 0; i < n; ++i) {
        for(uint j = ly; j < n; j += ln)
            m1_local[j] = m1[j * n + i];
        barrier(CLK_LOCAL_MEM_FENCE);
        float v = 0;
        for(uint j = 0; j < n; ++j)
            v += row[j] * m1_local[j];
        m2[y * n + i] = v;
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

__kernel void mul_mat6(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __local float *restrict m0_local,
        __local float *restrict m1_local,
        __global float *restrict m2) {
    const uint y = get_global_id(0), x = get_global_id(1);
    const uint ly = get_local_id(0), lx = get_local_id(1);
    const uint ls = get_local_size(1);
    const uint ng = n / ls;
    const uint lid = ly * ls + lx;
    float v = 0;
    for(uint ig = 0; ig < ng; ++ig) {
        m0_local[lid] = m0[y * n + ig * ls + lx];
        m1_local[lid] = m1[(ig * ls + ly) * n + x];
        barrier(CLK_LOCAL_MEM_FENCE);
        for(uint i = 0; i < ls; ++i)
            v += m0_local[ly * ls + i] * m1_local[i * ls + lx];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    m2[y * n + x] = v;
}

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16
#endif

__kernel void mul_mat7(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __local float *restrict m0_local,
        __local float *restrict m1_local,
        __global float *restrict m2) {
    const uint y = get_global_id(0), x = get_global_id(1);
    const uint ly = get_local_id(0), lx = get_local_id(1);
    const uint ls = BLOCK_SIZE;
    const uint ng = n / ls;
    const uint lid = ly * ls + lx;
    float v = 0;
    for(uint ig = 0; ig < ng; ++ig) {
        m0_local[lid] = m0[y * n + ig * ls + lx];
        m1_local[lid] = m1[(ig * ls + ly) * n + x];
        barrier(CLK_LOCAL_MEM_FENCE);
        #pragma unroll
        for(uint i = 0; i < ls; ++i)
            v += m0_local[ly * ls + i] * m1_local[i * ls + lx];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    m2[y * n + x] = v;
}

__kernel void mul_mat8(
        const uint n,
        __global const float *restrict m0,
        __global const float *restrict m1,
        __local float *restrict m0_local,
        __local float *restrict m1_local,
        __global float *restrict m2) {
    const uint y = get_global_id(0), x = get_global_id(1);
    const uint ly = get_local_id(0), lx = get_local_id(1);
    const uint ls = BLOCK_SIZE;
    const uint ng = n / ls;
    const uint lid = lx * ls + ly;
    float v = 0;
    for(uint ig = 0; ig < ng; ++ig) {
        m0_local[lid] = m0[x * n + ig * ls + ly];
        m1_local[lid] = m1[(ig * ls + lx) * n + y];
        barrier(CLK_LOCAL_MEM_FENCE);
        #pragma unroll
        for(uint i = 0; i < ls; ++i)
            v += m0_local[lx * ls + i] * m1_local[i * ls + ly];
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    m2[x * n + y] = v;
}

__kernel void mul_mat_nvidia(
        const unsigned int             N,
        __global const float* restrict A,
        __global const float* restrict B,
        __local        float* restrict Awrk,
        __local        float* restrict Bwrk,
        __global       float* restrict C) {
    int kloc, Kblk;
    float Ctmp=0.0f;
    //  This work-item will compute element C(i,j)
    const int i = get_global_id(0);
    const int j = get_global_id(1);
    // Element C(i,j) is in block C(Iblk,Jblk)
    const int Iblk = get_group_id(0);
    const int Jblk = get_group_id(1);
    // C(i,j) is element C(iloc, jloc) of block C(Iblk, Jblk)
    const int iloc = get_local_id(0);
    const int jloc = get_local_id(1);
    // The number of blocks are the same in each dimension
    const int Num_BLK = N/BLOCK_SIZE;
    // Setup the upper-left-corner (base address) for the A and
    // B blocks plus the increments to advance base addresses as
    // we loop over blocks
    int Abase = Jblk*N*BLOCK_SIZE;
    const int Ainc  = BLOCK_SIZE;
    int Bbase = Iblk*BLOCK_SIZE;
    const int Binc  = BLOCK_SIZE*N;
    // C(Iblk,Jblk) = (sum over Kblk) A(Iblk,Kblk)*B(Kblk,Jblk)
    for (Kblk = 0;  Kblk<Num_BLK;  Kblk++) {
        // Load A(Iblk,Kblk) and B(Kblk,Jblk) into local memory.
        // Each work-item loads a single element of the two blocks
        // which are shared with the entire work-group.
        Awrk[jloc*BLOCK_SIZE+iloc] = A[Abase+jloc*N+iloc];
        Bwrk[jloc*BLOCK_SIZE+iloc] = B[Bbase+jloc*N+iloc];
        barrier(CLK_LOCAL_MEM_FENCE);
        // Compute dot products over local blocks to find
        // the contribution to C(i,j) from this block
        #pragma unroll
        for (kloc=0; kloc<BLOCK_SIZE; kloc++)
            Ctmp += Awrk[jloc*BLOCK_SIZE+kloc] * Bwrk[kloc*BLOCK_SIZE+iloc];
        barrier(CLK_LOCAL_MEM_FENCE);
        Abase += Ainc;
        Bbase += Binc;
    }
    // update global C matrix
    C[j*N+i] = Ctmp;
}

__kernel void pi_reduce(const uint n, const float dx, __global float *out) {
    const uint id = get_global_id(0);
    const uint gn = n / get_global_size(0);
    float sum = 0;
    for(uint i = id * gn, e = i + gn; i < e; ++i) {
        const float x = (i + .5f) * dx;
        sum += 4.0f / (1.0f + x * x);
    }
    out[id] = sum;
}

__kernel void pi_iter(
        const uint n,
        const float dx,
        __local float *g,
        __global float *out) {
    const uint id = get_global_id(0);
    float sum = 0;
    for(uint i = id * n, e = i + n; i < e; ++i) {
        const float x = (i + .5f) * dx;
        sum += 4.0f / (1 + x * x);
    }
    g[get_local_id(0)] = sum;
    barrier(CLK_LOCAL_MEM_FENCE);
    reduce(g, out);
}

__kernel void pi_vec4(
        const uint n,
        const float dx,
        __local float *g,
        __global float *out) {
    const uint id = get_global_id(0);
    float sum = 0;
    const float4 is = {.5f, 1.5f, 2.5f, 3.5f};
    const float4 ones = {1.0f, 1.0f, 1.0f, 1.0f};
    const float4 fours = {4.0f, 4.0f, 4.0f, 4.0f};
    for(uint i = id * n, e = i + n; i < e; i += 4) {
        const float4 xs = ((float4)i + is) * dx;
        const float4 sum4 = fours / (ones + xs * xs);
        sum += sum4.s0 + sum4.s1 + sum4.s2 + sum4.s3;
    }
    g[get_local_id(0)] = sum;
    barrier(CLK_LOCAL_MEM_FENCE);
    reduce(g, out);
}

__kernel void pi_vec8(
        const uint n,
        const float dx,
        __local float *g,
        __global float *out) {
    const uint id = get_global_id(0);
    float sum = 0;
    const float8 is = {.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f, 7.5f};
    const float8 ones = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    const float8 fours = {4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f, 4.0f};
    for(uint i = id * n, e = i + n; i < e; i += 8) {
        const float8 xs = ((float8)i + is) * dx;
        const float8 sum8 = fours / (ones + xs * xs);
        sum +=
            sum8[0] + sum8[1] + sum8[2] + sum8[3]
            + sum8[4] + sum8[5] + sum8[6] + sum8[7];
    }
    g[get_local_id(0)] = sum;
    barrier(CLK_LOCAL_MEM_FENCE);
    reduce(g, out);
}
