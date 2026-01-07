#pragma once
#include "types.h"

typedef struct {
  uint32_t w, h, layers;
  char *pixels;
} Bitmap;

char *readTextFile(const char *path);
Bitmap *readImage(const char *image);