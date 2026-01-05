#pragma once
#include <dawn/webgpu.h>

typedef struct {
    WGPUSurfaceTexture surface;
    WGPUTextureView texture;
} SurfaceTextureView;

