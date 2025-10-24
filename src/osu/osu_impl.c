#include <stdlib.h>
#define CIRCLES_IMPL
#include "circles.h"

void composite(unsigned char *dst, unsigned char *src, int w, int h, int dx, int dy, int dst_w, int dst_h) {  for (int y = 0; y < h; y++) {
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
          dst[dst_idx + c] =
              (unsigned char)((src[src_idx + c] * src_a +
                               dst[dst_idx + c] * dst_a * (1 - src_a)) /
                              out_a);
        }
        dst[dst_idx + 3] = (unsigned char)(out_a * 255);
      }
    }
  }
}

void tint_image(unsigned char *img, int w, int h, ColorRGB tint)  {
  for (int i = 0; i < w * h * 4; i += 4) {
    img[i] = (img[i] * tint.r) / 255;
    img[i + 1] = (img[i + 1] * tint.g) / 255;
    img[i + 2] = (img[i + 2] * tint.b) / 255;
    // Keep alpha unchanged
  }
}

void *genCircle(void *arg) {
  struct CircleInfo *info = (struct CircleInfo *)arg;
  int out_w = info->hw;
  int out_h = info->hh;

  unsigned char *output = (unsigned char *)calloc(out_w * out_h * 4, 1);

  if (info->num != -1) {

    // Copy and tint hitcircle
    memcpy(output, info->hitCircle, info->hw * info->hh * 4);
    tint_image(output, info->hw, info->hh, *(info->color));

    // Composite number
    int num_x = (out_w - info->nw) / 2;
    int num_y = (out_h - info->nh) / 2;
    composite(output, info->number, info->nw, info->nh, num_x, num_y, out_w,
              out_h);

    // Composite overlay
    int ov_x = (out_w - info->ow) / 2;
    int ov_y = (out_h - info->oh) / 2;
    composite(output, info->overlay, info->ow, info->oh, ov_x, ov_y, out_w,
              out_h);

    // Save output
    char outname[256];
    sprintf(outname, "%s/../osuGen/%s-%d.png", info->config->downloadPath,
            info->name, info->num);
    remove(outname);
    mkdir_recursive(info->config->downloadPath);
    stbi_write_png(outname, out_w, out_h, 4, output, out_w * 4);

  } else {

    mempcpy(output, info->hitCircle, info->hw * info->hh * 4);
    tint_image(output, info->hw, info->hh, *(info->color));

    int ov_x = (out_w - info->ow) / 2;
    int ov_y = (out_h - info->oh) / 2;
    composite(output, info->overlay, info->ow, info->oh, ov_x, ov_y, out_w,
              out_h);

    char outname[256];

    sprintf(outname, "%s/../osuGen/%s.png", info->config->downloadPath,
            info->name);
    remove(outname);
    mkdir_recursive(info->config->downloadPath);
    stbi_write_png(outname, out_w, out_h, 4, output, out_w * 4);
  }

  free(output);
  free(info);
  return NULL;
}

int generateCircles(Config *config) {
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

  const char *color_names[] = {
      "palette1",  "palette2",  "palette3",  "palette4",
      "palette5",  "palette6",  "palette7",  "palette8",
      "palette9",  "palette10", "palette11", "palette12",
      "palette13", "palette14", "palette15", "palette16"};
  int num_colors = 16;

  // Load base images
  int hw, hh, hc;
  char *osuPath = config->osuSkin;
  size_t baseLen = strlen(osuPath);

  char *hitCirclePath = (char *)malloc(baseLen + 20);
  char *circleOverlayPath = (char *)malloc(baseLen + 25);

  snprintf(hitCirclePath, baseLen + 20, "%s/hitcircle.png", osuPath);
  snprintf(circleOverlayPath, baseLen + 25, "%s/hitcircleoverlay.png", osuPath);

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

  free(hitCirclePath);
  free(circleOverlayPath);

  unsigned char *numbers[10];
  int nw[10], nh[10];

  // Load config once, not in loop
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
    snprintf(filename, sizeof(filename), "%s/fonts/hitcircle/default-%d.png",
             config->osuSkin, i);

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
  }

  pthread_t *threads = NULL;
  pthread_attr_t *attrs = NULL;
  int thread_count = 0;
  int total_threads =
      num_colors * 10; // 10 per color (1 without number + 9 with numbers)

  threads = (pthread_t *)malloc(total_threads * sizeof(pthread_t));
  attrs = (pthread_attr_t *)malloc(total_threads * sizeof(pthread_attr_t));

  // Generate circles for all combo numbers and none, and all colors
  for (int c = 0; c < num_colors; c++) {

    struct CircleInfo *info =
        (struct CircleInfo *)malloc(sizeof(struct CircleInfo));
    info->name = color_names[c];
    info->hw = hw;
    info->hh = hh;
    info->ow = ow;
    info->oh = oh;
    info->hitCircle = hitcircle;
    info->overlay = overlay;
    info->color = &(colors->palette[c]);
    info->config = config;
    info->num = -1;

    pthread_attr_init(&attrs[thread_count]);
    pthread_create(&threads[thread_count], &attrs[thread_count], genCircle,
                   (void *)info);
    thread_count++;

    for (int n = 1; n <= 9; n++) {

      struct CircleInfo *info =
          (struct CircleInfo *)malloc(sizeof(struct CircleInfo));
      info->name = color_names[c];
      info->hw = hw;
      info->hh = hh;
      info->ow = ow;
      info->oh = oh;
      info->nh = nh[n];
      info->nw = nw[n];
      info->num = n;
      info->number = numbers[n];
      info->hitCircle = hitcircle;
      info->overlay = overlay;
      info->color = &(colors->palette[c]);
      info->config = config;

      pthread_attr_init(&attrs[thread_count]);
      pthread_create(&threads[thread_count], &attrs[thread_count], genCircle,
                     (void *)info);
      thread_count++;
    }
  }

  // Wait for all threads to complete
  for (int i = 0; i < thread_count; i++) {
    pthread_join(threads[i], NULL);
  }

  // Cleanup thread resources
  for (int i = 0; i < thread_count; i++) {
    pthread_attr_destroy(&attrs[i]);
  }

  free(threads);
  free(attrs);
  free_colorscheme(colors);

  // Cleanup
  stbi_image_free(hitcircle);
  stbi_image_free(overlay);
  for (int i = 0; i < 10; i++)
    stbi_image_free(numbers[i]);

  printf("Done! Generated %d circles.\n", num_colors * 10);
  return 0;
}

#define SOUNDS_IMPL
#include "hitsounds.h"

AudioBuffer *load_audio(const char* path) {
  SF_INFO sfinfo;
  memset(&sfinfo, 0, sizeof(sfinfo));

  SNDFILE *file = sf_open(path, SFM_READ, &sfinfo);
  if (!file) {
    printf("Failed to open %s: %s\n", path, sf_strerror(NULL));
    return NULL;
  }

  AudioBuffer *buf = (AudioBuffer *)malloc(sizeof(AudioBuffer));
  buf->frames = sfinfo.frames;
  buf->channels = sfinfo.channels;
  buf->samplerate = sfinfo.samplerate;
  buf->data = (float *)malloc(sizeof(float) * sfinfo.frames * sfinfo.channels);

  sf_count_t read = sf_readf_float(file, buf->data, sfinfo.frames);
  if (read != sfinfo.frames) {
    printf("Warning: expected %ld frames, got %ld\n", (long)sfinfo.frames,
           (long)read);
  }

  sf_close(file);
  return buf;
}

AudioBuffer* mix_audio(AudioBuffer *a, AudioBuffer *b) {
  if (!a)
    return b;
  if (!b)
    return a;

  // Use the longer buffer's length
  sf_count_t max_frames = a->frames > b->frames ? a->frames : b->frames;
  int channels = a->channels > b->channels ? a->channels : b->channels;
  int samplerate = a->samplerate;

  AudioBuffer *result = (AudioBuffer *)malloc(sizeof(AudioBuffer));
  result->frames = max_frames;
  result->channels = channels;
  result->samplerate = samplerate;
  result->data = (float *)calloc(max_frames * channels, sizeof(float));

  // Mix buffer a
  for (sf_count_t i = 0; i < a->frames; i++) {
    for (int c = 0; c < a->channels && c < channels; c++) {
      result->data[i * channels + c] += a->data[i * a->channels + c];
    }
  }

  // Mix buffer b
  for (sf_count_t i = 0; i < b->frames; i++) {
    for (int c = 0; c < b->channels && c < channels; c++) {
      result->data[i * channels + c] += b->data[i * b->channels + c];
    }
  }

  // Normalize to prevent clipping
  float max_sample = 0.0f;
  for (sf_count_t i = 0; i < max_frames * channels; i++) {
    float abs_val = result->data[i] < 0 ? -result->data[i] : result->data[i];
    if (abs_val > max_sample)
      max_sample = abs_val;
  }

  if (max_sample > 1.0f) {
    for (sf_count_t i = 0; i < max_frames * channels; i++) {
      result->data[i] /= max_sample;
    }
  }

  return result;
}

int save_audio(const char *path, AudioBuffer *buf) {
  SF_INFO sfinfo;
  memset(&sfinfo, 0, sizeof(sfinfo));
  sfinfo.frames = buf->frames;
  sfinfo.channels = buf->channels;
  sfinfo.samplerate = buf->samplerate;
  sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;

  SNDFILE *file = sf_open(path, SFM_WRITE, &sfinfo);
  if (!file) {
    printf("Failed to create %s: %s\n", path, sf_strerror(NULL));
    return 0;
  }

  sf_count_t written = sf_writef_float(file, buf->data, buf->frames);
  if (!written)
    printf("Can't save audio of file %s to WAV \n", path);
  sf_close(file);

  return 1;
}

void free_audio(AudioBuffer *buf) {
  if (buf) {
    free(buf->data);
    free(buf);
  }
}

int generateSounds(Config *config) {

  const char *skinDir = config->osuSkin;

  // Define hitsound types
  const char *samplesets[] = {"normal", "soft", "drum"};
  const char *additions[] = {"hitwhistle", "hitfinish", "hitclap"};

  // Generate combined hitsounds
  for (int s = 0; s < 3; s++) {
    const char *sampleset = samplesets[s];

    // Load base hitnormal
    char base_path[512];
    snprintf(base_path, sizeof(base_path), "%s/%s-hitnormal.wav", skinDir,
             sampleset);
    AudioBuffer *base = load_audio(base_path);

    if (!base) {
      printf("Skipping %s (base not found)\n", sampleset);
      continue;
    }

    // Create combinations with additions
    for (int a = 0; a < 3; a++) {
      const char *addition = additions[a];

      char add_path[512];
      snprintf(add_path, sizeof(add_path), "%s/%s-%s.wav", skinDir, sampleset,
               addition);
      AudioBuffer *add_sound = load_audio(add_path);

      if (add_sound) {
        AudioBuffer *mixed = mix_audio(base, add_sound);

        char out_path[512];
        snprintf(out_path, sizeof(out_path), "%s/../osuGen/%s-hitnormal-%s.wav",
                 config->downloadPath, sampleset, addition);
        save_audio(out_path, mixed);

        free_audio(mixed);
        free_audio(add_sound);
      }
    }

    // Create triple combinations (whistle + finish, etc.)
    for (int a1 = 0; a1 < 3; a1++) {
      for (int a2 = a1 + 1; a2 < 3; a2++) {
        char path1[512], path2[512];
        snprintf(path1, sizeof(path1), "%s/%s-%s.wav", skinDir, sampleset,
                 additions[a1]);
        snprintf(path2, sizeof(path2), "%s/%s-%s.wav", skinDir, sampleset,
                 additions[a2]);

        AudioBuffer *add1 = load_audio(path1);
        AudioBuffer *add2 = load_audio(path2);

        if (add1 && add2) {
          AudioBuffer *temp = mix_audio(base, add1);
          AudioBuffer *final = mix_audio(temp, add2);

          char out_path[512];
          snprintf(out_path, sizeof(out_path), "%s/%s-hitnormal-%s-%s.wav",
                   config->downloadPath, sampleset, additions[a1],
                   additions[a2]);
          remove(out_path);
          mkdir_recursive(config->downloadPath);
          save_audio(out_path, final);

          free_audio(final);
          free_audio(temp);
        }

        if (add1)
          free_audio(add1);
        if (add2)
          free_audio(add2);
      }
    }

    free_audio(base);
  }

  printf("Done generating hitsounds!\n");
  return 0;
}

#define OSU_IMPL
#include "osu.h"

void genOsu(void *foo) {
  Config *config = load_config();

  generateSounds(config);
  generateCircles(config);
  free_config(config);
}

