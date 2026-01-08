#pragma once
#include "../types.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
typedef struct {
  uint32_t width;
  uint32_t height;
  GLFWwindow *window;

  uint32_t canvasWidth, canvasHeight;
} RenderInitParams;

typedef union {
  double raw[4];
  struct {
    double r, g, b, a;
  } c;
} Color;

typedef struct {
  Rect src, dst;
  uint32_t layer;
  float depth;
  TextureRef texture;
} RenderSprite;

typedef struct {
  Color clearColor;
  RenderSprite *sprites;
  size_t numsprites;
} RenderState;

int renderInit(RenderInitParams *params);
void renderFinish();

TextureRef loadTexture(const char *path);
TextureRef loadTextureArray(const char *path, uint32_t nCols, uint32_t nRows);

void renderFrame(RenderState *state);