#include "render.h"
#include "../files.h"
#include "commands.h"
#include "sprite_renderer.h"
#include "wgpu.h"
#include <memory.h>
#include <stdlib.h>
RenderContext renderContext = {0};

int renderInit(RenderInitParams *params) {
  renderContext.width = params->width;
  renderContext.height = params->height;
  renderContext.window = params->window;
  renderContext.texturesDirty = 1;
  renderContext.numTextures = 0;
  renderContext.maxTextures = 256;
  renderContext.textures =
      malloc(sizeof(TextureView) * renderContext.maxTextures);
  if (initWGPU()) {
    return -1;
  }
  renderContext.frameData.cmd = createCommandBuffer(1024);

  if (spriteRendererCreate()) {
    renderFinish();
    return -1;
  }
  return 0;
}

void renderFinish() {
  for (int i = 0; i < renderContext.numTextures; i++) {
    textureViewDestroy(renderContext.textures + i);
  }
  free(renderContext.textures);
  renderContext.textures = 0;
  renderContext.numTextures = 0;
  commandBufferClear(&renderContext.frameData.cmd);
  finishWGPU();
}

void resizeTextures() {
  void *old = renderContext.textures;
  size_t size = renderContext.numTextures * sizeof(TextureView);
  renderContext.maxTextures *= 2;
  renderContext.textures =
      malloc(renderContext.maxTextures * sizeof(TextureView));
  memcpy(renderContext.textures, old, size);
  free(old);
}

TextureRef loadTexture(const char *path) {
  if (renderContext.numTextures == renderContext.maxTextures) {
    resizeTextures();
  }

  Bitmap *image = readImage(path);
  if (!image) {
    return 0;
  }

  renderContext.textures[renderContext.numTextures] = textureViewCreate(
      "Loaded texture", (Size){.w = image->w, .h = image->h}, image->layers,
      WGPUTextureFormat_RGBA32Float,
      WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding);

  textureViewWrite(renderContext.textures + renderContext.numTextures,
                   image->pixels);

  free(image);
  return ++renderContext.numTextures;
}

WGPUCommandEncoder wgpu_createCommandEncoder(const char *name) {
  WGPUCommandEncoderDescriptor desc = {.label = WGPU_STR(name)};

  return wgpuDeviceCreateCommandEncoder(renderContext.device, &desc);
}

void renderFrame(RenderState *state) {

  if (renderContext.texturesDirty) {
    spriteRendererUpdateTextures();
    renderContext.texturesDirty = 0;
  }

  WGPURenderPassEncoder renderPassEncoder;
  FrameData *frameData = &renderContext.frameData;

  // Init frame
  {
    frameData->screenSurface = getNextSurfaceViewData();
    frameData->encoder = wgpu_createCommandEncoder("My encoder");

    WGPURenderPassColorAttachment renderPassColorAttachment = {0};
    renderPassColorAttachment.view = frameData->screenSurface.view;
    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    renderPassColorAttachment.clearValue =
        *((WGPUColor *)((void *)state->clearColor.raw));
    WGPURenderPassDescriptor desc = {0};
    desc.colorAttachmentCount = 1;
    desc.colorAttachments = &renderPassColorAttachment;
    renderPassEncoder =
        wgpuCommandEncoderBeginRenderPass(frameData->encoder, &desc);
  }

  spriteRendererInitPass(0);
  Sprite spr = {.depth = 1,
                .idx = 0,
                .dst = {.x = 0, .y = 0, .w = 48, .h = 16},
                .src = {.x = 0, .y = 0, .w = 48, .h = 16}};
  spriteRendererDraw(&spr);
  spriteRendererEndPass(renderPassEncoder);

  // Finish Frame
  {
    FrameData *frameData = &renderContext.frameData;
    wgpuRenderPassEncoderEnd(renderPassEncoder);
    wgpuRenderPassEncoderRelease(renderPassEncoder);

    {
      WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
      cmdBufferDescriptor.nextInChain = 0;
      cmdBufferDescriptor.label = WGPU_STR("Command buffer");
      commandBufferAppend(
          &frameData->cmd,
          wgpuCommandEncoderFinish(frameData->encoder, &cmdBufferDescriptor));
    }
    wgpuCommandEncoderRelease(frameData->encoder);

    wgpuQueueSubmit(renderContext.queue, frameData->cmd.size,
                    frameData->cmd.commands);
    commandBufferClear(&frameData->cmd);

    wgpuSurfacePresent(renderContext.surface);

    wgpuTextureViewRelease(frameData->screenSurface.view);
  }
}