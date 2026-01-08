#pragma once
#include "../types.h"
#include "render_imp.h"

typedef struct {
  Rect src, dst;
  float depth;
  uint32_t idx;
} Sprite;

int spriteRendererCreate();
void spriteRendererFinish();

void spriteRendererUpdateTextures();

void spriteRendererInitPass(uint32_t textureID);
void spriteRendererDraw(Sprite *spr);
void spriteRendererEndPass(WGPURenderPassEncoder renderPass);

void spriteRendererBlitImage(WGPURenderPassEncoder renderPass, uint32_t texID);
