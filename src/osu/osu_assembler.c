#include "circles.h"
#include "hitsounds.h"

int main() {
  generateCircles();
  generateSounds(load_config()->osuSkin);
}
