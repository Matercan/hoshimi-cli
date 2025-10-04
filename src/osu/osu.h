#ifndef OSU_H
#define OSU_H

#include "circles.h"
#include "hitsounds.h"

void genOsu() {
  generateSounds(load_config()->osuSkin);
  generateCircles();
}

#endif
