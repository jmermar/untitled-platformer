#pragma once
#include "util.h"

int blitToScreenInit(WGPUDevice device, WGPUTextureFormat attachFormat,
                     TextureView *backbuffer);
void blitToScreenFinish();

void blitBufferToScreen(WGPUCommandEncoder encoder, WGPUTextureView attachView,
                        size_t attachW, size_t attachH);