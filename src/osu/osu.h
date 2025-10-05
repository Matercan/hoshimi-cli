#ifndef OSU_H
#define OSU_H

#include "circles.h"
#include "hitsounds.h"

inline void genOsu(void *foo) {
  generateSounds(load_config()->osuSkin);
  generateCircles(NULL);
}

#endif
