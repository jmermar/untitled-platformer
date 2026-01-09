#pragma once
#include "../types.h"
#include "util.h"

typedef uint32_t TilemapRef;

void *tilemapRendererInit(WGPUDevice device, WGPUTextureFormat pipelineFormat);
void tilemapRendererFinish();

TilemapRef tilemapRendererCreateTilemap(TextureView *texture, uint32_t w,
                                        uint32_t h, void *data);
void tilemapRendererClearTilemaps();

void tilemapRendererUpdateTextures(TextureView *textures, TilemapRef tilemap);

void tilemapRendererRenderLayer(WGPUQueue queue,
                                WGPURenderPassEncoder renderPass);
