#ifndef OSU_H
#define OSU_H

#include "circles.h"
#include "hitsounds.h"
#include "sys/stat.h"
#include <dirent.h>

inline void genOsu(void *foo) {
  Config *config = load_config();

  generateSounds(config);
  generateCircles(config);
  free_config(config);
}

#endif
