#include "sprite_renderer.h"
#include "buffers.h"
#include "pipelines.h"
#include "wgpu.h"
#include <stdio.h>
#define INITIAL_MAX_SPRITES 256

typedef struct {
  float pos[3];
  float uv[2];
  uint32_t id;
} SpriteVertex;

typedef struct {
  SpriteVertex data[6];
} SpriteData;

typedef struct {
  WGPURenderPipeline pipeline;
  WGPUPipelineLayout pipelineLayout;
  WGPUBindGroupLayout bindgroupLayout;
  WGPUSampler sampler;

  Buffer spriteBuffer;
  size_t maxSprites;
  size_t numSprites;
  SpriteData *cpuSpriteBuffer;

  uint32_t texture;
  float texW, texH;

  WGPUBindGroup *groups;
  size_t numGroups;
} SpriteRendererContext;
SpriteRendererContext context = {0};

int createPipelineLayout() {
  WGPUBindGroupLayoutEntry entry[3] = {0};
  entry[0].binding = 0;
  entry[0].visibility = WGPUShaderStage_Fragment;
  entry[0].texture.sampleType = WGPUTextureSampleType_Float;
  entry[0].texture.viewDimension = WGPUTextureViewDimension_2DArray;

  entry[1].binding = 1;
  entry[1].visibility = WGPUShaderStage_Fragment;
  entry[1].sampler.type = WGPUSamplerBindingType_Filtering;

  WGPUBindGroupLayoutDescriptor desc = {0};
  desc.entries = entry;
  desc.entryCount = 2;
  desc.label = WGPU_STR("SpriteRenderer Layout Group");

  if ((context.bindgroupLayout =
           wgpuDeviceCreateBindGroupLayout(renderContext.device, &desc)) == 0) {
    return -1;
  }

  WGPUPipelineLayoutDescriptor layoutDesc = {0};
  layoutDesc.bindGroupLayoutCount = 1;
  layoutDesc.bindGroupLayouts = &context.bindgroupLayout;
  layoutDesc.label = WGPU_STR("SpriteRenderer Pipeline Layout");

  if ((context.pipelineLayout = wgpuDeviceCreatePipelineLayout(
           renderContext.device, &layoutDesc)) == 0) {
    return -1;
  }

  return 0;
}

int createSampler() {
  WGPUSamplerDescriptor desc = {0};
  desc.addressModeU = WGPUAddressMode_ClampToEdge;
  desc.addressModeV = WGPUAddressMode_ClampToEdge;
  desc.addressModeW = WGPUAddressMode_ClampToEdge;

  desc.magFilter = WGPUFilterMode_Nearest;
  desc.minFilter = WGPUFilterMode_Nearest;
  desc.mipmapFilter = WGPUMipmapFilterMode_Linear;
  desc.lodMaxClamp = 1.f;
  desc.maxAnisotropy = 1;
  desc.compare = WGPUCompareFunction_Undefined;

  return (context.sampler =
              wgpuDeviceCreateSampler(renderContext.device, &desc))
             ? 0
             : -1;
}

int createPipeline() {
  WGPURenderPipelineDescriptor desc = {0};

  WGPUShaderModule module;
  if ((module = createShaderModule("res/shaders/sprite.wgsl")) == 0) {
    return -1;
  }

  WGPUVertexAttribute attr[3];

  WGPUVertexBufferLayout bufferLayout = getSpriteBufferLayout(attr);

  desc.vertex.module = module;
  desc.vertex.entryPoint = WGPU_STR("vs_main");
  desc.vertex.buffers = &bufferLayout;
  desc.vertex.bufferCount = 1;

  desc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  desc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  desc.primitive.frontFace = WGPUFrontFace_CCW;
  desc.primitive.cullMode = WGPUCullMode_None;

  WGPUFragmentState fragment = {0};

  fragment.module = module;
  fragment.entryPoint = WGPU_STR("fs_main");

  desc.fragment = &fragment;

  desc.depthStencil = 0;

  WGPUBlendState blendState = {0};
  WGPUColorTargetState colorTarget = {0};
  colorTarget.format = WGPUTextureFormat_RGBA8Unorm;
  colorTarget.blend = 0;
  colorTarget.writeMask = WGPUColorWriteMask_All;
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  desc.multisample.count = 1;
  desc.multisample.mask = ~0u;
  desc.multisample.alphaToCoverageEnabled = 0;
  desc.layout = context.pipelineLayout;

  context.pipeline =
      wgpuDeviceCreateRenderPipeline(renderContext.device, &desc);

  wgpuShaderModuleRelease(module);

  if (context.pipeline == 0)
    return -1;
  return 0;
}

int spriteRendererCreate() {
  if (createPipelineLayout()) {
    spriteRendererFinish();
    return -1;
  }

  if (createSampler()) {
    spriteRendererFinish();
    return -1;
  }

  if (createPipeline()) {
    spriteRendererFinish();
    return -1;
  }
  context.spriteBuffer =
      bufferCreate("Sprite", sizeof(SpriteData) * INITIAL_MAX_SPRITES,
                   BUFFER_USAGE_DEFAULT | WGPUBufferUsage_Vertex);
  if (context.spriteBuffer.buffer == 0) {
    spriteRendererFinish();
    return -1;
  }
  context.maxSprites = INITIAL_MAX_SPRITES;
  context.numSprites = 0;
  context.texH = context.texW = 1;
  context.cpuSpriteBuffer = malloc(sizeof(SpriteData) * INITIAL_MAX_SPRITES);
  if (context.cpuSpriteBuffer->data == 0) {
    return -1;
  }

  return 0;
}

void deleteBindGroups() {
  if (context.groups) {
    for (int i = 0; i < context.numGroups; i++) {
      wgpuBindGroupRelease(context.groups[i]);
    }
    context.groups = 0;
    context.numGroups = 0;
  }
}

void spriteRendererFinish() {
  deleteBindGroups();
  if (context.spriteBuffer.buffer) {
    bufferDestroy(&context.spriteBuffer);
  }
  if (context.pipeline) {
    wgpuRenderPipelineRelease(context.pipeline);
  }

  if (context.bindgroupLayout) {
    wgpuBindGroupLayoutRelease(context.bindgroupLayout);
  }
  if (context.pipelineLayout) {
    wgpuPipelineLayoutRelease(context.pipelineLayout);
  }

  context = (SpriteRendererContext){0};
}

void spriteRendererUpdateTextures() {
  deleteBindGroups();
  context.numGroups = renderContext.numTextures;
  context.groups = malloc(sizeof(WGPUBindGroup) * context.numGroups);
  WGPUBindGroupDescriptor desc = {0};
  desc.layout = context.bindgroupLayout;
  WGPUBindGroupEntry entries[2] = {0};
  entries[0].binding = 0;

  entries[1].binding = 1;
  entries[1].sampler = context.sampler;
  desc.entryCount = 2;
  desc.entries = entries;

  for (int i = 0; i < context.numGroups; i++) {
    entries[0].textureView = renderContext.textures[i].view;
    context.groups[i] = wgpuDeviceCreateBindGroup(renderContext.device, &desc);
  }
}

void resizeSpriteBuffer() {
  context.maxSprites = context.maxSprites * 2;
  void *prevData = context.cpuSpriteBuffer;
  if ((context.cpuSpriteBuffer =
           malloc(context.maxSprites * sizeof(SpriteData))) == 0) {
    printf("Error malloc at file %s line %d", __FILE_NAME__, __LINE__);
    abort();
  }
  memcpy(context.cpuSpriteBuffer, prevData,
         context.numSprites * sizeof(SpriteData));
  free(prevData);

  bufferDestroy(&context.spriteBuffer);
  context.spriteBuffer =
      bufferCreate("Sprites", sizeof(SpriteData) * context.maxSprites,
                   BUFFER_USAGE_DEFAULT | WGPUBufferUsage_Vertex);
}

float clipX(int32_t x) {
  return ((float)x / renderContext.backbuffer.size.width) * 2 - 1;
}
float clipY(int32_t y) {
  return ((float)y / renderContext.backbuffer.size.height) * 2 - 1;
}

void spriteRendererDraw(Sprite *spr) {
  if (context.numSprites == context.maxSprites) {
    resizeSpriteBuffer();
  }

  float depth = spr->depth;

  float left = clipX(spr->dst.x), right = clipX(spr->dst.x + spr->dst.w);
  float up = clipY(spr->dst.y), down = clipY(spr->dst.y + spr->dst.h);

  float uleft = spr->src.x / context.texW,
        uright = (spr->src.x + spr->src.w) / context.texW;
  float uup = spr->src.y / context.texH,
        udown = (spr->src.y + spr->src.h) / context.texH;
  SpriteData data = {
      .data = {
          {.pos = {left, up, depth}, .uv = {uleft, uup}, .id = spr->idx},
          {.pos = {right, up, depth}, .uv = {uright, uup}, .id = spr->idx},
          {.pos = {right, down, depth}, .uv = {uright, udown}, .id = spr->idx},
          {.pos = {left, up, depth}, .uv = {uleft, uup}, .id = spr->idx},
          {.pos = {right, down, depth}, .uv = {uright, udown}, .id = spr->idx},
          {.pos = {left, down, depth}, .uv = {uleft, udown}, .id = spr->idx},

      }};

  context.cpuSpriteBuffer[context.numSprites++] = data;
}

void spriteRendererInitPass(uint32_t textureID) {
  context.texture = textureID;
  context.texW = renderContext.textures[textureID].size.width;
  context.texH = renderContext.textures[textureID].size.height;
  context.numSprites = 0;
}

void spriteRendererEndPass(WGPURenderPassEncoder encoder) {
  bufferWrite(&context.spriteBuffer, sizeof(SpriteData) * context.numSprites,
              context.cpuSpriteBuffer);
  wgpuRenderPassEncoderSetBindGroup(encoder, 0, context.groups[context.texture],
                                    0, 0);

  wgpuRenderPassEncoderSetPipeline(encoder, context.pipeline);
  wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, context.spriteBuffer.buffer,
                                       0,
                                       sizeof(SpriteData) * context.numSprites);
  wgpuRenderPassEncoderDraw(encoder, context.numSprites * 6, 1, 0, 0);
}
