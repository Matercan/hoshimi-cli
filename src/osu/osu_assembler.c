#include "osu.h"
#include <unistd.h>

int main(const int argc, const char **argv) {
  generateCircles(load_config());
  generateSounds(load_config());
}
