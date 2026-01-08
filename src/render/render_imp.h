#pragma once
#include "../types.h"
#include <GLFW/glfw3.h>
#include <assert.h>
#include <dawn/webgpu.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webgpu/webgpu_glfw.h>

#define WGPU_STR(str) ((WGPUStringView){.data = str, .length = strlen(str)})

typedef struct {
  WGPUTexture texture;
  WGPUTextureView view;
  WGPUTextureFormat format;
  WGPUExtent3D size;
  WGPUTextureAspect aspect;
} TextureView;

typedef struct {
  WGPUSurfaceTexture texture;
  WGPUTextureView view;
} SurfaceTextureView;

typedef struct {
  WGPUBuffer buffer;
  size_t size;
} Buffer;

typedef struct {
  size_t capacity;
  size_t size;
  WGPUCommandBuffer *commands;
} CommandBuffer;

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

  TextureView backbuffer;

} RenderContext;

extern RenderContext renderContext;