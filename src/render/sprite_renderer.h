#pragma once
#include "render_context.h"
#include "../types.h"

int spriteRendererCreate();
void spriteRendererFinish();

void renderSprite(Rect* dst, Rect* src);

void spriteRendererPass();