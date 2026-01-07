#include "buffers.h"
#include "render_context.h"
#include "wgpu.h"
#include <assert.h>
Buffer bufferCreate(const char *label, size_t size, WGPUBufferUsage usage) {
  assert(size > 0);
  WGPUBufferDescriptor desc = {0};
  desc.size = size;
  desc.usage = usage;
  desc.label = WGPU_STR(label);
  return (Buffer){.buffer = wgpuDeviceCreateBuffer(renderContext.device, &desc),
                  .size = size};
}
void bufferDestroy(Buffer *buffer) {
  if (buffer && buffer->buffer) {
    wgpuBufferRelease(buffer->buffer);
    *buffer = (Buffer){0};
  }
}

void bufferWrite(Buffer *buffer, size_t size, void *data) {
  assert(buffer && buffer->size >= size);
  wgpuQueueWriteBuffer(renderContext.queue, buffer->buffer, 0, data, size);
}