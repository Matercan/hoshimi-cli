#ifndef CIRCLES_H
#define CIRCLES_H

#include "../common/json/json_wrapper.h"
#include "../common/utils/utils.h"

#include "stb_image.h"
#include "stb_image_write.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct CircleInfo {
  const char *name;
  int hw;
  int hh;
  int ow;
  int oh;
  int nw;
  int nh;
  unsigned char *hitCircle;
  unsigned char *overlay;
  unsigned char *number;
  const ColorRGB *color;
  const Config *config;
  int num;
} circleinfo_t;

// Tint an RGBA image with a color
void tint_image(unsigned char *img, int w, int h, ColorRGB tint);

// Composite src onto dst with alpha blending
void composite(unsigned char *dst, unsigned char *src, int w, int h, int dx,
               int dy, int dst_w, int dst_h);

void *genCircle(void *arg);
int generateCircles(Config *config);

#endif
