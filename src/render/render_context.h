#pragma once
#include "commands.h"
#include "textures.h"
#include <GLFW/glfw3.h>
#include <dawn/webgpu.h>

typedef struct {
  WGPUSurfaceTexture texture;
  WGPUTextureView view;
} SurfaceTextureView;

typedef struct {
  SurfaceTextureView screenSurface;
  WGPUCommandEncoder encoder;
  CommandBuffer cmd;
} FrameData;

typedef struct {
  GLFWwindow *window;
  uint32_t width, height;

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
} RenderContext;

extern RenderContext renderContext;