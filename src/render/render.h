#pragma once
#include "../types.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
typedef struct {
  uint32_t width;
  uint32_t height;
  GLFWwindow *window;
} RenderInitParams;

typedef union {
  double raw[4];
  struct {
    double r, g, b, a;
  } c;
} Color;

typedef struct {
  Color clearColor;
} RenderState;

int renderInit(RenderInitParams *params);
void renderFinish();

TextureRef loadTexture(const char *path);

void renderFrame(RenderState *state);