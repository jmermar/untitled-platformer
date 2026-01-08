struct VertexOut {
  @builtin(position) pos: vec4f,
  @location(0) uv: vec2f
};

@vertex
fn vs_main(@builtin(vertex_index) vertex_index: u32) -> VertexOut {
    var pos = array<vec2f, 6>(
        vec2(-1.0, -1.0),
        vec2(1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, -1.0),
        vec2(1.0, 1.0),
        vec2(-1.0, 1.0)
    );

    var out: VertexOut;
    out.pos = vec4f(pos[vertex_index % 6], 0.0, 1.0);
    out.uv = (out.pos.xy  + vec2f(1.0)) * 0.5;
    return out;
}

@group(0) @binding(0) var texture: texture_2d_array<f32>;
@group(0) @binding(1) var texSampler: sampler;
@fragment
fn fs_main(in: VertexOut) -> @location(0) vec4f {
    return textureSample(texture, texSampler, in.uv.xy, 0);
}