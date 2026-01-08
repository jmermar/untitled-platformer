#include "util.h"
#include "../files.h"
#include "assert.h"
TextureView textureViewCreate(WGPUDevice device, const char *name, uint32_t w,
                              uint32_t h, uint32_t layers,
                              WGPUTextureFormat format,
                              WGPUTextureUsage usage) {
  WGPUTextureDescriptor desc = {0};
  desc.format = format;
  desc.label = WGPU_STR(name);
  desc.mipLevelCount = 1;
  desc.size =
      (WGPUExtent3D){.width = w, .height = h, .depthOrArrayLayers = layers};
  desc.viewFormatCount = 0;
  desc.viewFormats = 0;
  desc.dimension = WGPUTextureDimension_2D;
  desc.usage = usage;
  desc.sampleCount = 1;
  WGPUTexture tex;
  if (!(tex = wgpuDeviceCreateTexture(device, &desc))) {
    printf("Error creating texture\n");
    abort();
  }

  WGPUTextureViewDescriptor vdesc = {0};
  vdesc.arrayLayerCount = layers;
  vdesc.baseArrayLayer = 0;
  vdesc.baseMipLevel = 0;
  vdesc.mipLevelCount = 1;
  vdesc.format = format;
  vdesc.label = WGPU_STR(name);
  vdesc.usage = usage;
  vdesc.aspect = WGPUTextureAspect_All;
  vdesc.dimension = WGPUTextureViewDimension_2DArray;

  WGPUTextureView view;
  if (!(view = wgpuTextureCreateView(tex, &vdesc))) {
    printf("Error creating texture view\n");
    abort();
  }

  return (TextureView){.format = format,
                       .view = view,
                       .texture = tex,
                       .size = desc.size,
                       .aspect = vdesc.aspect};
}

void textureViewDestroy(TextureView *tex) {
  if (!tex)
    return;
  if (tex->view) {
    wgpuTextureViewRelease(tex->view);
  }
  if (tex->texture) {
    wgpuTextureRelease(tex->texture);
  }

  *tex = (TextureView){0};
}

size_t getTexturePixelSize(TextureView *t) {
  switch (t->format) {
  case WGPUTextureFormat_RGBA8Uint:
  case WGPUTextureFormat_RGBA8Unorm:
    return 4;
  case WGPUTextureFormat_RGBA32Float:
    return 16;
  case WGPUTextureFormat_R32Float:
    return 4;
  }
  printf("Unssuported texture format\n");
  abort();
}

void textureViewWrite(WGPUQueue queue, TextureView *tex, void *data) {
  size_t bytes = getTexturePixelSize(tex);
  WGPUTexelCopyTextureInfo info = {0};
  info.mipLevel = 0;
  info.aspect = tex->aspect;
  info.origin = (WGPUOrigin3D){.x = 0, .y = 0, .z = 0};
  info.texture = tex->texture;
  WGPUTexelCopyBufferLayout layout = {0};
  layout.bytesPerRow = bytes * tex->size.width;
  layout.rowsPerImage = tex->size.height;
  wgpuQueueWriteTexture(queue, &info, data,
                        layout.bytesPerRow * tex->size.height *
                            tex->size.depthOrArrayLayers,
                        &layout, &tex->size);
}

Buffer bufferCreate(WGPUDevice device, const char *label, size_t size,
                    WGPUBufferUsage usage) {
  assert(size > 0);
  WGPUBufferDescriptor desc = {0};
  desc.size = size;
  desc.usage = usage;
  desc.label = WGPU_STR(label);
  return (Buffer){.buffer = wgpuDeviceCreateBuffer(device, &desc),
                  .size = size};
}
void bufferDestroy(Buffer *buffer) {
  if (buffer && buffer->buffer) {
    wgpuBufferRelease(buffer->buffer);
    *buffer = (Buffer){0};
  }
}

void bufferWrite(WGPUQueue queue, Buffer *buffer, size_t size, void *data) {
  assert(buffer && buffer->size >= size);
  wgpuQueueWriteBuffer(queue, buffer->buffer, 0, data, size);
}

WGPUShaderModule compileShaderModule(WGPUDevice device, const char *src) {
  WGPUShaderModuleDescriptor desc = {0};

  WGPUShaderSourceWGSL shaderCodeDesc = {0};
  shaderCodeDesc.chain.sType = WGPUSType_ShaderSourceWGSL;
  shaderCodeDesc.code = WGPU_STR(src);
  desc.nextInChain = &shaderCodeDesc.chain;

  return wgpuDeviceCreateShaderModule(device, &desc);
}

WGPUShaderModule createShaderModule(WGPUDevice device, const char *filepath) {
  char *src = readTextFile(filepath);

  if (src == 0) {
    return 0;
  }

  WGPUShaderModule module = compileShaderModule(device, src);
  free(src);

  return module;
}

CommandBuffer createCommandBuffer(size_t capacity) {
  return (CommandBuffer){.capacity = capacity,
                         .commands =
                             malloc(sizeof(WGPUCommandBuffer) * capacity)};
}

void commandBufferDestroy(CommandBuffer *buffer) { free(buffer->commands); }

void commandBufferResize(CommandBuffer *buffer, size_t newCapacity) {
  size_t cpyLength =
      (newCapacity > buffer->capacity ? buffer->capacity : newCapacity) *
      sizeof(WGPUCommandBuffer);

  void *old = buffer->commands;
  buffer->commands = malloc(sizeof(WGPUCommandBuffer) * newCapacity);
  memcpy(buffer->commands, old, cpyLength);
  free(old);
}

void commandBufferAppend(CommandBuffer *buffer, WGPUCommandBuffer command) {
  if (buffer->size == buffer->capacity) {
    commandBufferResize(buffer, buffer->capacity + 256);
  }

  buffer->commands[buffer->size++] = command;
}

void commandBufferClear(CommandBuffer *buffer) {
  for (size_t i = 0; i < buffer->size; i++) {
    wgpuCommandBufferRelease(buffer->commands[i]);
  }
  buffer->size = 0;
}