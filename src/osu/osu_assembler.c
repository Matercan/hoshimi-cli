#include "osu.h"
#include <unistd.h>

int main(const int argc, const char **argv) {
  generateCircles(NULL);
  generateSounds(load_config()->osuSkin);
}
