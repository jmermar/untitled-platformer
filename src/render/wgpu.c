#include "wgpu.h"
#include "webgpu/webgpu_glfw.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int configureSurface() {
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
  return 0;
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
  if (configureSurface()) {
    finishWGPU();
    return -1;
  }

  renderContext.queue = wgpuDeviceGetQueue(renderContext.device);

  return 0;
}
