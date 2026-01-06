#pragma once
#include "render_context.h"
#include <string.h>

#define WGPU_STR(str) ((WGPUStringView){.data=str, .length=strlen(str)})

int initWGPU();
void finishWGPU();

SurfaceTextureView getNextSurfaceViewData();