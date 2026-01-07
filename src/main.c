#include "render/render.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

GLFWwindow *window = 0;
const uint32_t width = 512;
const uint32_t height = 512;

void finishWindow() {
  if (window) {
    glfwDestroyWindow(window);
  }
  glfwTerminate();
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

  RenderInitParams params = {
      .height = height, .width = width, .window = window};

  if (renderInit(&params)) {
    printf("Error trying to initialize renderer\n");
    finishWindow();
    return -1;
  }

  TextureRef tileset = loadTexture("res/textures/tileset.png");

  RenderState state;
  state.clearColor = (Color){.c = {.r = 1, .g = 0, .b = 0, .a = 1}};

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();

    renderFrame(&state);
  }

  renderFinish();
  finishWindow();
}