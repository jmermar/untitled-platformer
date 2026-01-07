#pragma once
#include <stdint.h>

typedef uint32_t TextureRef;

typedef struct {
  int32_t x, y;
  uint32_t w, h;
} Rect;

typedef struct {
  int32_t x, y;
} Point;

typedef struct {
  uint32_t w, h;
} Size;