#include "files.h"
#include "stb_image.h"
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
char *readTextFile(const char *path) {
  FILE *file = fopen(path, "r");
  if (file == 0) {
    return 0;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *str = malloc(size);
  fread(str, 1, size, file);

  return str;
}

Bitmap *readImage(const char *image) {
  assert(image);

  int w, h;
  void *data = stbi_load(image, &w, &h, 0, 4);
  if (!data) {
    return 0;
  }
  size_t dataSize = w * h * 4;

  Bitmap *bmp = malloc(sizeof(Bitmap) + dataSize);
  bmp->w = w;
  bmp->h = h;
  bmp->layers = 1;
  bmp->pixels = (void *)(bmp + 1);

  memcpy(bmp->pixels, data, dataSize);
  stbi_image_free(data);

  return bmp;
}

Bitmap *readImageArray(const char *image, uint32_t columns, uint32_t rows) {
  assert(image);

  int w, h;
  void *data = stbi_load(image, &w, &h, 0, 4);
  if (!data) {
    return 0;
  }
  size_t dataSize = w * h * 4;

  Bitmap *bmp = malloc(sizeof(Bitmap) + dataSize);
  bmp->w = w / columns;
  bmp->h = h / rows;
  bmp->layers = rows * columns;
  bmp->pixels = (void *)(bmp + 1);

  size_t sizeofLayer = bmp->w * bmp->h;

  for (int row = 0; row < rows; row++) {
    for (int column = 0; column < columns; column++) {
      size_t layer = row * columns + column;
      for (int y = 0; y < bmp->h; y++) {
        for (int x = 0; x < bmp->w; x++) {
          uint32_t *dstPixel =
              ((uint32_t *)bmp->pixels) + layer * sizeofLayer + y * bmp->w + x;
          uint32_t *srcPixel = ((uint32_t *)data) + (y + row * bmp->h) * (w) +
                               x + column * bmp->w;
          *dstPixel = *srcPixel;
        }
      }
    }
  }
  stbi_image_free(data);

  return bmp;
}