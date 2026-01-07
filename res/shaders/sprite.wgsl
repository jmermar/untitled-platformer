struct VertexOut {
  @builtin(position) pos: vec4f,
  @location(0) uv: vec2f,
};

@vertex
fn vs_main(
    @location(0) pos: vec3f,
    @location(1) uv: vec2f) -> VertexOut {
    var out: VertexOut;
    out.pos = vec4f(pos, 1);
    out.uv = uv;
    return out;
}

@group(0) @binding(0) var texture: texture_2d<f32>;
@fragment
fn fs_main(in: VertexOut) -> @location(0) vec4f {
    let color = textureLoad(texture, in.uv).rgb;
    return vec4f(color, 1.0);
}