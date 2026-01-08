#include "sprite_renderer.h"
#include "util.h"
#include <assert.h>
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
  WGPUDevice device;
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
} SpriteRenderer;

SpriteRenderer *context = 0;

void *spriteRendererCreate(WGPUDevice device,
                           WGPUTextureFormat pipelineFormat) {
  assert(context == 0);
  context = malloc(sizeof(SpriteRenderer));
  memset(context, 0, sizeof(SpriteRenderer));

  context->device = device;
#pragma region CREATE PIPELINE_LAYOUT
  WGPUBindGroupLayoutEntry bindGroupEntry[3] = {0};
  bindGroupEntry[0].binding = 0;
  bindGroupEntry[0].visibility = WGPUShaderStage_Fragment;
  bindGroupEntry[0].texture.sampleType = WGPUTextureSampleType_Float;
  bindGroupEntry[0].texture.viewDimension = WGPUTextureViewDimension_2DArray;

  bindGroupEntry[1].binding = 1;
  bindGroupEntry[1].visibility = WGPUShaderStage_Fragment;
  bindGroupEntry[1].sampler.type = WGPUSamplerBindingType_Filtering;

  WGPUBindGroupLayoutDescriptor groupLayoutDesc = {0};
  groupLayoutDesc.entries = bindGroupEntry;
  groupLayoutDesc.entryCount = 2;
  groupLayoutDesc.label = WGPU_STR("SpriteRenderer Layout Group");

  if ((context->bindgroupLayout = wgpuDeviceCreateBindGroupLayout(
           context->device, &groupLayoutDesc)) == 0) {
    spriteRendererFinish();
    return 0;
  }

  WGPUPipelineLayoutDescriptor pipelineLayoutDesc = {0};
  pipelineLayoutDesc.bindGroupLayoutCount = 1;
  pipelineLayoutDesc.bindGroupLayouts = &context->bindgroupLayout;
  pipelineLayoutDesc.label = WGPU_STR("SpriteRenderer Pipeline Layout");

  if ((context->pipelineLayout = wgpuDeviceCreatePipelineLayout(
           context->device, &pipelineLayoutDesc)) == 0) {
    spriteRendererFinish();
    return 0;
  }
#pragma end
#pragma region CREATE_SAMPLER

  WGPUSamplerDescriptor samplerDesc = {0};
  samplerDesc.addressModeU = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeV = WGPUAddressMode_ClampToEdge;
  samplerDesc.addressModeW = WGPUAddressMode_ClampToEdge;

  samplerDesc.magFilter = WGPUFilterMode_Nearest;
  samplerDesc.minFilter = WGPUFilterMode_Nearest;
  samplerDesc.mipmapFilter = WGPUMipmapFilterMode_Linear;
  samplerDesc.lodMaxClamp = 1.f;
  samplerDesc.maxAnisotropy = 1;
  samplerDesc.compare = WGPUCompareFunction_Undefined;

  if ((context->sampler =
           wgpuDeviceCreateSampler(context->device, &samplerDesc)) == 0) {
    spriteRendererFinish();
    return 0;
  }
#pragma end
#pragma region CREATE_PIPELINE
  WGPURenderPipelineDescriptor pipelineDesc = {0};

  WGPUShaderModule module;
  if ((module = createShaderModule(context->device,
                                   "res/shaders/sprite.wgsl")) == 0) {
    spriteRendererFinish();
    return 0;
  }

  WGPUVertexAttribute vertexAttr[3];

  WGPUVertexBufferLayout bufferLayout = {0};

  vertexAttr[0].format = WGPUVertexFormat_Float32x3;
  vertexAttr[0].offset = 0;
  vertexAttr[0].shaderLocation = 0;

  vertexAttr[1].format = WGPUVertexFormat_Float32x2;
  vertexAttr[1].offset = 3 * sizeof(float);
  vertexAttr[1].shaderLocation = 1;

  vertexAttr[2].format = WGPUVertexFormat_Uint32;
  vertexAttr[2].offset = 5 * sizeof(uint32_t);
  vertexAttr[2].shaderLocation = 2;

  bufferLayout.arrayStride = 5 * sizeof(float) + sizeof(uint32_t);
  bufferLayout.attributeCount = 3;
  bufferLayout.attributes = vertexAttr;
  bufferLayout.stepMode = WGPUVertexStepMode_Vertex;

  pipelineDesc.vertex.module = module;
  pipelineDesc.vertex.entryPoint = WGPU_STR("vs_main");
  pipelineDesc.vertex.buffers = &bufferLayout;
  pipelineDesc.vertex.bufferCount = 1;

  pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
  pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
  pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
  pipelineDesc.primitive.cullMode = WGPUCullMode_None;

  WGPUFragmentState fragment = {0};

  fragment.module = module;
  fragment.entryPoint = WGPU_STR("fs_main");

  pipelineDesc.fragment = &fragment;

  pipelineDesc.depthStencil = 0;

  WGPUBlendState blendState = {0};
  WGPUColorTargetState colorTarget = {0};
  colorTarget.format = pipelineFormat;
  colorTarget.blend = 0;
  colorTarget.writeMask = WGPUColorWriteMask_All;
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  pipelineDesc.multisample.count = 1;
  pipelineDesc.multisample.mask = ~0u;
  pipelineDesc.multisample.alphaToCoverageEnabled = 0;
  pipelineDesc.layout = context->pipelineLayout;

  context->pipeline =
      wgpuDeviceCreateRenderPipeline(context->device, &pipelineDesc);

  wgpuShaderModuleRelease(module);

  if (context->pipeline == 0) {
    spriteRendererFinish();
    return 0;
  }
#pragma END
#pragma region CREATE_SPRITE_BUFFER
  context->spriteBuffer = bufferCreate(
      context->device, "Sprite", sizeof(SpriteData) * INITIAL_MAX_SPRITES,
      WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc |
          WGPUBufferUsage_Vertex);
  if (context->spriteBuffer.buffer == 0) {
    spriteRendererFinish();
    return 0;
  }
  context->maxSprites = INITIAL_MAX_SPRITES;
  context->numSprites = 0;
  context->texH = context->texW = 1;
  context->cpuSpriteBuffer = malloc(sizeof(SpriteData) * INITIAL_MAX_SPRITES);
  if (context->cpuSpriteBuffer->data == 0) {
    spriteRendererFinish();
    return 0;
  }

#pragma end

  return context;
}

void deleteBindGroups() {
  if (context->groups) {
    for (int i = 0; i < context->numGroups; i++) {
      wgpuBindGroupRelease(context->groups[i]);
    }
    context->groups = 0;
    context->numGroups = 0;
  }
}

void spriteRendererFinish() {
  deleteBindGroups();
  if (context->spriteBuffer.buffer) {
    bufferDestroy(&context->spriteBuffer);
  }
  if (context->pipeline) {
    wgpuRenderPipelineRelease(context->pipeline);
  }

  if (context->bindgroupLayout) {
    wgpuBindGroupLayoutRelease(context->bindgroupLayout);
  }
  if (context->pipelineLayout) {
    wgpuPipelineLayoutRelease(context->pipelineLayout);
  }

  free(context);
  context = 0;
}

void spriteRendererUpdateTextures(TextureView *textures, size_t num) {
  WGPUDevice device = context->device;
  deleteBindGroups();
  context->numGroups = num;
  context->groups = malloc(sizeof(WGPUBindGroup) * context->numGroups);
  WGPUBindGroupDescriptor desc = {0};
  desc.layout = context->bindgroupLayout;
  WGPUBindGroupEntry entries[2] = {0};
  entries[0].binding = 0;

  entries[1].binding = 1;
  entries[1].sampler = context->sampler;
  desc.entryCount = 2;
  desc.entries = entries;

  for (int i = 0; i < context->numGroups; i++) {
    entries[0].textureView = textures[i].view;
    context->groups[i] = wgpuDeviceCreateBindGroup(context->device, &desc);
  }
}

#define clipX(x) (((float)(x) / viewport->w) * 2 - 1)
#define clipY(y) (((float)(y) / viewport->h) * 2 - 1)

void spriteRendererDraw(Sprite *spr, Rect *viewport) {
  WGPUDevice device = context->device;
  if (context->numSprites == context->maxSprites) {
    // [RESIZE SPRITE BUFFER]
    context->maxSprites = context->maxSprites * 2;
    void *prevData = context->cpuSpriteBuffer;
    if ((context->cpuSpriteBuffer =
             malloc(context->maxSprites * sizeof(SpriteData))) == 0) {
      printf("Error malloc at file %s line %d", __FILE_NAME__, __LINE__);
      abort();
    }
    memcpy(context->cpuSpriteBuffer, prevData,
           context->numSprites * sizeof(SpriteData));
    free(prevData);

    bufferDestroy(&context->spriteBuffer);
    context->spriteBuffer = bufferCreate(
        device, "Sprites", sizeof(SpriteData) * context->maxSprites,
        WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc |
            WGPUBufferUsage_Vertex);
  }

  float depth = spr->depth;

  float left = clipX(spr->dst.x), right = clipX(spr->dst.x + spr->dst.w);
  float up = clipY(spr->dst.y), down = clipY(spr->dst.y + spr->dst.h);

  float uleft = spr->src.x / context->texW,
        uright = (spr->src.x + spr->src.w) / context->texW;
  float uup = spr->src.y / context->texH,
        udown = (spr->src.y + spr->src.h) / context->texH;
  SpriteData data = {
      .data = {
          {.pos = {left, up, depth}, .uv = {uleft, uup}, .id = spr->idx},
          {.pos = {right, up, depth}, .uv = {uright, uup}, .id = spr->idx},
          {.pos = {right, down, depth}, .uv = {uright, udown}, .id = spr->idx},
          {.pos = {left, up, depth}, .uv = {uleft, uup}, .id = spr->idx},
          {.pos = {right, down, depth}, .uv = {uright, udown}, .id = spr->idx},
          {.pos = {left, down, depth}, .uv = {uleft, udown}, .id = spr->idx},

      }};

  context->cpuSpriteBuffer[context->numSprites++] = data;
}

void spriteRendererInitPass(uint32_t id, Size textureSize) {
  context->texture = id;
  context->texW = textureSize.w;
  context->texH = textureSize.h;
  context->numSprites = 0;
}

void spriteRendererEndPass(WGPUQueue queue, WGPURenderPassEncoder encoder) {
  bufferWrite(queue, &context->spriteBuffer,
              sizeof(SpriteData) * context->numSprites,
              context->cpuSpriteBuffer);
  wgpuRenderPassEncoderSetBindGroup(encoder, 0,
                                    context->groups[context->texture], 0, 0);

  wgpuRenderPassEncoderSetPipeline(encoder, context->pipeline);
  wgpuRenderPassEncoderSetVertexBuffer(
      encoder, 0, context->spriteBuffer.buffer, 0,
      sizeof(SpriteData) * context->numSprites);
  wgpuRenderPassEncoderDraw(encoder, context->numSprites * 6, 1, 0, 0);
}
