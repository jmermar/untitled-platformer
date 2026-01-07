#include "textures.h"
#include "render_context.h"
#include "wgpu.h"
#include <stdio.h>
#include <stdlib.h>

TextureView textureViewCreate(const char *name, Size size, uint32_t layers,
                              WGPUTextureFormat format,
                              WGPUTextureUsage usage) {
  WGPUTextureDescriptor desc = {0};
  desc.format = format;
  desc.label = WGPU_STR(name);
  desc.mipLevelCount = 4;
  desc.size = (WGPUExtent3D){.width = size.w, .height = size.h, layers};
  desc.viewFormatCount = 1;
  desc.viewFormats = &format;
  desc.dimension = WGPUTextureDimension_2D;
  desc.usage = usage;
  desc.sampleCount = 1;
  WGPUTexture tex;
  if (!(tex = wgpuDeviceCreateTexture(renderContext.device, &desc))) {
    printf("Error creating texture\n");
    abort();
  }

  WGPUTextureViewDescriptor vdesc = {0};
  vdesc.arrayLayerCount = 1;
  vdesc.baseArrayLayer = 0;
  vdesc.baseMipLevel = 0;
  vdesc.arrayLayerCount = layers;
  vdesc.mipLevelCount = 4;
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
    return 4;
  case WGPUTextureFormat_RGBA32Float:
    return 16;
  case WGPUTextureFormat_R32Float:
    return 4;
  }
  printf("Unssuported texture format\n");
  abort();
}

void textureViewWrite(TextureView *tex, void *data) {
  size_t bytes = getTexturePixelSize(tex);
  WGPUTexelCopyTextureInfo info = {0};
  info.mipLevel = 0;
  info.aspect = tex->aspect;
  info.origin = (WGPUOrigin3D){.x = 0, .y = 0, .z = 0};
  info.texture = tex->texture;
  WGPUTexelCopyBufferLayout layout = {0};
  layout.bytesPerRow = bytes * tex->size.width;
  layout.rowsPerImage = layout.bytesPerRow * tex->size.height;
  wgpuQueueWriteTexture(renderContext.queue, &info, data,
                        layout.bytesPerRow * tex->size.height, &layout,
                        &tex->size);
}
