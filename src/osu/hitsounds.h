#pragma once

#include "../common/json/json_wrapper.h"

#include <sndfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  float *data;
  sf_count_t frames;
  int channels;
  int samplerate;
} AudioBuffer;

// Load a WAV file into memory
AudioBuffer *load_audio(const char *path) {
  SF_INFO sfinfo;
  memset(&sfinfo, 0, sizeof(sfinfo));

  SNDFILE *file = sf_open(path, SFM_READ, &sfinfo);
  if (!file) {
    printf("Failed to open %s: %s\n", path, sf_strerror(NULL));
    return NULL;
  }

  AudioBuffer *buf = malloc(sizeof(AudioBuffer));
  buf->frames = sfinfo.frames;
  buf->channels = sfinfo.channels;
  buf->samplerate = sfinfo.samplerate;
  buf->data = malloc(sizeof(float) * sfinfo.frames * sfinfo.channels);

  sf_count_t read = sf_readf_float(file, buf->data, sfinfo.frames);
  if (read != sfinfo.frames) {
    printf("Warning: expected %ld frames, got %ld\n", (long)sfinfo.frames,
           (long)read);
  }

  sf_close(file);
  return buf;
}

// Mix two audio buffers together
AudioBuffer *mix_audio(AudioBuffer *a, AudioBuffer *b) {
  if (!a)
    return b;
  if (!b)
    return a;

  // Use the longer buffer's length
  sf_count_t max_frames = a->frames > b->frames ? a->frames : b->frames;
  int channels = a->channels > b->channels ? a->channels : b->channels;
  int samplerate = a->samplerate;

  AudioBuffer *result = malloc(sizeof(AudioBuffer));
  result->frames = max_frames;
  result->channels = channels;
  result->samplerate = samplerate;
  result->data = calloc(max_frames * channels, sizeof(float));

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

// Save audio buffer to WAV file
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
  sf_close(file);

  return 1;
}

void free_audio(AudioBuffer *buf) {
  if (buf) {
    free(buf->data);
    free(buf);
  }
}

int generateSounds(const char *skinDir) {

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
        snprintf(out_path, sizeof(out_path), "%s%s%s-hitnormal-%s.wav",
                 load_config()->osuSkin, "../osuGen/", sampleset, addition);
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
          snprintf(out_path, sizeof(out_path), "%s%s%s-hitnormal-%s-%s.wav",
                   load_config()->osuSkin, "../osuGen/", sampleset,
                   additions[a1], additions[a2]);
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
