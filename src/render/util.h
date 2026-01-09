#pragma once
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webgpu/webgpu.h>

#define WGPU_STR(str) ((WGPUStringView){.data = str, .length = strlen(str)})

typedef struct {
  WGPUTexture texture;
  WGPUTextureView view;
  WGPUTextureFormat format;
  WGPUExtent3D size;
  WGPUTextureAspect aspect;
} TextureView;

typedef struct {
  WGPUSurfaceTexture texture;
  WGPUTextureView view;
} SurfaceTextureView;

typedef struct {
  WGPUBuffer buffer;
  size_t size;
} Buffer;

typedef struct {
  size_t capacity;
  size_t size;
  WGPUCommandBuffer *commands;
} CommandBuffer;

TextureView textureViewCreate(WGPUDevice device, const char *name, uint32_t w,
                              uint32_t h, uint32_t layers,
                              WGPUTextureFormat format, WGPUTextureUsage usage);
void textureViewDestroy(TextureView *tex);
void textureViewWrite(WGPUQueue queue, TextureView *tex, void *data);

WGPUShaderModule createShaderModule(WGPUDevice device, const char *filepath);

Buffer bufferCreate(WGPUDevice device, const char *label, size_t size,
                    WGPUBufferUsage usage);
void bufferDestroy(Buffer *buffer);
void bufferWrite(WGPUQueue queue, Buffer *buffer, size_t size, void *data);

CommandBuffer createCommandBuffer(size_t capacity);
void commandBufferDestroy(CommandBuffer *buffer);
void commandBufferResize(CommandBuffer *buffer, size_t newCapacity);
void commandBufferAppend(CommandBuffer *buffer, WGPUCommandBuffer command);
void commandBufferClear(CommandBuffer *buffer);
