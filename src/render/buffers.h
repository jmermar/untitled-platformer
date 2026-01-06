#pragma once
#include <dawn/webgpu.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  WGPUBuffer buffer;
  size_t size;
} Buffer;

#define BUFFER_USAGE_DEFAULT (WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc)

Buffer bufferCreate(const char *label, size_t size, WGPUBufferUsage usage);
void bufferDestroy(Buffer *buffer);

void bufferWrite(Buffer *buffer, size_t size, void *data);