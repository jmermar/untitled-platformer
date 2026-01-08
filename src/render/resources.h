#pragma once
#include "render_imp.h"

TextureView textureViewCreate(const char *name, Size size, uint32_t layers,
                              WGPUTextureFormat format, WGPUTextureUsage usage);

void textureViewDestroy(TextureView *tex);

void textureViewWrite(TextureView *tex, void *data);

WGPUShaderModule createShaderModule(const char *filepath);

WGPUVertexBufferLayout getSpriteBufferLayout(WGPUVertexAttribute *attr);

Buffer bufferCreate(const char *label, size_t size, WGPUBufferUsage usage);
void bufferDestroy(Buffer *buffer);

void bufferWrite(Buffer *buffer, size_t size, void *data);