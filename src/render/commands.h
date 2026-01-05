#pragma once
#include <dawn/webgpu.h>
#include <stdint.h>

typedef struct {
    size_t capacity;
    size_t size;
    WGPUCommandBuffer* commands;
} CommandBuffer;

CommandBuffer createCommandBuffer(size_t capacity);
void commandBufferDestroy(CommandBuffer *buffer);
void commandBufferResize(CommandBuffer *buffer, size_t newCapacity);
void commandBufferAppend(CommandBuffer *buffer, WGPUCommandBuffer command);
void commandBufferClear(CommandBuffer *buffer);