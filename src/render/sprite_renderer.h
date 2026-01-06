#pragma once
#include "../types.h"
#include "render_context.h"

typedef struct {
  Rect src, dst;
  float depth;
} Sprite;

int spriteRendererCreate();
void spriteRendererFinish();

void drawSprite(Sprite *spr);

void spriteRendererPass();