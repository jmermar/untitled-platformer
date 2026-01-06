#include "sprite_renderer.h"
#include "buffers.h"
#include "pipelines.h"
#include "wgpu.h"
#include <stdio.h>
#define INITIAL_MAX_SPRITES 256

typedef struct {
  float pos[3];
  float uv[2];
} SpriteVertex;

typedef struct {
  SpriteVertex data[6];
} SpriteData;

struct {
  WGPURenderPipeline pipeline;
  Buffer spriteBuffer;
  size_t maxSprites;
  size_t numSprites;
  SpriteData *cpuSpriteBuffer;

  float texW, texH;
} context = {0};

int createPipeline() {
  WGPURenderPipelineDescriptor desc = {0};

  WGPUShaderModule module;
  if ((module = createShaderModule("res/shaders/sprite.wgsl")) == 0) {
    return -1;
  }

  WGPUVertexAttribute attr[2];

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
  colorTarget.format = renderContext.format;
  colorTarget.blend = 0;
  colorTarget.writeMask = WGPUColorWriteMask_All;
  fragment.targetCount = 1;
  fragment.targets = &colorTarget;

  desc.multisample.count = 1;
  desc.multisample.mask = ~0u;
  desc.multisample.alphaToCoverageEnabled = 0;

  context.pipeline =
      wgpuDeviceCreateRenderPipeline(renderContext.device, &desc);

  wgpuShaderModuleRelease(module);

  if (context.pipeline == 0)
    return -1;
  return 0;
}

int spriteRendererCreate() {
  if (createPipeline()) {
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

void spriteRendererFinish() {
  if (context.spriteBuffer.buffer) {
    bufferDestroy(&context.spriteBuffer);
    context.spriteBuffer = (Buffer){0};
  }
  if (context.pipeline) {
    wgpuRenderPipelineRelease(context.pipeline);
    context.pipeline = 0;
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

float clipX(int32_t x) { return ((float)x / renderContext.width) * 2 - 1; }
float clipY(int32_t y) {
  return (1 - ((float)y / renderContext.height)) * 2 - 1;
}

void drawSprite(Sprite *spr) {
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
  SpriteData data = {.data = {
                         {.pos = {left, up, depth}, .uv = {uleft, uup}},
                         {.pos = {right, up, depth}, .uv = {uright, uup}},
                         {.pos = {right, down, depth}, .uv = {uright, udown}},
                         {.pos = {left, up, depth}, .uv = {uleft, uup}},
                         {.pos = {right, down, depth}, .uv = {uright, udown}},
                         {.pos = {left, down, depth}, .uv = {uleft, udown}},

                     }};

  context.cpuSpriteBuffer[context.numSprites++] = data;
}

void spriteRendererPass() {
  bufferWrite(&context.spriteBuffer, sizeof(SpriteData) * context.numSprites,
              context.cpuSpriteBuffer);

  wgpuRenderPassEncoderSetPipeline(renderContext.frameData.renderPass,
                                   context.pipeline);
  wgpuRenderPassEncoderSetVertexBuffer(renderContext.frameData.renderPass, 0,
                                       context.spriteBuffer.buffer, 0,
                                       sizeof(SpriteData) * context.numSprites);
  wgpuRenderPassEncoderDraw(renderContext.frameData.renderPass,
                            context.numSprites * 6, 1, 0, 0);

  context.numSprites = 0;
}
