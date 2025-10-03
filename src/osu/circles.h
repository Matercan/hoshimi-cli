#pragma once

#include "../common/json/json_wrapper.h"

#include "stb_image.h"
#include "stb_image_write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Tint an RGBA image with a color
void tint_image(unsigned char *img, int w, int h, ColorRGB tint) {
  for (int i = 0; i < w * h * 4; i += 4) {
    img[i] = (img[i] * tint.r) / 255;
    img[i + 1] = (img[i + 1] * tint.g) / 255;
    img[i + 2] = (img[i + 2] * tint.b) / 255;
    // Keep alpha unchanged
  }
}

// Composite src onto dst with alpha blending
void composite(unsigned char *dst, unsigned char *src, int w, int h, int dx, int dy, int dst_w, int dst_h) {
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      int dst_x = dx + x;
      int dst_y = dy + y;

      if (dst_x < 0 || dst_x >= dst_w || dst_y < 0 || dst_y >= dst_h)
        continue;

      int src_idx = (y * w + x) * 4;
      int dst_idx = (dst_y * dst_w + dst_x) * 4;

      float src_a = src[src_idx + 3] / 255.0f;
      float dst_a = dst[dst_idx + 3] / 255.0f;
      float out_a = src_a + dst_a * (1 - src_a);

      if (out_a > 0) {
        for (int c = 0; c < 3; c++) {
          dst[dst_idx + c] = (unsigned char)((src[src_idx + c] * src_a + dst[dst_idx + c] * dst_a * (1 - src_a)) / out_a);
        }
        dst[dst_idx + 3] = (unsigned char)(out_a * 255);
      }
    }
  }
}

int generateCircles() {
  // Load colors from JSON
  C_ColorScheme *colors = load_colorscheme();
  if (!colors) {
    printf("Failed to load colorscheme from JSON\n");
    return 1;
  }

  // Use the palette colors from your theme
  ColorRGB color_array[16];
  for (int i = 0; i < 16; i++) {
    color_array[i] = colors->palette[i];
  }

  const char *color_names[] = {"palette1", "palette2",  "palette3",  "palette4",  "palette5",  "palette6",  "palette7",  "palette8",
                               "palette9", "palette10", "palette11", "palette12", "palette13", "palette14", "palette15", "palette16"};
  int num_colors = 16;

  // Load base images
  int hw, hh, hc;
  char *osuPath = load_config()->osuSkin;
  char *osuPathCopy = strdup(osuPath); // Duplicate to avoid modifying original
  char *hitCirclePath = strcat(osuPath, "hitcircle.png");
  char *circleOverlayPath = strcat(osuPathCopy, "hitcircleoverlay.png");

  unsigned char *hitcircle = stbi_load(hitCirclePath, &hw, &hh, &hc, 4);
  if (!hitcircle) {
    printf("Failed to load %s\n", hitCirclePath);
    return 1;
  }

  int ow, oh, oc;
  unsigned char *overlay = stbi_load(circleOverlayPath, &ow, &oh, &oc, 4);
  if (!overlay) {
    printf("Failed to load %s\n", circleOverlayPath);
    stbi_image_free(hitcircle);
    return 1;
  }

  unsigned char *numbers[10];
  int nw[10], nh[10];

  // Load config once, not in loop
  Config *config = load_config();
  if (!config || !config->osuSkin) {
    printf("Failed to load config or osuSkin path\n");
    stbi_image_free(hitcircle);
    stbi_image_free(overlay);
    free_colorscheme(colors);
    return 1;
  }

  for (int i = 0; i < 10; i++) {
    // Build the full path properly
    char filename[1024];
    snprintf(filename, sizeof(filename), "%s/fonts/hitcircle/default-%d.png", config->osuSkin, i);

    printf("Loading: %s\n", filename);

    int nc;
    numbers[i] = stbi_load(filename, &nw[i], &nh[i], &nc, 4);
    if (!numbers[i]) {
      printf("Failed to load %s\n", filename);
      printf("stbi_failure_reason: %s\n", stbi_failure_reason());
      stbi_image_free(hitcircle);
      stbi_image_free(overlay);
      for (int j = 0; j < i; j++)
        stbi_image_free(numbers[j]);
      free_config(config);
      free_colorscheme(colors);
      return 1;
    }
    printf("Loaded number %d: %dx%d\n", i, nw[i], nh[i]);
  }

  // Generate circles for each color and combo number 1-9
  for (int c = 0; c < num_colors; c++) {
    for (int n = 1; n <= 9; n++) {
      // Create output buffer
      int out_w = hw;
      int out_h = hh;
      unsigned char *output = (unsigned char *)calloc(out_w * out_h * 4, 1);

      // Copy and tint hitcircle
      memcpy(output, hitcircle, hw * hh * 4);
      tint_image(output, hw, hh, colors->palette[c]);

      // Composite number
      int num_x = (out_w - nw[n]) / 2;
      int num_y = (out_h - nh[n]) / 2;
      composite(output, numbers[n], nw[n], nh[n], num_x, num_y, out_w, out_h);

      // Composite overlay
      int ov_x = (out_w - ow) / 2;
      int ov_y = (out_h - oh) / 2;
      composite(output, overlay, ow, oh, ov_x, ov_y, out_w, out_h);

      // Save output
      char outname[256];
      sprintf(outname, "%s%s%s-%d.png", load_config()->osuSkin, "../osuGen/", color_names[c], n);
      stbi_write_png(outname, out_w, out_h, 4, output, out_w * 4);
      printf("Generated %s\n", outname);

      free(output);
    }
  }

  // Cleanup
  stbi_image_free(hitcircle);
  stbi_image_free(overlay);
  for (int i = 0; i < 10; i++)
    stbi_image_free(numbers[i]);

  printf("Done! Generated %d circles.\n", num_colors * 9);
  return 0;
}
