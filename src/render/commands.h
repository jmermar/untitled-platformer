#pragma once
#include "render_imp.h"

CommandBuffer createCommandBuffer(size_t capacity);
void commandBufferDestroy(CommandBuffer *buffer);
void commandBufferResize(CommandBuffer *buffer, size_t newCapacity);
void commandBufferAppend(CommandBuffer *buffer, WGPUCommandBuffer command);
void commandBufferClear(CommandBuffer *buffer);