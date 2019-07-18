struct rnd_gen { uint4 state; };

// https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-37-efficient-random-number-generation-and-application
// https://math.stackexchange.com/questions/337782/pseudo-random-number-generation-on-the-gpu
uint taus_step(uint z, int s1, int s2, int s3, uint m) {
    uint b = ((z << s1) ^ z) >> s2;
    return ((z & m) << s3) ^ b;
}

uint lcg_step(uint z, uint a, uint c) { return a * z + c; }

float rnd_gen_next(struct rnd_gen *g) {
    uint4 state = g->state;
    state.x = taus_step(state.x, 13, 19, 12, 4294967294);
    state.y = taus_step(state.y, 2, 25, 4, 4294967288);
    state.z = taus_step(state.z, 3, 11, 17, 4294967280);
    state.w = lcg_step(state.w, 1664525, 1013904223);
    g->state = state;
    return 2.3283064365387e-10f * (state.x ^ state.y ^ state.z ^ state.w);
}
