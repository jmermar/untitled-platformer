#pragma once
#include "../types.h"
#include <dawn/webgpu.h>

typedef struct {
  WGPUTexture texture;
  WGPUTextureView view;
  WGPUTextureFormat format;
  WGPUExtent3D size;
  WGPUTextureAspect aspect;
} TextureView;

TextureView textureViewCreate(const char *name, Size size, uint32_t layers,
                              WGPUTextureFormat format, WGPUTextureUsage usage);

void textureViewDestroy(TextureView *tex);

void textureViewWrite(TextureView *tex, void *data);