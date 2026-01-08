#pragma once
#include "../types.h"
#include "util.h"

typedef struct {
  Rect src, dst;
  float depth;
  uint32_t idx;
} Sprite;

void *spriteRendererCreate(WGPUDevice device, WGPUTextureFormat pipelineFormat);
void spriteRendererFinish();

void spriteRendererUpdateTextures(TextureView *textures, size_t num);

void spriteRendererInitPass(uint32_t id, Size textureSize);
void spriteRendererDraw(Sprite *spr, Rect *viewport);
void spriteRendererEndPass(WGPUQueue queue, WGPURenderPassEncoder renderPass);

void spriteRendererBlitImage(WGPURenderPassEncoder renderPass, uint32_t texID);
