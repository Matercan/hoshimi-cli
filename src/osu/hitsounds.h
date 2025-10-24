#ifndef SOUNDS_H
#define SOUNDS_H

#include "../common/json/json_wrapper.h"
#include "../common/utils/utils.h"

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
AudioBuffer *load_audio(const char *path);

// Mix two audio buffers together
AudioBuffer *mix_audio(AudioBuffer *a, AudioBuffer *b);
// Save audio buffer to WAV file
int save_audio(const char *path, AudioBuffer *buf);

void free_audio(AudioBuffer *buf);
int generateSounds(Config *config);

#endif
