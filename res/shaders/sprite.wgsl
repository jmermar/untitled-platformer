struct VertexOut {
  @builtin(position) pos: vec4f,
  @location(0) uv: vec2f,
  @location(1)@interpolate(flat) idx: u32
};

@vertex
fn vs_main(
    @location(0) pos: vec3f,
    @location(1) uv: vec2f,
    @location(2) idx: u32) -> VertexOut {
    var out: VertexOut;
    out.pos = vec4f(pos, 1);
    out.uv = uv;
    out.idx = idx;
    return out;
}

@group(0) @binding(0) var texture: texture_2d_array<f32>;
@group(0) @binding(1) var texSampler: sampler;
@fragment
fn fs_main(in: VertexOut) -> @location(0) vec4f {
    let color = textureSample(texture, texSampler, in.uv.xy, in.idx).rgb;
    return vec4f(color, 1.0);
}