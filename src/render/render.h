#pragma once
#include <stdint.h>
#include <GLFW/glfw3.h>
typedef struct {
    uint32_t width;
    uint32_t height;
    GLFWwindow* window;
} RenderInitParams;

typedef union {
    double raw[4];
    struct {
        double r, g, b, a;
    }c;
} Color;

typedef struct {
    Color clearColor;
} RenderState;

int renderInit(RenderInitParams* params);
void renderFinish();

void renderFrame(RenderState* state);