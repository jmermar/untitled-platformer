#include "commands.h"
#include <memory.h>
#include <stdlib.h>

CommandBuffer createCommandBuffer(size_t capacity)
{
    return (CommandBuffer){.capacity=capacity, .commands = malloc(sizeof(WGPUCommandBuffer)*capacity)};
}

void commandBufferDestroy(CommandBuffer *buffer)
{
    free(buffer->commands);
}

void commandBufferResize(CommandBuffer *buffer, size_t newCapacity)
{
    size_t cpyLength = (newCapacity > buffer->capacity ? buffer->capacity : newCapacity) * sizeof(WGPUCommandBuffer);

    void* old = buffer->commands;
    buffer->commands = malloc(sizeof(WGPUCommandBuffer) * newCapacity);
    memcpy(buffer->commands, old, cpyLength);
    free(old);
}

void commandBufferAppend(CommandBuffer *buffer, WGPUCommandBuffer command)
{
    if (buffer->size == buffer->capacity) {
        commandBufferResize(buffer, buffer->capacity + 256);
    }

    buffer->commands[buffer->size++] = command;
}

void commandBufferClear(CommandBuffer *buffer)
{
    for(size_t i = 0; i < buffer->size; i++) {
        wgpuCommandBufferRelease(buffer->commands[i]);
    }
    buffer->size = 0;
}