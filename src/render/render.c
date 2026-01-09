#include "render.h"
#include "../files.h"
#include "../types.h"
#include "blit_to_screen.h"
#include "render_imp.h"
#include "sprite_renderer.h"
#include "util.h"
#include <GLFW/glfw3.h>
#include <assert.h>
#include <dawn/webgpu.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webgpu/webgpu_glfw.h>

typedef struct {
  SurfaceTextureView screenSurface;
  WGPUCommandEncoder encoder;
  CommandBuffer cmd;
} FrameData;

typedef struct {
  GLFWwindow *window;
  uint32_t width, height;

  void *spriteRenderer;

  WGPUInstance instance;
  WGPUAdapter adapter;
  WGPUDevice device;
  WGPUSurface surface;
  WGPUQueue queue;
  WGPUCommandEncoder encoder;
  WGPUTextureFormat format;

  int requestAdapterEnded;

  WGPUBindGroup texturesBind;

  FrameData frameData;

  TextureView *textures;
  size_t numTextures;
  size_t maxTextures;
  int32_t texturesDirty;

  TextureView backbuffer;

} RenderContext;

RenderContext renderContext = {0};

int instanceWait(WGPUFuture future) {
  WGPUFutureWaitInfo wait = {.future = future};
  WGPUWaitStatus status =
      wgpuInstanceWaitAny(renderContext.instance, 1, &wait, 0);

  if (status != WGPUWaitStatus_Success) {
    return -1;
  }
  return 0;
}

void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter,
                           WGPUStringView message, void *userdata,
                           void *userdata2) {
  if (status == WGPURequestAdapterStatus_Success) {
    *((WGPUAdapter *)userdata) = adapter;
  }
}

void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice device,
                          WGPUStringView message, void *userdata1,
                          void *userdata2) {
  if (status == WGPURequestDeviceStatus_Success) {
    *((WGPUDevice *)userdata1) = device;
  }
}

void onError(WGPUDevice const *device, WGPUErrorType type,
             WGPUStringView message, WGPU_NULLABLE void *userdata1,
             WGPU_NULLABLE void *userdata2) {
  printf("WGPU ERROR: %s\n", message.data);
  abort();
}

WGPUAdapter requestAdapter() {
  WGPURequestAdapterOptions adapterOpts = {0};

  WGPUAdapter adapter = 0;

  WGPURequestAdapterCallbackInfo info = {.mode = WGPUCallbackMode_WaitAnyOnly,
                                         .callback = &onAdapterRequestEnded,
                                         .userdata1 = (void *)&adapter};

  if (instanceWait(wgpuInstanceRequestAdapter(renderContext.instance,
                                              &adapterOpts, info))) {
    return 0;
  }

  return adapter;
}

WGPUDevice requestDevice() {
  WGPUDeviceDescriptor desc = {
      .uncapturedErrorCallbackInfo.callback = onError,
  };

  WGPUDevice device = 0;

  WGPURequestDeviceCallbackInfo info = {
      .mode = WGPUCallbackMode_WaitAnyOnly,
      .callback = &onDeviceRequestEnded,
      .userdata1 = &device,
  };
  if (instanceWait(
          wgpuAdapterRequestDevice(renderContext.adapter, &desc, info))) {
    return 0;
  }
  return device;
}

void finishWGPU() {
  if (renderContext.encoder) {
    wgpuCommandEncoderRelease(renderContext.encoder);
    renderContext.encoder = 0;
  }
  if (renderContext.queue) {
    wgpuQueueRelease(renderContext.queue);
    renderContext.queue = 0;
  }
  if (renderContext.surface) {
    wgpuSurfaceUnconfigure(renderContext.surface);
    wgpuSurfaceRelease(renderContext.surface);
    renderContext.surface = 0;
  }
  if (renderContext.device) {
    wgpuDeviceDestroy(renderContext.device);
    renderContext.device = 0;
  }
  if (renderContext.adapter) {
    wgpuAdapterRelease(renderContext.adapter);
    renderContext.adapter = 0;
  }
  if (renderContext.instance) {
    wgpuInstanceRelease(renderContext.instance);
    renderContext.instance = 0;
  }
}

SurfaceTextureView getNextSurfaceViewData() {
  WGPUSurfaceTexture surfaceTexture = {0};
  wgpuSurfaceGetCurrentTexture(renderContext.surface, &surfaceTexture);

  WGPUTextureViewDescriptor viewDescriptor = {0};
  viewDescriptor.label = WGPU_STR("Surface texture view");
  viewDescriptor.format = renderContext.format;
  viewDescriptor.dimension = WGPUTextureViewDimension_2D;
  viewDescriptor.mipLevelCount = 1;
  viewDescriptor.arrayLayerCount = 1;
  viewDescriptor.aspect = WGPUTextureAspect_All;
  viewDescriptor.usage = WGPUTextureUsage_RenderAttachment;
  WGPUTextureView targetView =
      wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

  return (SurfaceTextureView){.texture = surfaceTexture, .view = targetView};
}

int initWGPU() {
  WGPUInstanceDescriptor desc = {0};

  renderContext.instance = wgpuCreateInstance(&desc);

  if (!renderContext.instance) {
    return -1;
  }

  if ((renderContext.adapter = requestAdapter()) == 0) {
    finishWGPU();
    return -1;
  }

  if ((renderContext.device = requestDevice()) == 0) {
    finishWGPU();
    return -1;
  }

  if ((renderContext.surface = wgpuGlfwCreateSurfaceForWindow(
           renderContext.instance, renderContext.window)) == 0) {
    finishWGPU();
    return -1;
  }
#pragma region CONFIGURE_SURFACE
  WGPUSurfaceCapabilities capabilities = {0};
  wgpuSurfaceGetCapabilities(renderContext.surface, renderContext.adapter,
                             &capabilities);
  WGPUSurfaceConfiguration config = {0};
  config.width = renderContext.width;
  config.height = renderContext.height;
  config.format = WGPUTextureFormat_BGRA8Unorm;
  config.usage = WGPUTextureUsage_RenderAttachment;
  config.device = renderContext.device;
  config.presentMode = WGPUPresentMode_Fifo;
  config.alphaMode = WGPUCompositeAlphaMode_Auto;
  wgpuSurfaceConfigure(renderContext.surface, &config);
  renderContext.format = capabilities.formats[0];
#pragma end

  renderContext.queue = wgpuDeviceGetQueue(renderContext.device);

  return 0;
}

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
    renderFinish();
    return -1;
  }

  renderContext.frameData.cmd = createCommandBuffer(1024);

  if ((renderContext.spriteRenderer = spriteRendererCreate(
           renderContext.device, WGPUTextureFormat_RGBA8Unorm)) == 0) {
    renderFinish();
    return -1;
  }

  renderContext.backbuffer = textureViewCreate(
      renderContext.device, "Backbuffer", params->canvasWidth,
      params->canvasHeight, 1, WGPUTextureFormat_RGBA8Unorm,
      WGPUTextureUsage_CopyDst | WGPUTextureUsage_CopySrc |
          WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_TextureBinding);

  if (blitToScreenInit(renderContext.device, WGPUTextureFormat_BGRA8Unorm,
                       &renderContext.backbuffer)) {
    renderFinish();
    return -1;
  }
  return 0;
}

void renderFinish() {
  blitToScreenFinish();
  textureViewDestroy(&renderContext.backbuffer);
  for (int i = 0; i < renderContext.numTextures; i++) {
    textureViewDestroy(renderContext.textures + i);
  }
  free(renderContext.textures);
  if (renderContext.spriteRenderer) {
    spriteRendererFinish();
  }
  renderContext.textures = 0;
  renderContext.numTextures = 0;
  commandBufferClear(&renderContext.frameData.cmd);
  finishWGPU();
}

TextureRef loadTexture(const char *path) {
  return loadTextureArray(path, 1, 1);
}

TextureRef loadTextureArray(const char *path, uint32_t nCols, uint32_t nRows) {
  if (renderContext.numTextures == renderContext.maxTextures) {
#pragma region RESIZE TEXTURES
    void *old = renderContext.textures;
    size_t size = renderContext.numTextures * sizeof(TextureView);
    renderContext.maxTextures *= 2;
    renderContext.textures =
        malloc(renderContext.maxTextures * sizeof(TextureView));
    memcpy(renderContext.textures, old, size);
    free(old);
#pragma end
  }

  Bitmap *image = readImageArray(path, nCols, nRows);
  if (!image) {
    return 0;
  }

  renderContext.textures[renderContext.numTextures] = textureViewCreate(
      renderContext.device, "Loaded texture", image->w, image->h, image->layers,
      WGPUTextureFormat_RGBA8Unorm,
      WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding);

  textureViewWrite(renderContext.queue,
                   renderContext.textures + renderContext.numTextures,
                   image->pixels);

  free(image);
  return ++renderContext.numTextures;
}

void renderFrame(RenderState *state) {

  Rect viewport = {.x = 0,
                   .y = 0,
                   .h = renderContext.backbuffer.size.height,
                   .w = renderContext.backbuffer.size.width};

  if (renderContext.texturesDirty) {
    spriteRendererUpdateTextures(renderContext.textures,
                                 renderContext.numTextures);
    renderContext.texturesDirty = 0;
  }

  WGPURenderPassEncoder renderPassEncoder;
  FrameData *frameData = &renderContext.frameData;

  // Init frame
  {
    frameData->screenSurface = getNextSurfaceViewData();

    WGPUCommandEncoderDescriptor commandEncoderDesc = {0};

    frameData->encoder = wgpuDeviceCreateCommandEncoder(renderContext.device,
                                                        &commandEncoderDesc);

    WGPURenderPassColorAttachment renderPassColorAttachment = {0};
    renderPassColorAttachment.view = renderContext.backbuffer.view;
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

  TextureRef prevTexture = 0;
  for (int i = 0; i < state->numsprites; i++) {
    RenderSprite *spr = state->sprites + i;
    if (spr->texture != prevTexture) {
      if (prevTexture) {
        spriteRendererEndPass(renderContext.queue, renderPassEncoder);
      }
      spriteRendererInitPass(
          spr->texture - 1,
          (Size){.w = renderContext.textures[spr->texture - 1].size.width,
                 .h = renderContext.textures[spr->texture - 1].size.height});
      prevTexture = spr->texture;
    }
    Sprite s = {.depth = spr->depth,
                .dst = spr->dst,
                .src = spr->src,
                .idx = spr->layer,
                .depth = spr->depth};

    spriteRendererDraw(&s, &viewport);
  }
  if (prevTexture) {
    spriteRendererEndPass(renderContext.queue, renderPassEncoder);
  }

  // Finish Frame
  {
    wgpuRenderPassEncoderEnd(renderPassEncoder);
    blitBufferToScreen(frameData->encoder, frameData->screenSurface.view,
                       renderContext.width, renderContext.height);
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
