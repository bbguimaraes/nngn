LAYOUT(location = 1) in vec3 frag_pos;
LAYOUT(location = 2) flat in vec3 frag_normal;

float attenuation(uint i, float d) {
    return 1.0f / (
        lights.point.att_cutoff[i][0]
        + lights.point.att_cutoff[i][1] * d
        + lights.point.att_cutoff[i][2] * d * d);
}

float spot(uint i, vec3 d) {
    float inner = lights.point.att_cutoff[i].w;
    if(inner == 0.0)
        return 1.0;
    const float penumbra = 0.1;
    float outer = inner - penumbra;
    float a = dot(-d, lights.point.dir[i].xyz);
    return a > outer ? clamp((a - outer) / penumbra, 0.0, 1.0) : 0.0;
}

float light_spec(float spec, vec3 dir, vec3 view_dir) {
    return spec * pow(
        max(dot(view_dir, reflect(dir, frag_normal)), 0.0f),
        32.0f);
}

vec3 light() {
    vec3 view_dir = normalize(lights.view_pos - frag_pos);
    vec3 light = lights.ambient;
    for(uint i = 0u; i < lights.n_dir; ++i) {
        float a = dot(frag_normal, -lights.dir.dir[i].xyz);
        if(a <= 0.0f)
            continue;
        light += lights.dir.color_spec[i].xyz * (
            a + light_spec(
                lights.dir.color_spec[i].w,
                lights.dir.dir[i].xyz,
                view_dir));
    }
    for(uint i = 0u; i < lights.n_point; ++i) {
        float a = dot(frag_normal, lights.point.pos[i].xyz - frag_pos);
        if(a <= 0.0f)
            continue;
        vec3 d = lights.point.pos[i].xyz - frag_pos;
        float att = attenuation(i, length(d));
        d = normalize(d);
        float diff = max(0.0f, dot(frag_normal, d));
        float spec = light_spec(lights.point.color_spec[i].w, -d, view_dir);
        light += lights.point.color_spec[i].xyz
            * att * spot(i, d) * (diff + spec);
    }
    return light;
}
