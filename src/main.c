#include "render/render.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

GLFWwindow *window = 0;
const uint32_t width = 320 * 4;
const uint32_t height = 240 * 4;

RenderSprite sprites[1024 * 1024];
size_t numSprites = 0;
TextureRef tileset;

void finishWindow() {
  if (window) {
    glfwDestroyWindow(window);
  }
  glfwTerminate();
}

void drawTile(uint32_t tileID, Point pos) {
  sprites[numSprites++] = (RenderSprite){
      .dst = {.x = pos.x * 16, .y = pos.y * 16, .w = 16, .h = 16},
      .src = {.x = 0, .y = 0, .w = 16, .h = 16},
      .depth = 1,
      .layer = tileID,
      .texture = tileset};
}

int initWindow() {
  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  if ((window = glfwCreateWindow(width, height, "Wgpu!", 0, 0)) == 0) {
    finishWindow();
    return -1;
  }

  return 0;
}

int main() {
  if (initWindow()) {
    printf("Error initializing system");
    return -1;
  }

  RenderInitParams params = {.height = height,
                             .width = width,
                             .window = window,
                             .canvasHeight = 240,
                             .canvasWidth = 320};

  if (renderInit(&params)) {
    printf("Error trying to initialize renderer\n");
    finishWindow();
    return -1;
  }

  tileset = loadTextureArray("res/textures/tileset.png", 4, 4);

  RenderState state = {0};
  state.sprites = sprites;
  state.clearColor = (Color){.c = {.r = 0, .g = 0, .b = 0, .a = 1}};

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    for (int y = 0; y < (height / 16) + 1; y++) {
      for (int x = 0; x < (width / 16) + 1; x++) {
        uint32_t id = 0;
        if (y < 9)
          continue;
        if (y < 10)
          id = 0;
        else if (y < 12)
          id = 1;
        else
          id = 2;
        drawTile(id, (Point){.x = x, .y = y});
      }
    }

    state.numsprites = numSprites;

    renderFrame(&state);

    numSprites = 0;
  }

  renderFinish();
  finishWindow();
}
