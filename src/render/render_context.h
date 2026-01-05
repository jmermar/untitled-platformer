#pragma once
#include <dawn/webgpu.h>
#include <GLFW/glfw3.h>
#include "commands.h"

typedef struct {
    WGPUSurfaceTexture texture;
    WGPUTextureView view;
} SurfaceTextureView;

typedef struct {
    SurfaceTextureView screenSurface;
    WGPUCommandEncoder encoder;
    WGPURenderPassEncoder renderPass;
    CommandBuffer cmd;
} FrameData;

typedef struct {
    GLFWwindow* window;
    uint32_t width, height;

    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUSurface surface;
    WGPUQueue queue;
    WGPUCommandEncoder encoder;
    WGPUTextureFormat format;

    int requestAdapterEnded;

    FrameData frameData;
} RenderContext;

extern RenderContext renderContext;